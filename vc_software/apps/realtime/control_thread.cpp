#include "shared.h"
#include "vc_common.h"
#include "config.h"
#include <algorithm>

static int map_joy_to_throttle(int y){ // 0~99 -> -100~+100
    return (y - 50) * 2;
}
static int map_joy_to_steer(int x){    // 0~99 -> -100~+100
    return (x - 50) * 2;
}

void control_thread(){
    set_realtime_sched(PRIO_CONTROL);

    uint64_t last_tick = now_ms();
    while(g_shared.running){
        uint64_t t = now_ms();
        int lapse = (int)(t - last_tick);
        if (lapse < PERIOD_CTRL_MS){
            sleep_ms(1);
            continue;
        }
        last_tick = t;

        bool engine=false;
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

        if (!engine){
            // 엔진 OFF → 정지/소등
            out = ControlOutput{};
        } else {
            // HBA: 어두우면 켜기 (stub)
            out.headlamp_on = (sen.ambient_lux < 50);

            // AEB: 근접 시 강제 제동
            if (sen.dist_cm <= AEB_DISTANCE_CM){
                out.aeb_brake = true;
                out.throttle  = -100; // 풀 브레이크
                out.steer     = 0;
            } else {
                // Manual/Assist 기본값: 조이스틱 반영
                int thr = map_joy_to_throttle(joy.y);
                int str = map_joy_to_steer(joy.x);

                // Assist라면 약간의 제약/필터를 둘 수 있음(예: 급조향 제한)
                if (mode == ControlMode::Assist){
                    str = std::clamp(str, -80, 80);
                }

                out.throttle = std::clamp(thr, -100, 100);
                out.steer    = std::clamp(str, -100, 100);
            }
        }

        out.ts_ms = now_ms();
        {
            std::lock_guard<std::mutex> lk(g_shared.mtx);
            g_shared.out = out;
        }
    }
}
