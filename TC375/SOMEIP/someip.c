#include "someip.h"
#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/udp.h"
#include "Configuration.h"
#include "Ifx_Lwip.h"

#include <string.h>

#include "etc.h"

#include "my_stdio.h"
#include "tof.h"
#include "ultrasonic.h"

#include "emer_alert.h"
#include "motor_controller.h"

#include "output_status.h"

#if LWIP_UDP

struct udp_pcb *g_SOMEIP_SERVICE1_PCB;
struct udp_pcb *g_SOMEIP_SERVICE2_PCB;

void SOMEIP_Service1_Callback (void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, uint16 port)
{
    static ToFData_t tof_latest_data;
    static UltrasonicData_t ult_latest_data[ULTRASONIC_COUNT];

    uint16 ServiceID = 0;
    uint16 MethodID = 0;
    uint8 MessageType = 0;

    if (p != NULL)
    {
        uint8 rxBuf[p->len];
        memcpy(rxBuf, p->payload, p->len);
        ServiceID = (rxBuf[0] << 8) + rxBuf[1];
        MethodID = (rxBuf[2] << 8) + rxBuf[3];

        if (ServiceID != 0x0100U)
        {
            my_printf("Requested Unknown Service ID\n");
        }
        else
        {
            /* Message Type: Request */
            MessageType = rxBuf[14];
            if (MessageType == 0x00)
            {
                /* Check Service ID & Method ID */
                if (ServiceID == 0x0100U)
                {
                    if (MethodID == 0x0101U || MethodID == 0x0102U)
                    {
                        uint8 txBuf[256];  // 충분한 크기의 전송 버퍼
                        uint16 txLen = 0;

                        // SOME/IP 헤더 복사 (16 bytes)
                        memcpy(txBuf, rxBuf, 16);

                        // Message Type을 Response로 변경
                        txBuf[14] = 0x80;

                        if (MethodID == 0x0101U)
                        {
                            // ToF 센서 데이터 가져오기
                            ToF_GetLatestData(&tof_latest_data);

                            // Payload에 ToF 데이터 추가
                            txLen = 16;  // SOME/IP 헤더 크기

                            // ToF 데이터 구조체를 바이트 배열로 복사
                            memcpy(&txBuf[txLen], &tof_latest_data, sizeof(ToFData_t));
                            txLen += sizeof(ToFData_t);
                        }
                        else if (MethodID == 0x0102U)
                        {
                            // 초음파 센서 데이터 가져오기
                            for (int i = 0; i < ULTRASONIC_COUNT; i++)
                            {
                                Ultrasonic_GetLatestData(i, &ult_latest_data[i]);
                            }

                            // Payload에 초음파 센서 데이터 추가
                            txLen = 16;  // SOME/IP 헤더 크기

                            // 초음파 센서 개수 추가 (옵션)
                            txBuf[txLen++] = ULTRASONIC_COUNT;

                            // 모든 초음파 센서 데이터를 바이트 배열로 복사
                            for (int i = 0; i < ULTRASONIC_COUNT; i++)
                            {
                                memcpy(&txBuf[txLen], &ult_latest_data[i], sizeof(UltrasonicData_t));
                                txLen += sizeof(UltrasonicData_t);
                            }
                        }

                        // SOME/IP Length 필드 업데이트 (Byte 4-7)
                        // Length = Payload 크기 + 8 (Message ID부터 Payload까지)
                        uint32 length = txLen - 8;
                        txBuf[4] = (length >> 24) & 0xFF;
                        txBuf[5] = (length >> 16) & 0xFF;
                        txBuf[6] = (length >> 8) & 0xFF;
                        txBuf[7] = length & 0xFF;

                        /* Send Response Message */
                        err_t err;
                        struct pbuf *txbuf = pbuf_alloc(PBUF_TRANSPORT, txLen, PBUF_RAM);
                        if (txbuf != NULL)
                        {
                            pbuf_take(txbuf, txBuf, txLen);

                            ip_addr_t destination_ip;
                            unsigned char a = (unsigned char) (addr->addr);
                            unsigned char b = (unsigned char) (addr->addr >> 8);
                            unsigned char c = (unsigned char) (addr->addr >> 16);
                            unsigned char d = (unsigned char) (addr->addr >> 24);

                            IP4_ADDR(&destination_ip, a, b, c, d);
                            u16_t destination_port = port;  // 요청자의 포트로 전송

                            err = udp_sendto(upcb, txbuf, &destination_ip, destination_port);
                            if (err == ERR_OK)
                            {
                                my_printf("Send SOME/IP Service Response with sensor data!! (MethodID: %x)\n",
                                        MethodID);
                            }
                            else
                            {
                                my_printf("Send SOME/IP Service Response Failed!! \n");
                            }

                            pbuf_free(txbuf);
                        }
                        else
                        {
                            my_printf("Failed to allocate memory for UDP packet buffer.\n");
                        }
                    }
                }
            }
        }
        pbuf_free(p);
    }
}

