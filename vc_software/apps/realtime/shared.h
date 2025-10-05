#ifndef SHARED_H
#define SHARED_H

#include <mutex>
#include <atomic>
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
    // 상태
    std::atomic<bool> running{true};
    std::atomic<bool> engine_on{false};

    // 입력
    JoystickData joy{};
    SensorData   sensor{};

    // 제어모드
    ControlMode mode = ControlMode::Assist;

    // 출력
    ControlOutput out{};

    // 보호
    std::mutex mtx;
};

extern SharedData g_shared;

#endif
