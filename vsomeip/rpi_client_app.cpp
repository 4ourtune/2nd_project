#include <vsomeip/vsomeip.hpp>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <algorithm> // for std::reverse (Big Endian <-> Host Endian)

#define SERVICE_ID_SENSOR   0x0100
#define METHOD_ID_TOF       0x0101
#define METHOD_ID_ULT       0x0102

#define SERVICE_ID_CONTROL  0x0200
#define METHOD_ID_MOTOR     0x0201
#define METHOD_ID_ALERT     0x0202

#define INSTANCE_ID         0x0001
#define CLIENT_ID           0x5678

typedef struct
{
    uint8_t id;
    uint32_t system_time_ms;
    float distance_m;
    uint8_t distance_status;
    uint16_t signal_strength;
    uint64_t received_time_us;
} ToFData_t;

typedef struct
{
    int32_t dist_raw_mm;
    int32_t dist_filt_mm;
    uint64_t received_time_us;
} UltrasonicData_t;

typedef struct
{
    int32_t motorChA_speed;
    int32_t motorChB_speed;
    uint64_t output_time_us;
} MotorControllerData_t;

typedef struct
{
    int64_t interval_ms;
    uint64_t output_time_us;
} EmerAlertData_t;

std::shared_ptr<vsomeip::application> app;

/* 서비스 가용성 콜백 */
void on_availability(vsomeip::service_t service,
                     vsomeip::instance_t instance,
                     bool is_available) {
    std::cout << "Service [0x"
              << std::setw(4) << std::setfill('0') << std::hex << service
              << ".0x" << std::setw(4) << instance
              << "] is " << (is_available ? "AVAILABLE" : "NOT available")
              << std::endl;

    if (is_available) {
        std::cout << "Requesting service 0x" << std::hex << service
                  << " instance 0x" << instance << "..." << std::endl;
        app->request_service(service, instance);
    }
}

// 네트워크 바이트(Big Endian)를 호스트 바이트로 변환하는 템플릿 함수
// vsomeip의 데이터는 이미 바이트 배열 형태이므로, 수동으로 처리
template <typename T>
T get_value_from_payload(const vsomeip::byte_t* data, size_t offset) {
    T value;
    // 데이터를 복사한 후, Big Endian -> Host Endian 변환 로직 (if needed)
    // vsomeip는 항상 Big Endian으로 송수신하므로, 플랫폼이 Little Endian이면 변환 필요
    std::memcpy(&value, data + offset, sizeof(T));

    // 일반적인 x86/ARM(Little Endian) 환경이라면 다음 코드 사용
    if (sizeof(T) > 1) {
        vsomeip::byte_t* byte_ptr = reinterpret_cast<vsomeip::byte_t*>(&value);
        std::reverse(byte_ptr, byte_ptr + sizeof(T));
    }
    return value;
}

