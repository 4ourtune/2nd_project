// sensor_thread만 실행할 때 main
// #include "shared.h"
// #include "vsomeip_manager.h"
// #include <iostream>
// #include <thread>
// #include <chrono>
// #include <csignal>

// using namespace std;

// // sensor_thread() 선언 (sensor_thread.cpp에서 정의됨)
// void sensor_thread();

// bool g_exit_flag = false;

// void signal_handler(int sig) {
//     cout << "\n[main] Caught signal " << sig << ", shutting down..." << endl;
//     g_exit_flag = true;
// }

// int main() {
//     cout << "===============================" << endl;
//     cout << "   Raspberry Pi SOME/IP Client " << endl;
//     cout << "   Sensor Receive Test Program " << endl;
//     cout << "===============================" << endl;

//     // Ctrl+C 핸들러 등록
//     signal(SIGINT, signal_handler);

//     // 센서 스레드 실행
//     thread t_sensor(sensor_thread);
//     cout << "[main] sensor_thread started" << endl;

//     // 메인 루프: 주기적으로 공유데이터 출력
//     while (!g_exit_flag) {
//         {
//             lock_guard<mutex> lock(g_shared.mtx);
//             cout << fixed;
//             cout << "[main] Lux=" << g_shared.sensor.ambient_lux
//                  << " | ToF=" << g_shared.sensor.front_tof_mm << " mm"
//                  << " | Ultra(mm): L=" << g_shared.sensor.left_ultra_mm
//                  << ", R=" << g_shared.sensor.right_ultra_mm
//                  << ", Rear=" << g_shared.sensor.rear_ultra_mm
//                  << endl;
//         }
//         this_thread::sleep_for(chrono::seconds(1));
//     }

//     // 종료 처리
//     g_shared.running = false;
//     if (t_sensor.joinable()) t_sensor.join();

//     cout << "[main] program exited cleanly" << endl;
//     return 0;
// }

// comm_thread만 실행할 때 main
// #include "shared.h"
// #include "vsomeip_manager.h"
// #include <iostream>
// #include <thread>
// #include <chrono>
// #include <csignal>

// using namespace std;

// // comm_thread() 선언
// void comm_thread();

// bool g_exit_flag = false;

// void signal_handler(int sig) {
//     cout << "\n[main] Caught signal " << sig << ", shutting down..." << endl;
//     g_exit_flag = true;
//     g_shared.running = false;
// }

// int main() {
//     cout << "===============================" << endl;
//     cout << "   Raspberry Pi SOME/IP Client " << endl;
//     cout << "   Control Thread Test Program " << endl;
//     cout << "===============================" << endl;

//     // Ctrl+C 핸들러 등록
//     signal(SIGINT, signal_handler);

//     // 1️. vsomeip 초기화
//     VSomeIPManager& someip = VSomeIPManager::getInstance();
//     if (!someip.init()) {
//         cerr << "[main] ERROR: vsomeip init failed" << endl;
//         return -1;
//     }
//     cout << "[main] vsomeip initialized successfully" << endl;

//     // 2️. 서비스 OFFER 대기
//     cout << "[main] Waiting for SOME/IP services (0x200, 0x300)..." << endl;
//     for (int i = 0; i < 50; ++i) { // 최대 5초 대기
//         if (someip.isServiceAvailable(SERVICE_ID_CONTROL) &&
//             someip.isServiceAvailable(SERVICE_ID_SYSTEM)) {
//             cout << "[main] All required services are AVAILABLE!" << endl;
//             break;
//         }
//         this_thread::sleep_for(chrono::milliseconds(100));
//     }

//     // OFFER 직후 안정화 대기 (vsomeip routing setup 대기)
//     std::this_thread::sleep_for(std::chrono::milliseconds(500));
//     cout << "[main] Waiting 0.5s for routing stabilization..." << endl;

//     // 3️. comm_thread 실행
//     thread t_comm(comm_thread);
//     cout << "[main] comm_thread started" << endl;