void SOMEIP_Service2_Callback (void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, uint16 port)
{
    static MotorControllerData_t motor_controller_latest_data;
    static EmerAlertData_t emerAlert_latest_data;

    uint16 ServiceID = 0;
    uint16 MethodID = 0;
    uint8 MessageType = 0;

    if (p != NULL)
    {
        uint8 rxBuf[p->len];
        memcpy(rxBuf, p->payload, p->len);
        ServiceID = (rxBuf[0] << 8) + rxBuf[1];
        MethodID = (rxBuf[2] << 8) + rxBuf[3];

        if (ServiceID != 0x0200U)
        {
            my_printf("Requested Unknown Service ID\n");
        }
        else
        {
            /* Message Type: Request */
            MessageType = rxBuf[14];
            if (MessageType == 0x00)
            {
                /* Check Service ID & Method ID */
                if (ServiceID == 0x0200U)
                {
                    if (MethodID == 0x0201U || MethodID == 0x0202U)
                    {
                        uint8 txBuf[256];  // 충분한 크기의 전송 버퍼
                        uint16 txLen = 0;

                        // SOME/IP 헤더 복사 (16 bytes)
                        memcpy(txBuf, rxBuf, 16);

                        // Message Type을 Response로 변경
                        txBuf[14] = 0x80;

                        if (MethodID == 0x0201U)
                        {
                            // Motor Control
                            // Payload에서 motor_x, motor_y 값 추출
                            int motor_x, motor_y;

                            // motor_x 추출 (4 bytes, signed int)
                            motor_x = (rxBuf[16] << 24) | (rxBuf[17] << 16) | (rxBuf[18] << 8) | rxBuf[19];

                            // motor_y 추출 (4 bytes, signed int)
                            motor_y = (rxBuf[20] << 24) | (rxBuf[21] << 16) | (rxBuf[22] << 8) | rxBuf[23];

                            // 모터 제어 수행
                            if (MotorController_ProcessJoystickInput(motor_x, motor_y))
                            {
                                // 모터 컨트롤러 최신 데이터 가져오기
                                MotorController_GetLatestData(&motor_controller_latest_data);
                            }

                            // Response payload 구성
                            txLen = 16;  // SOME/IP 헤더 크기

                            // 모터 컨트롤러 데이터 추가
                            memcpy(&txBuf[txLen], &motor_controller_latest_data, sizeof(MotorControllerData_t));
                            txLen += sizeof(MotorControllerData_t);

                            my_printf("Motor Control: x=%d, y=%d\n", motor_x, motor_y);
                        }
                        else if (MethodID == 0x0202U)
                        {
                            // Emergency Alert Control
                            // Payload에서 emerAlert_cycle_ms 값 추출
                            int64_t emerAlert_cycle_ms;

                            // emerAlert_cycle_ms 추출 (8 bytes, signed int64)
                            emerAlert_cycle_ms = ((int64_t) rxBuf[16] << 56) | ((int64_t) rxBuf[17] << 48)
                                    | ((int64_t) rxBuf[18] << 40) | ((int64_t) rxBuf[19] << 32)
                                    | ((int64_t) rxBuf[20] << 24) | ((int64_t) rxBuf[21] << 16)
                                    | ((int64_t) rxBuf[22] << 8) | (int64_t) rxBuf[23];

                            // Emergency Alert 업데이트 수행
                            if (EmerAlert_Set_Interval(emerAlert_cycle_ms))
                            {
                                // Emergency Alert 최신 데이터 가져오기
                                EmerAlert_GetLatestData(&emerAlert_latest_data);
                            }

                            // Response payload 구성
                            txLen = 16;  // SOME/IP 헤더 크기

                            // Emergency Alert 데이터 추가
                            memcpy(&txBuf[txLen], &emerAlert_latest_data, sizeof(EmerAlertData_t));
                            txLen += sizeof(EmerAlertData_t);

                            my_printf("Emergency Alert Control: cycle=%lld ms\n", emerAlert_cycle_ms);
                        }

                        // SOME/IP Length 필드 업데이트 (Byte 4-7)
                        // Length = Payload 크기 + 8 (Message ID부터 Payload까지)
                        uint32 length = txLen - 8;
                        txBuf[4] = (length >> 24) & 0xFF;
                        txBuf[5] = (length >> 16) & 0xFF;
                        txBuf[6] = (length >> 8) & 0xFF;
                        txBuf[7] = length & 0xFF;

                        /* Send Response Message */
                        err_t err;
                        struct pbuf *txbuf = pbuf_alloc(PBUF_TRANSPORT, txLen, PBUF_RAM);
                        if (txbuf != NULL)
                        {
                            pbuf_take(txbuf, txBuf, txLen);

                            ip_addr_t destination_ip;
                            unsigned char a = (unsigned char) (addr->addr);
                            unsigned char b = (unsigned char) (addr->addr >> 8);
                            unsigned char c = (unsigned char) (addr->addr >> 16);
                            unsigned char d = (unsigned char) (addr->addr >> 24);

                            IP4_ADDR(&destination_ip, a, b, c, d);
                            u16_t destination_port = port;  // 요청자의 포트로 전송

                            err = udp_sendto(upcb, txbuf, &destination_ip, destination_port);
                            if (err == ERR_OK)
                            {
                                my_printf("Send SOME/IP Service2 Response!! (MethodID: %x)\n", MethodID);
                            }
                            else
                            {
                                my_printf("Send SOME/IP Service2 Response Failed!! \n");
                            }

                            pbuf_free(txbuf);
                        }
                        else
                        {
                            my_printf("Failed to allocate memory for UDP packet buffer.\n");
                        }
                    }
                }
            }
        }
        pbuf_free(p);
    }
}

