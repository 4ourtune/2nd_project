import json
import paho.mqtt.client as mqtt
from shared.config import BROKER_HOST, BROKER_PORT, TOPIC

def publish_ota_message(json_path):
    # UTF-8로 강제 지정
    with open(json_path, "r", encoding="utf-8") as f:
        message = json.load(f)

    client = mqtt.Client()
    client.connect(BROKER_HOST, BROKER_PORT, 60)
    client.publish(TOPIC, json.dumps(message, ensure_ascii=False))  # ensure_ascii=False → 한글 유지
    print(f"[OTA] '{message['version']}' 버전 메시지를 전송했습니다.")

if __name__ == "__main__":
    publish_ota_message("ota_message.json")
