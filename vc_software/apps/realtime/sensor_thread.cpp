#include "shared.h"
#include "vc_common.h"
#include "config.h"
#include <cmath>

void sensor_thread(){
    set_realtime_sched(PRIO_SENSOR);

    while(g_shared.running){
        // Stub: 거리/조도에 단순 파형을 줘서 AEB/HBA 반응 유도
        uint64_t t = now_ms();
        int phase = (t/1000)%20;

        int dist = 100; // cm
        if (phase >= 5 && phase < 8) dist = 15;   // 잠깐 근접 → AEB 트리거
        if (phase >= 12 && phase < 16) dist = 30;

        // 간단한 자율주차용 센서 스텁(추후 실측치로 대체)
        int left_ultra_mm = (phase >= 3 && phase < 7) ? 800 : 250;   // 공간 탐색 시 좌측 여유 거리 증가
        int right_ultra_mm = 220;                                    // 벽과 일정 거리 유지
        int rear_ultra_mm = (phase >= 12 && phase < 16) ? 120 : 600; // 후진 시 안전거리 감소

        int lux = (phase<10) ? 200 : 5; // 어두워짐 → HBA 점등

        {
            std::lock_guard<std::mutex> lk(g_shared.mtx);
            g_shared.sensor.dist_cm   = dist;
            g_shared.sensor.ambient_lux = lux;
            g_shared.sensor.ts_ms = t;
            g_shared.sensor.front_tof_mm = dist * 10;
            g_shared.sensor.left_ultra_mm = left_ultra_mm;
            g_shared.sensor.right_ultra_mm = right_ultra_mm;
            g_shared.sensor.rear_ultra_mm = rear_ultra_mm;
            uint64_t t_us = t * 1000;
            g_shared.sensor.front_ts_us = t_us;
            g_shared.sensor.left_ts_us = t_us;
            g_shared.sensor.right_ts_us = t_us;
            g_shared.sensor.rear_ts_us = t_us;
        }

        sleep_ms(PERIOD_SENSOR_MS);
    }
}
