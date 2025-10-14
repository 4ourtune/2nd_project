# publisher/ota_publisher.py
import json
import paho.mqtt.client as mqtt
from config import BROKER_HOST, BROKER_PORT, TOPIC

def publish_ota_message(json_path: str):
    """
    지정된 OTA JSON 파일을 읽어 MQTT Broker로 발행한다.
    """
    # 1) JSON 로드
    with open(json_path, "r", encoding="utf-8") as f:
        message = json.load(f)

    payload = json.dumps(message, ensure_ascii=False)

    # 2) MQTT 연결
    client = mqtt.Client(protocol=mqtt.MQTTv311)
    # 필요시 인증:
    # client.username_pw_set("user", "pass")

    # 연결 확인용 콜백(선택)
    # def on_connect(c, u, flags, rc): print("[MQTT] connected rc=", rc)
    # client.on_connect = on_connect

    client.connect(BROKER_HOST, BROKER_PORT, keepalive=30)

    # 3) 전송 루프 시작 + QoS=1로 전달 보장 강화
    client.loop_start()
    try:
        info = client.publish(TOPIC, payload=payload, qos=1, retain=False)
        info.wait_for_publish()  # ★ flush 보장
        # print(f"[DEBUG] published mid={info.mid}")
        print(f"[MQTT] {BROKER_HOST}:{BROKER_PORT} → {TOPIC}")
        print(f"[MQTT] {json_path} OTA 메시지 전송 완료")
    finally:
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    import sys
    if len(sys.argv) < 2:
        print("사용법: python ota_publisher.py <json파일경로>")
    else:
        publish_ota_message(sys.argv[1])
