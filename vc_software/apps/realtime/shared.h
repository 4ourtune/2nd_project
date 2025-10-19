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
    int ambient_lux;    // 조도(Stub)
    int front_tof_mm;  // 전방 ToF (mm)
    int left_ultra_mm; // 좌측 초음파 (mm)
    int right_ultra_mm; // 우측 초음파 (mm)
    int rear_ultra_mm;  // 후방 초음파 (mm)
    uint64_t ts_ms;
    uint64_t front_ts_us;
    uint64_t left_ts_us;
    uint64_t right_ts_us;
    uint64_t rear_ts_us;
};
 


enum class ControlMode : uint8_t { Manual=0, Assist=1, Auto=2 };

struct ControlOutput {
    // buzzer
    bool buzzerOn;
    int32_t frequency; // 250 ~ 1000
    // led
    int side; // LED_BACK = 0, LED_FRONT_DOWN = 1, LED_FRONT_UP = 2
    bool isOn;
    // emerAlert
    int64_t interval_ms; // 알람 토글 시간 간격 값. 0이면 계속 켜짐. -1이면 꺼짐
    // motor
    int throttle; // -100~+100
    int steer;    // -100~+100
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
