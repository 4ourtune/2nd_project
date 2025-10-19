#include "shared.h"
#include "vc_common.h"
#include "config.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace {

constexpr uint64_t kSensorIntervalUs  = static_cast<uint64_t>(PERIOD_SENSOR_MS) * 1000;

static int map_joy_to_throttle(int y){ // 0~99 -> -100~+100
    return std::clamp((y - 50) * 2, -100, 100);
}
static int map_joy_to_steer(int x){    // 0~99 -> -100~+100
    return std::clamp((x - 50) * 2, -100, 100);
}

class AebController {
public:
    bool update(int distance_mm, int forward_speed){
        if (distance_mm < 0){
            state_ = State::Normal;
            return false;
        }

        if (distance_mm <= kEmergencyThresholdMm + kToleranceMm){
            state_ = State::Emergency;
            return true;
        }

        if (forward_speed <= 0){
            state_ = State::Normal;
            return false;
        }

        long long speed_sq = static_cast<long long>(forward_speed) * forward_speed;
        long long numerator = static_cast<long long>(kCoeffA) * speed_sq
                            + static_cast<long long>(kCoeffB) * forward_speed
                            + static_cast<long long>(kCoeffC);
        if (numerator < 0){
            numerator = 0;
        }

        unsigned int braking_distance = static_cast<unsigned int>(numerator / kSpeedDivider);
        if (distance_mm <= static_cast<int>(braking_distance) + kEmergencyThresholdMm){
            state_ = State::Emergency;
        } else {
            state_ = State::Normal;
        }
        return state_ == State::Emergency;
    }

private:
    enum class State { Normal, Emergency };

    static constexpr int kToleranceMm          = 5;
    static constexpr int kEmergencyThresholdMm = 100;
    static constexpr int kCoeffA               = -27;
    static constexpr int kCoeffB               = 6496;
    static constexpr int kCoeffC               = -112642;
    static constexpr int kSpeedDivider         = 1000;

    State state_ = State::Normal;
};

class AutoParkingController {
public:
    void start(){
        reset();
        active_ = true;
    }

    void stop(){
        active_ = false;
        reset();
    }

    bool active() const { return active_; }

    void step(uint64_t now_us, const SensorData& sensor, uint64_t sensor_interval_us){
        if (!active_){
            result_x_ = 50;
            result_y_ = 50;
            sensors_ready_ = false;
            return;
        }

        if (!updateSensors(now_us, sensor, sensor_interval_us)){
            result_x_ = 50;
            result_y_ = 50;
            return;
        }

        computeOutputs(now_us);
    }

    int throttlePercent() const { return mapJoystickValue(result_y_); }
    int steerPercent() const { return mapJoystickValue(result_x_); }
    bool completed() const { return done_; }
    bool sensorsReady() const { return sensors_ready_; }

private:
    enum class ParkingPhase { SpaceDetection, ParkingExecution, Completed };
    enum class WallSpaceState { WallDetected, SpaceDetected };

    struct Config {
        int wall_threshold_mm;
        float vehicle_speed_cm_per_ms;
        float min_space_size_cm;
        int rear_safety_distance_mm;
        int rotate_limit;
    };

    void reset(){
        done_ = false;
        sensors_ready_ = false;
        phase_ = ParkingPhase::SpaceDetection;
        state_ = WallSpaceState::WallDetected;
        execution_state_ = 1;
        wall_reference_initialized_ = false;
        wall_reference_distance_mm_ = 0;
        space_start_time_us_ = 0;
        space_end_time_us_ = 0;
        measured_space_size_cm_ = 0.0f;
        rotate_counter_ = 0;
        result_x_ = 50;
        result_y_ = 50;
        sense_dist_.fill(-1);
        sense_time_.fill(0);
    }

