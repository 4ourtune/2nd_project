#!/bin/bash
LOG_FILE="/home/hj/2nd_project/vc_software/apps/test/test_app.log"

echo "[test_app_v3] 실행됨 - $(date)" >> "$LOG_FILE"

while true; do
    echo "[test_app_v3] heartbeat - $(date)" >> "$LOG_FILE"
    sleep 10
done
