@echo off
title OTA Local Server
color 0A

REM ===========================================================
REM 1. MQTT Broker 실행
REM ===========================================================
echo [INFO] Starting Mosquitto MQTT Broker...
start "MQTT Broker" cmd /k "cd /d C:\Program Files\mosquitto && mosquitto -c mosquitto.conf -v"

REM 브로커가 먼저 안정적으로 뜨도록 약간 대기
timeout /t 3 >nul

REM ===========================================================
REM 2. OTA File Watcher 실행
REM ===========================================================
echo [INFO] Starting Python OTA File Watcher...
cd /d %~dp0
start "OTA Watcher" cmd /k "python watcher\file_watcher.py"

REM ===========================================================
REM 3. 완료 메시지
REM ===========================================================
echo.
echo [SUCCESS] OTA Server started successfully!
echo [INFO] Mosquitto Broker and OTA Watcher are now running in separate windows.
echo.
pause