    bool updateSensors(uint64_t now_us, const SensorData& sensor, uint64_t sensor_interval_us){
        auto assign_sensor = [&](int idx, int distance_mm, uint64_t ts_us){
            sense_dist_[idx] = distance_mm;
            sense_time_[idx] = ts_us != 0 ? ts_us : now_us;
        };

        assign_sensor(0,
                      sensor.front_tof_mm >= 0 ? sensor.front_tof_mm
                                               : (sensor.dist_cm >= 0 ? sensor.dist_cm * 10 : -1),
                      sensor.front_ts_us);
        assign_sensor(1, sensor.left_ultra_mm, sensor.left_ts_us);
        assign_sensor(2, sensor.right_ultra_mm, sensor.right_ts_us);
        assign_sensor(3, sensor.rear_ultra_mm, sensor.rear_ts_us);

        sensors_ready_ = (sense_dist_[1] >= 0 && sense_dist_[3] >= 0);
        if (!sensors_ready_){
            return false;
        }

        uint64_t max_age_us = sensor_interval_us * 5;
        for (size_t i = 0; i < sense_time_.size(); ++i){
            if (sense_time_[i] == 0){
                sense_time_[i] = now_us;
            }
            if (now_us >= sense_time_[i] && (now_us - sense_time_[i]) > max_age_us){
                sensors_ready_ = false;
                return false;
            }
        }
        sensors_ready_ = true;
        return true;
    }

    WallSpaceState analyzeSpace(int distance_mm){
        if (distance_mm < 0){
            return WallSpaceState::WallDetected;
        }

        if (!wall_reference_initialized_){
            wall_reference_distance_mm_ = distance_mm;
            wall_reference_initialized_ = true;
            return WallSpaceState::WallDetected;
        }

        if (distance_mm < wall_reference_distance_mm_){
            wall_reference_distance_mm_ = distance_mm;
        }

        if (distance_mm > wall_reference_distance_mm_ + cfg_.wall_threshold_mm){
            return WallSpaceState::SpaceDetected;
        }
        return WallSpaceState::WallDetected;
    }

    void processStateTransition(WallSpaceState new_state, uint64_t now_us){
        if (state_ == new_state){
            return;
        }

        if (state_ == WallSpaceState::WallDetected && new_state == WallSpaceState::SpaceDetected){
            space_start_time_us_ = now_us;
        } else if (state_ == WallSpaceState::SpaceDetected && new_state == WallSpaceState::WallDetected){
            space_end_time_us_ = now_us;
            measured_space_size_cm_ = calculateSpaceSize(space_start_time_us_, space_end_time_us_);
        }
        state_ = new_state;
    }

    bool detectParkingSpace(uint64_t now_us){
        WallSpaceState analyzed = analyzeSpace(sense_dist_[1]);
        processStateTransition(analyzed, now_us);
        return measured_space_size_cm_ >= cfg_.min_space_size_cm;
    }

    float calculateSpaceSize(uint64_t start_us, uint64_t end_us) const{
        if (end_us <= start_us){
            return 0.0f;
        }
        float diff_ms = static_cast<float>(end_us - start_us) / 1000.0f;
        return diff_ms * cfg_.vehicle_speed_cm_per_ms;
    }

    void computeOutputs(uint64_t now_us){
        int left_distance = sense_dist_[1];
        int rear_distance = sense_dist_[3];

        switch (phase_){
        case ParkingPhase::SpaceDetection:
            result_x_ = 50;
            result_y_ = 70;
            if (detectParkingSpace(now_us)){
                phase_ = ParkingPhase::ParkingExecution;
                execution_state_ = 1;
                rotate_counter_ = 0;
                wall_reference_initialized_ = false;
                state_ = WallSpaceState::WallDetected;
            }
            break;

        case ParkingPhase::ParkingExecution:
            if (execution_state_ == 1){
                state_ = analyzeSpace(left_distance);
                if (state_ == WallSpaceState::SpaceDetected){
                    result_x_ = 50;
                    result_y_ = 50;
                    execution_state_ = 2;
                } else {
                    result_x_ = 50;
                    result_y_ = 35;
                }
            } else {
                if (rotate_counter_ < cfg_.rotate_limit){
                    result_x_ = 64;
                    result_y_ = 45;
                    ++rotate_counter_;
                } else {
                    result_x_ = 50;
                    result_y_ = 35;
                }

                if (rotate_counter_ >= cfg_.rotate_limit &&
                    rear_distance >= 0 &&
                    rear_distance <= cfg_.rear_safety_distance_mm){
                    result_x_ = 50;
                    result_y_ = 50;
                    phase_ = ParkingPhase::Completed;
                    done_ = true;
                }
            }
            break;

        case ParkingPhase::Completed:
            result_x_ = 50;
            result_y_ = 50;
            done_ = true;
            break;
        }
    }