//     // 4️. 테스트 루프 (2초마다 상태 토글)
//     int cycle = 0;
//     while (!g_exit_flag) {
//         {
//             lock_guard<mutex> lock(g_shared.mtx);
//             if (cycle % 2 == 0) {
//                 g_shared.out.led_back_on = !g_shared.out.led_back_on;
//                 g_shared.out.led_front_down_on = !g_shared.out.led_front_down_on;
//                 g_shared.out.led_front_up_on = !g_shared.out.led_front_up_on;
//                 g_shared.out.buzzerOn = !g_shared.out.buzzerOn;
//                 g_shared.out.frequency = g_shared.out.buzzerOn ? 600 : 500;
//                 g_shared.out.alert_interval_ms = (g_shared.out.alert_interval_ms == -1) ? 500 : -1;
//                 g_shared.out.throttle = (g_shared.out.throttle == 0) ? 40 : 0;
//                 g_shared.out.steer = (g_shared.out.steer == 0) ? 10 : 0;
//             }
//         }

//         cycle++;
//         this_thread::sleep_for(chrono::seconds(2));
//     }

//     // 5️. 종료 처리
//     g_shared.running = false;
//     if (t_comm.joinable()) t_comm.join();

//     cout << "[main] program exited cleanly" << endl;
//     return 0;
// }

// sensor_thread랑 comm_thread 같이 실행할 때 main
// #include "shared.h"
// #include "vsomeip_manager.h"
// #include <iostream>
// #include <thread>
// #include <chrono>
// #include <csignal>

// using namespace std;

// // 스레드 선언
// void sensor_thread();
// void comm_thread();

// bool g_exit_flag = false;

// void signal_handler(int sig) {
//     cout << "\n[main] Caught signal " << sig << ", shutting down..." << endl;
//     g_exit_flag = true;
//     g_shared.running = false;
// }

// int main() {
//     cout << "===============================" << endl;
//     cout << "   Raspberry Pi SOME/IP Client " << endl;
//     cout << "   Sensor + Control Thread Run " << endl;
//     cout << "===============================" << endl;

//     // Ctrl+C 핸들러 등록
//     signal(SIGINT, signal_handler);

//     // 1️. vsomeip 초기화
//     VSomeIPManager& someip = VSomeIPManager::getInstance();
//     if (!someip.init()) {
//         cerr << "[main] ERROR: vsomeip init failed" << endl;
//         return -1;
//     }
//     cout << "[main] vsomeip initialized successfully" << endl;

//     // 2️. 서비스 OFFER 대기
//     cout << "[main] Waiting for SOME/IP services (0x100, 0x200, 0x300)..." << endl;
//     for (int i = 0; i < 50; ++i) { // 최대 5초 대기
//         if (someip.isServiceAvailable(SERVICE_ID_SENSOR) &&
//             someip.isServiceAvailable(SERVICE_ID_CONTROL) &&
//             someip.isServiceAvailable(SERVICE_ID_SYSTEM)) {
//             cout << "[main] All required services are AVAILABLE!" << endl;
//             break;
//         }
//         this_thread::sleep_for(chrono::milliseconds(100));
//     }

//     // OFFER 직후 안정화 대기
//     std::this_thread::sleep_for(std::chrono::milliseconds(500));
//     cout << "[main] Waiting 0.5s for routing stabilization..." << endl;

//     // 3️. 두 스레드 병렬 실행
//     thread t_sensor(sensor_thread);
//     thread t_comm(comm_thread);
//     cout << "[main] sensor_thread & comm_thread started" << endl;

//     // 4️. 모니터링 루프
//     int cycle = 0;
//     while (!g_exit_flag) {
//         {
//             lock_guard<mutex> lock(g_shared.mtx);
//             cout << fixed;
//             cout << "[main] Lux=" << g_shared.sensor.ambient_lux
//                  << " | ToF=" << g_shared.sensor.front_tof_mm << " mm"
//                  << " | Ultra(mm): L=" << g_shared.sensor.left_ultra_mm
//                  << ", R=" << g_shared.sensor.right_ultra_mm
//                  << ", Rear=" << g_shared.sensor.rear_ultra_mm
//                  << " | Throttle=" << g_shared.out.throttle
//                  << ", Steer=" << g_shared.out.steer
//                  << endl;
//         }

