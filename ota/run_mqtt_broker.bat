@echo off
chcp 65001 >nul
echo [1/2] Starting MQTT Broker...
call "C:\Program Files\mosquitto\mosquitto.exe" -c "broker\mosquitto.conf" -v

echo [2/2] Ready to send OTA JSON file.
cd /d "%~dp0publisher"
cmd /k
