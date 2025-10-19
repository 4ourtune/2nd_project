#ifndef SHARED_H
#define SHARED_H

#include <mutex>
#include <cstdint>

struct JoystickData {
    int x = 0;     // 0~99
    int y = 0;     // 0~99
    uint64_t ts_ms = 0;
};

struct SensorData {
    int dist_cm = 999;      // 앞 장애물 거리(Stub)
    int ambient_lux = 0;    // 조도(Stub)
    uint64_t ts_ms = 0;
};

enum class ControlMode : uint8_t { Manual=0, Assist=1, Auto=2 };

struct ControlOutput {
    int throttle = 0; // -100~+100
    int steer = 0;    // -100~+100
    bool headlamp_on = false;
    bool aeb_brake = false;
    uint64_t ts_ms = 0;
};

struct SharedData {
    // 보호
    std::mutex mtx;

    // 상태
    bool engine_on = false;
    bool door_locked = true;
    bool running = true;

    // 입력
    JoystickData joy{};
    SensorData   sensor{};

    // 제어모드
    ControlMode mode = ControlMode::Assist;

    // 출력
    ControlOutput out{};
};

extern SharedData g_shared;

#endif