bool SOMEIP_UPCB_Init (struct udp_pcb *g_SOMEIP_SERVICE_PCB, uint16 port, udp_recv_fn recv)
{
    g_SOMEIP_SERVICE_PCB = udp_new();
    if (g_SOMEIP_SERVICE_PCB)
    {
        /* bind pcb to the port */
        /* Using IP_ADDR_ANY allow the pcb to be used by any local interface */
        err_t err = udp_bind(g_SOMEIP_SERVICE_PCB, IP_ADDR_ANY, port);
        if (err == ERR_OK)
        {
            /* Set a receive callback for the pcb */
            udp_recv(g_SOMEIP_SERVICE_PCB, recv, NULL);
            my_printf("SOME/IP Service PCB Initialized! (Port: %d)\n", port);
        }
        else
        {
            udp_remove(g_SOMEIP_SERVICE_PCB);
            my_printf("SOME/IP Service PCB init failed!\n");
            return false;
        }
    }
    else
    {
        my_printf("Failed to create UDP PCB\n");
        return false;
    }

    return true;
}

bool SOMEIP_Init (void)
{
    if (!SOMEIP_UPCB_Init(g_SOMEIP_SERVICE1_PCB, PN_SERVICE_1, (void*) SOMEIP_Service1_Callback))
        return false;
    if (!SOMEIP_UPCB_Init(g_SOMEIP_SERVICE2_PCB, PN_SERVICE_2, (void*) SOMEIP_Service2_Callback))
        return false;

    return true;
}

#endif /* LWIP_UDP */
