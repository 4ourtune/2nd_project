#include "shared.h"
#include "vc_common.h"
#include "config.h"
#include <iostream>
#include "vsomeip_manager.h"

void comm_thread(){
    set_realtime_sched(PRIO_COMM);

    VSomeIPManager& someip = VSomeIPManager::getInstance();

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
			static ControlOutput preOut;

			if (out.buzzerOn != preOut.buzzerOn || out.frequency != preOut.frequency) {
				someip.requestBuzzerControl(preOut.buzzerOn, out.frequency);
				preOut.buzzerOn = out.buzzerOn;
				preOut.frequency = out.frequency;
                std::cout << "Request buzzer control\n";
			}

            {
                if (out.led_back_on != preOut.led_back_on) {
                    someip.requestLedControl(0, out.led_back_on);
                    preOut.led_back_on = out.led_back_on;
                    std::cout << "Request led_back control\n";
                }
                
                if (out.led_front_down_on != preOut.led_front_down_on) {
                    someip.requestLedControl(1, out.led_front_down_on);
                    preOut.led_front_down_on = out.led_front_down_on;
                    std::cout << "Request led_front_down control\n";
                }
                
                if (out.led_front_up_on != preOut.led_front_up_on) {
                    someip.requestLedControl(2, out.led_front_up_on);
                    preOut.led_front_up_on = out.led_front_up_on;
                    std::cout << "Request led_front_up control\n";
                }
            }

			if (out.interval_ms != preOut.interval_ms) {
				someip.requestAlertControl(out.interval_ms);
				preOut.interval_ms = out.interval_ms;
                std::cout << "Request emerAlert control\n";
			}

			if (out.throttle != preOut.throttle || out.steer != preOut.steer) {
				someip.requestMotorControl(out.throttle, out.steer);
				preOut.throttle = out.throttle;
				preOut.steer = out.steer;
                std::cout << "Request motor control\n";
			}
        }
    }
}