// 응답 메시지 처리 콜백 함수
void on_message(const std::shared_ptr<vsomeip::message>& response_msg) {
    vsomeip::service_t service_id = response_msg->get_service();
    vsomeip::method_t method_id = response_msg->get_method();
    std::shared_ptr<vsomeip::payload> payload = response_msg->get_payload();
    const vsomeip::byte_t* data = payload->get_data();
    size_t length = payload->get_length();

    std::cout << "\n--- Received Response for Service 0x" << std::hex << service_id
        << ", Method 0x" << method_id << std::dec << " (Length: " << length << " bytes) ---" << std::endl;

    if (service_id == SERVICE_ID_SENSOR) {
        if (method_id == METHOD_ID_TOF) {
            // 응답 페이로드: ToFData_t (20 bytes)
            size_t offset = 0;
            uint8_t id = get_value_from_payload<uint8_t>(data, offset); offset += 1;
            uint32_t time = get_value_from_payload<uint32_t>(data, offset); offset += 4;
            float distance = get_value_from_payload<float>(data, offset); offset += 4;
            uint8_t status = get_value_from_payload<uint8_t>(data, offset); offset += 1;
            uint16_t strength = get_value_from_payload<uint16_t>(data, offset); offset += 2;
            uint64_t received_time = get_value_from_payload<uint64_t>(data, offset); offset += 8;

            std::cout << "  [ToF Data] ID: " << (int)id << ", Dist: " << distance << " m, Time: " << received_time << " us" << std::endl;

        }
        else if (method_id == METHOD_ID_ULT) {
            // 응답 페이로드: 1 byte Count + N * UltrasonicData_t
            if (length > 0) {
                uint8_t count = get_value_from_payload<uint8_t>(data, 0);
                std::cout << "  [Ultrasonic] Sensor Count: " << (int)count << std::endl;

                size_t offset = 1; // 1 byte for count
                for (int i = 0; i < count; ++i) {
                    if (offset + sizeof(UltrasonicData_t) <= length) {
                        int32_t raw = get_value_from_payload<int32_t>(data, offset); offset += 4;
                        int32_t filt = get_value_from_payload<int32_t>(data, offset); offset += 4;
                        uint64_t received_time = get_value_from_payload<uint64_t>(data, offset); offset += 8;

                        std::cout << "    Sensor " << i << " Raw/Filt: " << raw << " / " << filt << " mm" << std::endl;
                    }
                    else {
                        break;
                    }
                }
            }
        }
    }
    else if (service_id == SERVICE_ID_CONTROL) {
        if (method_id == METHOD_ID_MOTOR && length >= sizeof(MotorControllerData_t)) {
            // 응답 페이로드: MotorControllerData_t (16 bytes)
            size_t offset = 0;
            int32_t speed_A = get_value_from_payload<int32_t>(data, offset); offset += 4;
            int32_t speed_B = get_value_from_payload<int32_t>(data, offset); offset += 4;
            uint64_t output_time = get_value_from_payload<uint64_t>(data, offset); offset += 8;

            std::cout << "  [Motor Status] Speed A/B: " << speed_A << " / " << speed_B << ", Time: " << output_time << " us" << std::endl;

        }
        else if (method_id == METHOD_ID_ALERT && length >= sizeof(EmerAlertData_t)) {
            // 응답 페이로드: EmerAlertData_t (16 bytes)
            size_t offset = 0;
            int64_t interval = get_value_from_payload<int64_t>(data, offset); offset += 8;
            uint64_t output_time = get_value_from_payload<uint64_t>(data, offset); offset += 8;

            std::cout << "  [Alert Status] Interval: " << interval << " ms, Time: " << output_time << " us" << std::endl;
        }
    }
}

/* 네트워크 바이트(Big Endian)로 값을 페이로드에 쓰는 템플릿 함수 */
template <typename T>
void append_to_payload(std::vector<vsomeip::byte_t>& payload_data, T value) {
    // Little Endian -> Big Endian 변환
    if (sizeof(T) > 1) {
        vsomeip::byte_t* byte_ptr = reinterpret_cast<vsomeip::byte_t*>(&value);
        std::reverse(byte_ptr, byte_ptr + sizeof(T));
    }

    const vsomeip::byte_t* data_ptr = reinterpret_cast<const vsomeip::byte_t*>(&value);
    for (size_t i = 0; i < sizeof(T); ++i) {
        payload_data.push_back(data_ptr[i]);
    }
}

void request_sensor_data(vsomeip::service_t service_id, vsomeip::method_t method_id) {
    std::shared_ptr<vsomeip::message> request = vsomeip::runtime::get()->create_request();

    request->set_service(service_id);
    request->set_instance(INSTANCE_ID);
    request->set_method(method_id);
    request->set_client(CLIENT_ID);
    request->set_interface_version(0x01);
    request->set_message_type(vsomeip::message_type_e::MT_REQUEST);

    std::shared_ptr<vsomeip::payload> its_payload = vsomeip::runtime::get()->create_payload();
    // Payload is empty for sensor data request
    request->set_payload(its_payload);

    std::cout << "\nSending Request for Service 0x" << std::hex << service_id
        << ", Method 0x" << method_id << std::dec << " (No Payload)" << std::endl;
    app->send(request);
}

