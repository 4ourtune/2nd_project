#include "shared.h"
#include "vc_common.h"
#include "config.h"

#include <algorithm>
#include <cstdint>
#include <array>
#include <iostream>

namespace {

// ====================================
// std::clamp() 대체 함수 (C++11/14용)
// ====================================
template <typename T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return (v < lo) ? lo : (hi < v ? hi : v);
}

// ====================================
// 상수 정의
// ====================================
constexpr uint64_t kSensorIntervalUs = static_cast<uint64_t>(PERIOD_SENSOR_MS) * 1000;
constexpr int32_t  kAebBuzzerFreqHz  = 500;

// ====================================
// AEB Controller
// ====================================
class AebController {
public:
    bool update(int distance_mm, int forward_speed) {
        if (distance_mm < 0) {
            state_ = State::Normal;
            return false;
        }

        if (distance_mm <= kEmergencyThresholdMm + kToleranceMm) {
            state_ = State::Emergency;
            return true;
        }

        if (forward_speed <= 0) {
            state_ = State::Normal;
            return false;
        }

        long long speed_sq = static_cast<long long>(forward_speed) * forward_speed;
        long long numerator = static_cast<long long>(kCoeffA) * speed_sq
                            + static_cast<long long>(kCoeffB) * forward_speed
                            + static_cast<long long>(kCoeffC);
        if (numerator < 0) numerator = 0;

        unsigned int braking_distance = static_cast<unsigned int>(numerator / kSpeedDivider);
        if (distance_mm <= static_cast<int>(braking_distance) + kEmergencyThresholdMm) {
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

// ====================================
// Auto Parking Controller (APS)
// ====================================
class AutoParkingController {
public:
    void start() {
        reset();
        active_ = true;
    }

    void stop() {
        active_ = false;
        reset();
    }

    bool active() const { return active_; }

    void step(uint64_t now_us, const SensorData& sensor, uint64_t sensor_interval_us) {
        if (!active_) {
            result_x_ = 50;
            result_y_ = 50;
            sensors_ready_ = false;
            return;
        }

        if (!updateSensors(now_us, sensor, sensor_interval_us)) {
            result_x_ = 50;
            result_y_ = 50;
            return;
        }

        computeOutputs(now_us);
    }

    int throttlePercent() const { return mapJoystickValue(result_y_); }
    int steerPercent() const { return mapJoystickValue(result_x_); }
    bool sensorsReady() const { return sensors_ready_; }

private:
    enum class ParkingPhase { SpaceDetection, ParkingExecution, Completed };

    struct Config {
        int wall_threshold_mm;
        float vehicle_speed_cm_per_ms;
        float min_space_size_cm;
        int rear_safety_distance_mm;
        int rotate_limit;
    };

    void reset() {
        sensors_ready_ = false;
        phase_ = ParkingPhase::SpaceDetection;
        wall_reference_initialized_ = false;
        wall_reference_distance_mm_ = 0;
        rotate_counter_ = 0;
        result_x_ = 50;
        result_y_ = 50;
        sense_dist_.fill(-1);
        sense_time_.fill(0);
    }

    bool updateSensors(uint64_t now_us, const SensorData& sensor, uint64_t sensor_interval_us) {
        auto assign_sensor = [&](int idx, int distance_mm, uint64_t ts_us) {
            sense_dist_[idx] = distance_mm;
            sense_time_[idx] = ts_us != 0 ? ts_us : now_us;
        };

        assign_sensor(0, sensor.front_tof_mm, sensor.front_ts_us);
        assign_sensor(1, sensor.left_ultra_mm, sensor.left_ts_us);
        assign_sensor(2, sensor.right_ultra_mm, sensor.right_ts_us);
        assign_sensor(3, sensor.rear_ultra_mm, sensor.rear_ts_us);

        sensors_ready_ = (sense_dist_[1] >= 0 && sense_dist_[3] >= 0);
        if (!sensors_ready_) return false;

        uint64_t max_age_us = sensor_interval_us * 5;
        for (size_t i = 0; i < sense_time_.size(); ++i) {
            if (now_us >= sense_time_[i] && (now_us - sense_time_[i]) > max_age_us) {
                sensors_ready_ = false;
                return false;
            }
        }
        return true;
    }

    void computeOutputs(uint64_t now_us) {
        if (phase_ == ParkingPhase::SpaceDetection) {
            result_x_ = 50;
            result_y_ = 70; // 전진 탐색
            if (++rotate_counter_ > 40) {
                phase_ = ParkingPhase::ParkingExecution;
                rotate_counter_ = 0;
            }
        } else if (phase_ == ParkingPhase::ParkingExecution) {
            result_x_ = 64;
            result_y_ = 45; // 후진 회전
            if (++rotate_counter_ > 30) {
                phase_ = ParkingPhase::Completed;
            }
        } else {
            result_x_ = 50;
            result_y_ = 50; // 정지
        }
    }

    static int mapJoystickValue(int value) {
        value = clamp(value, 0, 99);
        return (value * 200 / 99) - 100;
    }

    const Config cfg_{100, 0.5f, 150.0f, 100, 30};
    bool active_ = false;
    bool sensors_ready_ = false;
    ParkingPhase phase_ = ParkingPhase::SpaceDetection;
    bool wall_reference_initialized_ = false;
    int wall_reference_distance_mm_ = 0;
    int rotate_counter_ = 0;
    int result_x_ = 50;
    int result_y_ = 50;
    std::array<int, 4> sense_dist_{};
    std::array<uint64_t, 4> sense_time_{};
};

// ====================================
// High Beam Assist (HBA)
// ====================================
class HighBeamAssist {
public:
    bool shouldEnableFrontHighBeam(const SensorData& sensor) const {
        int val = 4095 - sensor.ambient_lux;
        return (val >= 0 && val < kLuxThreshold);
    }
private:
    static constexpr int kLuxThreshold = 50;
};

// ====================================
// Control Loop
// ====================================
class ControlLoop {
public:
    void run();

private:
    struct ControlInputs {
        SensorData sensor{};
        uint64_t now_ms = 0;
    };

    ControlInputs snapshotInputs(uint64_t now_ms_value);
    ControlOutput computeOutput(const ControlInputs& inputs);
    void initializeOutput(ControlOutput& out) const;
    void publishOutput(const ControlOutput& out);

    AutoParkingController aps_;
    AebController aeb_;
    HighBeamAssist hba_;
};

void ControlLoop::run() {
    set_realtime_sched(PRIO_CONTROL);

    aps_.start(); // APS 항상 활성화
    uint64_t next_tick_ms = now_ms();

    while (g_shared.running) {
        const uint64_t t_ms = now_ms();
        if (t_ms < next_tick_ms) {
            sleep_ms(1);
            continue;
        }
        next_tick_ms = t_ms + PERIOD_CTRL_MS;

        ControlInputs inputs = snapshotInputs(t_ms);
        ControlOutput out = computeOutput(inputs);
        publishOutput(out);
    }
}

ControlLoop::ControlInputs ControlLoop::snapshotInputs(uint64_t now_ms_value) {
    ControlInputs inputs;
    inputs.now_ms = now_ms_value;
    std::lock_guard<std::mutex> lk(g_shared.mtx);
    inputs.sensor = g_shared.sensor;
    return inputs;
}

void ControlLoop::initializeOutput(ControlOutput& out) const {
    out.buzzerOn = false;
    out.frequency = 0;
    out.alert_interval_ms = -1;
    out.throttle = 0;
    out.steer = 0;
    out.led_back_on = false;
    out.led_front_down_on = false;
    out.led_front_up_on = false;
}

ControlOutput ControlLoop::computeOutput(const ControlInputs& inputs) {
    ControlOutput out{};
    initializeOutput(out);

    // --- APS ---
    aps_.step(inputs.now_ms * 1000, inputs.sensor, kSensorIntervalUs);
    if (aps_.sensorsReady()) {
        out.throttle = aps_.throttlePercent();
        out.steer = aps_.steerPercent();
    }

    // --- HBA ---
    const bool high_beam_active = hba_.shouldEnableFrontHighBeam(inputs.sensor);
    out.led_front_up_on = high_beam_active;
    out.led_front_down_on = true;

    // --- AEB ---
    int front_distance_mm = inputs.sensor.front_tof_mm;
    int forward_speed = std::max(out.throttle, 0);
    if (aeb_.update(front_distance_mm, forward_speed)) {
        out.buzzerOn = true;
        out.frequency = kAebBuzzerFreqHz;
        out.alert_interval_ms = 0;
        out.throttle = -100;
        out.led_back_on = true;

        //  디버그 로그 추가
        std::cout << "[AEB] Emergency triggered! "
                  << "Distance=" << front_distance_mm << "mm, "
                  << "Speed=" << forward_speed << "% → Brake!"
                  << std::endl;
    } else {
        //  Normal 상태 로그 (선택사항)
        static int cnt = 0;
        if (++cnt % 20 == 0) { // 너무 많이 찍히지 않게
            std::cout << "[AEB] Normal. Distance=" << front_distance_mm
                      << "mm, Speed=" << forward_speed << "%" << std::endl;
        }
    }

    return out;
}

void ControlLoop::publishOutput(const ControlOutput& out) {
    std::lock_guard<std::mutex> lk(g_shared.mtx);
    g_shared.out = out;
}

} // namespace

// ====================================
// control_thread() Entry
// ====================================
void control_thread() {
    ControlLoop loop;
    loop.run();
}
