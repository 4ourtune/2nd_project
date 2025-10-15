@echo off
chcp 65001 >nul
title OTA MQTT Broker + HTTP Server

cd /d %~dp0

echo ==========================================
echo        OTA MQTT Broker + HTTP Server
echo ==========================================
echo.

:: 1) MQTT 브로커 실행
echo [1/2] MQTT 브로커를 실행합니다...
start "Mosquitto Broker" cmd /k "mosquitto.exe -c broker\mosquitto.conf -v"
timeout /t 3 >nul

:: 2) HTTP 서버 실행 (ota/publisher/build 폴더 기준)
echo [2/2] HTTP 서버를 실행합니다 (포트 8000)...
cd publisher\build
start "" cmd /k "python -m http.server 8000"
cd ../..

echo.
echo ==========================================
echo   모든 서버가 정상적으로 실행되었습니다.
echo   MQTT Broker 포트 : 1883
echo   HTTP Server 포트 : 8000
echo   (파일 경로: ota/publisher/build)
echo ==========================================
echo.
pause
