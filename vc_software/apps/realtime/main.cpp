#include <iostream>
#include <csignal>
#include <thread>
#include "shared.h"
#include "vc_common.h"
#include "config.h"

SharedData g_shared;

void joystick_thread();
void sensor_thread();
void control_thread();
void comm_thread();

static void sigint_handler(int){ g_shared.running = false; }

int main(){
    std::signal(SIGINT, sigint_handler);

    std::thread t_joy(joystick_thread);
    std::thread t_sensor(sensor_thread);
    std::thread t_ctrl(control_thread);
    std::thread t_comm(comm_thread);

    uint64_t last_log = 0;
    while(g_shared.running){
        uint64_t t = now_ms();
        if (t - last_log >= PERIOD_LOG_MS) {
            std::lock_guard<std::mutex> lk(g_shared.mtx);
            std::cout << "[STAT] eng=" << g_shared.engine_on
                      << " joy(" << g_shared.joy.x << "," << g_shared.joy.y << ")"
                      << " dist=" << g_shared.sensor.dist_cm
                      << " lux=" << g_shared.sensor.ambient_lux
                      << " out(thr=" << g_shared.out.throttle
                      << ",str=" << g_shared.out.steer
                      << ",lamp=" << g_shared.out.headlamp_on
                      << ",aeb=" << g_shared.out.aeb_brake << ")\n";
            last_log = t;
        }
        sleep_ms(50);
    }

    t_joy.join();
    t_sensor.join();
    t_ctrl.join();
    t_comm.join();
    return 0;
}

