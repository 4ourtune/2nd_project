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

        int lux = (phase<10) ? 200 : 5; // 어두워짐 → HBA 점등

        {
            std::lock_guard<std::mutex> lk(g_shared.mtx);
            g_shared.sensor.dist_cm   = dist;
            g_shared.sensor.ambient_lux = lux;
            g_shared.sensor.ts_ms = t;
        }

        sleep_ms(PERIOD_SENSOR_MS);
    }
}