//         // 제어 명령을 2초마다 토글
//         if (cycle % 20 == 0) {
//             lock_guard<mutex> lock(g_shared.mtx);
//             g_shared.out.led_back_on = !g_shared.out.led_back_on;
//             g_shared.out.led_front_down_on = !g_shared.out.led_front_down_on;
//             g_shared.out.led_front_up_on = !g_shared.out.led_front_up_on;
//             g_shared.out.buzzerOn = !g_shared.out.buzzerOn;
//             g_shared.out.frequency = g_shared.out.buzzerOn ? 600 : 500;
//             g_shared.out.alert_interval_ms = (g_shared.out.alert_interval_ms == -1) ? 500 : -1;
//             g_shared.out.throttle = (g_shared.out.throttle == 0) ? 40 : 0;
//             g_shared.out.steer = (g_shared.out.steer == 0) ? 10 : 0;
//         }

//         cycle++;
//         this_thread::sleep_for(chrono::milliseconds(100));
//     }

//     // 5️. 종료 처리
//     g_shared.running = false;
//     if (t_sensor.joinable()) t_sensor.join();
//     if (t_comm.joinable()) t_comm.join();

//     cout << "[main] program exited cleanly" << endl;
//     return 0;
// }

//sensor_thread, control_thread, comm_thread 같이 쓸 때 main
#include "shared.h"
#include "vsomeip_manager.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>

using namespace std;

// 스레드 선언
void sensor_thread();
void control_thread();
void comm_thread();

bool g_exit_flag = false;

void signal_handler(int sig) {
    cout << "\n[main] Caught signal " << sig << ", shutting down..." << endl;
    g_exit_flag = true;
    g_shared.running = false;
}

int main() {
    cout << "==========================================" << endl;
    cout << "   Raspberry Pi SOME/IP Client Controller " << endl;
    cout << "   Sensor + Control + Comm Thread Program " << endl;
    cout << "==========================================" << endl;

    // Ctrl+C 핸들러 등록
    signal(SIGINT, signal_handler);

    // 1️. vsomeip 초기화
    VSomeIPManager& someip = VSomeIPManager::getInstance();
    if (!someip.init()) {
        cerr << "[main] ERROR: vsomeip init failed" << endl;
        return -1;
    }
    cout << "[main] vsomeip initialized successfully" << endl;

    // 2️. 서비스 OFFER 대기
    cout << "[main] Waiting for SOME/IP services (0x100, 0x200, 0x300)..." << endl;
    for (int i = 0; i < 50; ++i) { // 최대 5초 대기
        if (someip.isServiceAvailable(SERVICE_ID_SENSOR) &&
            someip.isServiceAvailable(SERVICE_ID_CONTROL) &&
            someip.isServiceAvailable(SERVICE_ID_SYSTEM)) {
            cout << "[main] All required services are AVAILABLE!" << endl;
            break;
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    // OFFER 직후 안정화 대기
    this_thread::sleep_for(chrono::milliseconds(500));
    cout << "[main] Waiting 0.5s for routing stabilization..." << endl;

    // 3️. 세 스레드 병렬 실행
    thread t_sensor(sensor_thread);
    thread t_control(control_thread);
    thread t_comm(comm_thread);
    cout << "[main] sensor_thread, control_thread, comm_thread started" << endl;

    // 4️. 모니터링 루프
    int cycle = 0;
    while (!g_exit_flag) {
        {
            lock_guard<mutex> lock(g_shared.mtx);
            cout << fixed;
            cout << "[main] Lux=" << g_shared.sensor.ambient_lux << endl;
                //  << " | ToF=" << g_shared.sensor.front_tof_mm << " mm"
                //  << " | Ultra(mm): L=" << g_shared.sensor.left_ultra_mm
                //  << ", R=" << g_shared.sensor.right_ultra_mm
                //  << ", Rear=" << g_shared.sensor.rear_ultra_mm
                //  << " | Throttle=" << g_shared.out.throttle
                //  << ", Steer=" << g_shared.out.steer
                //  << " | LED(B,Fd,Fu)=" << g_shared.out.led_back_on
                //  << "," << g_shared.out.led_front_down_on
                //  << "," << g_shared.out.led_front_up_on
                //  << " | Buzzer=" << (g_shared.out.buzzerOn ? "ON" : "OFF")
                 
        }

        cycle++;
        this_thread::sleep_for(chrono::milliseconds(200)); // 5Hz 주기 모니터링
    }

    // 5️. 종료 처리
    g_shared.running = false;
    if (t_sensor.joinable())  t_sensor.join();
    if (t_control.joinable()) t_control.join();
    if (t_comm.joinable())    t_comm.join();

    cout << "[main] program exited cleanly" << endl;
    return 0;
}
