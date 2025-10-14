# publisher/config.py
# ====================
# OTA Publisher 설정

# MQTT Broker 정보
BROKER_HOST = "192.168.45.166"   # 로컬 PC (broker IP)
BROKER_PORT = 1883                # mosquitto 포트

# 기본 OTA 전송 토픽
TOPIC = "ota/vehicle_control/update"

# 로컬 OTA 파일 경로
BASE_DIR = __file__
