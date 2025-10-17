#include "serialize_data.h"

#include <string.h>
#include <lwip/inet.h>

/* ToF 데이터를 txBuf에 직렬화 */
/* 반환값: 최종 버퍼 길이 (txLen) */
uint16_t Serialize_ToFData (uint8_t *txBuf, uint16_t txLen, const ToFData_t *data)
{
    // 1) id
    txBuf[txLen++] = data->id;

    // 2) system_time_ms (uint32_t → big-endian)
    uint32_t t = htonl(data->system_time_ms);
    memcpy(&txBuf[txLen], &t, 4);
    txLen += 4;

    // 3) distance_m (float → uint32_t 비트 단위로 변환 후 big-endian)
    uint32_t f;
    memcpy(&f, &data->distance_m, 4);
    f = htonl(f);
    memcpy(&txBuf[txLen], &f, 4);
    txLen += 4;

    // 4) distance_status
    txBuf[txLen++] = data->distance_status;

    // 5) signal_strength (uint16_t → big-endian)
    uint16_t s = htons(data->signal_strength);
    memcpy(&txBuf[txLen], &s, 2);
    txLen += 2;

    // 6) received_time_us (uint64_t → 상/하위 32비트 분리 후 big-endian)
    uint64_t val = data->received_time_us;
    uint32_t high = htonl((uint32_t ) (val >> 32));
    uint32_t low = htonl((uint32_t ) (val & 0xFFFFFFFF));
    memcpy(&txBuf[txLen], &high, 4);
    memcpy(&txBuf[txLen + 4], &low, 4);
    txLen += 8;

    return txLen;
}

uint16_t Serialize_UltrasonicData (uint8_t *txBuf, uint16_t txLen, const UltrasonicData_t *data)
{
    // 1) dist_raw_mm (int32_t → big-endian)
    int32_t raw = htonl(data->dist_raw_mm);
    memcpy(&txBuf[txLen], &raw, 4);
    txLen += 4;

    // 2) dist_filt_mm (int32_t → big-endian)
    int32_t filt = htonl(data->dist_filt_mm);
    memcpy(&txBuf[txLen], &filt, 4);
    txLen += 4;

    // 3) received_time_us (uint64_t → 상/하위 32비트 나눠서 big-endian)
    uint64_t val = data->received_time_us;
    uint32_t high = htonl((uint32_t ) (val >> 32));
    uint32_t low = htonl((uint32_t ) (val & 0xFFFFFFFF));
    memcpy(&txBuf[txLen], &high, 4);
    memcpy(&txBuf[txLen + 4], &low, 4);
    txLen += 8;

    return txLen;
}

uint16_t Serialize_MotorControllerData (uint8_t *txBuf, uint16_t txLen, const MotorControllerData_t *data)
{
    // 1) motorChA_speed (int32_t → Big Endian)
    int32_t a = htonl(data->motorChA_speed);
    memcpy(&txBuf[txLen], &a, 4);
    txLen += 4;

    // 2) motorChB_speed (int32_t → Big Endian)
    int32_t b = htonl(data->motorChB_speed);
    memcpy(&txBuf[txLen], &b, 4);
    txLen += 4;

    // 3) output_time_us (uint64_t → 상/하위 32비트로 나눠서 Big Endian)
    uint64_t val = data->output_time_us;
    uint32_t high = htonl((uint32_t )(val >> 32));
    uint32_t low = htonl((uint32_t )(val & 0xFFFFFFFF));
    memcpy(&txBuf[txLen], &high, 4);
    memcpy(&txBuf[txLen + 4], &low, 4);
    txLen += 8;

    return txLen;
}

uint16_t Serialize_EmerAlertData (uint8_t *txBuf, uint16_t txLen, const EmerAlertData_t *data)
{
    // 1) interval_ms (int64_t → Big Endian)
    int64_t interval = data->interval_ms;
    uint32_t interval_high = htonl((uint32_t )(interval >> 32));
    uint32_t interval_low = htonl((uint32_t )(interval & 0xFFFFFFFF));
    memcpy(&txBuf[txLen], &interval_high, 4);
    memcpy(&txBuf[txLen + 4], &interval_low, 4);
    txLen += 8;

    // 2) output_time_us (uint64_t → Big Endian)
    uint64_t time = data->output_time_us;
    uint32_t time_high = htonl((uint32_t )(time >> 32));
    uint32_t time_low = htonl((uint32_t )(time & 0xFFFFFFFF));
    memcpy(&txBuf[txLen], &time_high, 4);
    memcpy(&txBuf[txLen + 4], &time_low, 4);
    txLen += 8;

    return txLen;
}