void request_motor_control(int32_t motor_x, int32_t motor_y) {
    std::shared_ptr<vsomeip::message> request = vsomeip::runtime::get()->create_request();

    request->set_service(SERVICE_ID_CONTROL);
    request->set_instance(INSTANCE_ID);
    request->set_method(METHOD_ID_MOTOR);
    request->set_client(CLIENT_ID);
    request->set_interface_version(0x01);
    request->set_message_type(vsomeip::message_type_e::MT_REQUEST);

    std::vector<vsomeip::byte_t> payload_data;

    append_to_payload(payload_data, motor_x);
    append_to_payload(payload_data, motor_y);

    std::shared_ptr<vsomeip::payload> its_payload = vsomeip::runtime::get()->create_payload();
    its_payload->set_data(payload_data);
    request->set_payload(its_payload);

    std::cout << "\nSending Motor Control Request (X=" << motor_x << ", Y=" << motor_y << ")" << std::endl;
    app->send(request);
}

void request_alert_control(int64_t cycle_ms) {
    std::shared_ptr<vsomeip::message> request = vsomeip::runtime::get()->create_request();

    request->set_service(SERVICE_ID_CONTROL);
    request->set_instance(INSTANCE_ID);
    request->set_method(METHOD_ID_ALERT);
    request->set_client(CLIENT_ID);
    request->set_interface_version(0x01);
    request->set_message_type(vsomeip::message_type_e::MT_REQUEST);

    std::vector<vsomeip::byte_t> payload_data;

    append_to_payload(payload_data, cycle_ms);

    std::shared_ptr<vsomeip::payload> its_payload = vsomeip::runtime::get()->create_payload();
    its_payload->set_data(payload_data);
    request->set_payload(its_payload);

    std::cout << "\nSending Emergency Alert Control Request (Cycle: " << cycle_ms << " ms)" << std::endl;
    app->send(request);
}

// 요청 로직을 별도의 스레드에서 실행
void client_routine() {
    using namespace std::chrono;
    
    std::cout << "Waiting 4 second for vsomeip stack to stabilize..." << std::endl;
    std::this_thread::sleep_for(seconds(4));

    // --- 1. Sensor Service (0x0100) 요청 ---
    request_sensor_data(SERVICE_ID_SENSOR, METHOD_ID_TOF); // ToF Data
    std::this_thread::sleep_for(seconds(2));

    request_sensor_data(SERVICE_ID_SENSOR, METHOD_ID_ULT); // Ultrasonic Data
    std::this_thread::sleep_for(seconds(2));

    // --- 2. Control Service (0x0200) 요청 ---
    request_motor_control(50, 80); // Motor X=50, Y=80
    std::this_thread::sleep_for(seconds(2));

    request_alert_control(300); // Emergency Alert Cycle 300 ms
    std::this_thread::sleep_for(seconds(2));

    // 요청이 끝났다면 앱을 안전하게 종료합니다.
    std::cout << "\nClient routine finished. Stopping app in 1 second..." << std::endl;
    std::this_thread::sleep_for(seconds(1));
    app->stop();
}

int main() {
    app = vsomeip::runtime::get()->create_application("rpi_client_app");
    app->init();

    app->register_availability_handler(SERVICE_ID_SENSOR, INSTANCE_ID, on_availability);
    app->register_availability_handler(SERVICE_ID_CONTROL, INSTANCE_ID, on_availability);
    app->register_message_handler(vsomeip::ANY_SERVICE, INSTANCE_ID, vsomeip::ANY_METHOD, on_message); // 메시지 수신 콜백 등록 (ANY_SERVICE로 두 서비스 ID 모두 처리)
    
    app->request_service(SERVICE_ID_SENSOR, INSTANCE_ID);
    app->request_service(SERVICE_ID_CONTROL, INSTANCE_ID);

    // 요청 로직을 별도의 스레드에서 실행
    std::thread worker(client_routine);

    app->start();

    // worker 스레드가 app->stop()을 호출할 때까지 대기
    worker.join();

    return 0;
}