    static int mapJoystickValue(int value){
        value = std::clamp(value, 0, 99);
        return (value * 200 / 99) - 100;
    }

    const Config cfg_{100, 0.5f, 150.0f, 100, 30};
    bool active_ = false;
    bool done_ = false;
    bool sensors_ready_ = false;
    ParkingPhase phase_ = ParkingPhase::SpaceDetection;
    WallSpaceState state_ = WallSpaceState::WallDetected;
    int execution_state_ = 1;
    bool wall_reference_initialized_ = false;
    int wall_reference_distance_mm_ = 0;
    uint64_t space_start_time_us_ = 0;
    uint64_t space_end_time_us_ = 0;
    float measured_space_size_cm_ = 0.0f;
    int rotate_counter_ = 0;
    int result_x_ = 50;
    int result_y_ = 50;
    std::array<int, 4> sense_dist_{};
    std::array<uint64_t, 4> sense_time_{};
};

int resolve_front_distance_mm(const SensorData& sen){
    if (sen.front_tof_mm >= 0){
        return sen.front_tof_mm;
    }
    if (sen.dist_cm >= 0){
        return sen.dist_cm * 10;
    }
    return -1;
}

} // namespace

void control_thread(){
    set_realtime_sched(PRIO_CONTROL);

    AutoParkingController aps;
    AebController aeb;
    ControlMode last_mode = ControlMode::Assist;

    uint64_t last_tick_ms = now_ms();
    while (g_shared.running){
        uint64_t t_ms = now_ms();
        int lapse = static_cast<int>(t_ms - last_tick_ms);
        if (lapse < PERIOD_CTRL_MS){
            sleep_ms(1);
            continue;
        }
        last_tick_ms = t_ms;

        bool engine = false;
        JoystickData joy{};
        SensorData sen{};
        ControlMode mode;
        {
            std::lock_guard<std::mutex> lk(g_shared.mtx);
            engine = g_shared.engine_on;
            joy    = g_shared.joy;
            sen    = g_shared.sensor;
            mode   = g_shared.mode;
        }

        ControlOutput out{};

        out.front_low_beam_on  = engine;
        out.front_high_beam_on = false;
        out.rear_alert_on = false;
        out.buzzer_on = false;

        if (!engine){
            aps.stop();
        } else {
            if (mode == ControlMode::Auto && last_mode != ControlMode::Auto){
                aps.start();
            } else if (last_mode == ControlMode::Auto && mode != ControlMode::Auto){
                aps.stop();
            }
            if (mode == ControlMode::Auto && !aps.active()){
                aps.start();
            }

            out.front_high_beam_on = (sen.ambient_lux < 50);

            uint64_t now_us = t_ms * 1000;
            if (mode == ControlMode::Auto && aps.active()){
                aps.step(now_us, sen, kSensorIntervalUs);
                out.throttle = aps.throttlePercent();
                out.steer    = aps.steerPercent();
            } else {
                int thr = map_joy_to_throttle(joy.y);
                int str = map_joy_to_steer(joy.x);

                if (mode == ControlMode::Assist){
                    str = std::clamp(str, -80, 80);
                }

                out.throttle = std::clamp(thr, -100, 100);
                out.steer    = std::clamp(str, -100, 100);
            }

            int front_distance_mm = resolve_front_distance_mm(sen);
            int forward_speed = std::max(out.throttle, 0);
            if (aeb.update(front_distance_mm, forward_speed)){
                out.aeb_brake = true;
                out.throttle  = -100;
            } else {
                out.aeb_brake = false;
            }

            out.rear_alert_on = out.aeb_brake;
            out.buzzer_on     = out.aeb_brake;
        }

        last_mode = mode;

        out.ts_ms = t_ms;
        {
            std::lock_guard<std::mutex> lk(g_shared.mtx);
            g_shared.out = out;
        }
    }
}
