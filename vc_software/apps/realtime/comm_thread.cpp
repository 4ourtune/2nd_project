#include "shared.h"
#include "vc_common.h"
#include "config.h"
#include <iostream>

void comm_thread(){
    set_realtime_sched(PRIO_COMM);

    uint64_t last = 0;
    while(g_shared.running){
        sleep_ms(PERIOD_COMM_MS);

        ControlOutput out{};
        bool engine=false;
        {
            std::lock_guard<std::mutex> lk(g_shared.mtx);
            out = g_shared.out;
            engine = g_shared.engine_on;
        }

        // Stub: 여기에 SOME/IP TX로 교체
        // EX: send_control(throttle, steer, lamp, aeb)
        if (engine){
            std::cout << "[TX] thr=" << out.throttle
                      << " str=" << out.steer
                      << " low=" << out.front_low_beam_on
                      << " high=" << out.front_high_beam_on
                      << " rear_alert=" << out.rear_alert_on
                      << " buzzer=" << out.buzzer_on
                      << " aeb=" << out.aeb_brake << "\n";
        }
    }
}
