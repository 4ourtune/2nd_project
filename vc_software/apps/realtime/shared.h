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
 


// ---------------- LED 구분 상수 ----------------
enum LedSide {
    LED_BACK = 0,       // 후방 LED
    LED_FRONT_DOWN = 1, // 전방 전조등
    LED_FRONT_UP = 2    // 전방 하이빔
};

enum class ControlMode : uint8_t { Manual=0, Assist=1, Auto=2 };

struct ControlOutput {
    // buzzer
    bool buzzerOn = false;
    int32_t frequency = 500; // 250~1000Hz

    // led
    bool led_back_on = false;
    bool led_front_down_on = false;
    bool led_front_up_on = false;

    // emerAlert
    int64_t alert_interval_ms = -1; // -1=OFF, 0=항상 켜짐, >0=토글 간격

    // motor
    int throttle = 0; // -100~+100
    int steer    = 0; // -100~+100
};

struct SharedData {
    // 보호
    std::mutex mtx;

    // 상태
    bool engine_on = false;
    bool door_locked = false;
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
