# publisher/send_ota.py
import os
import sys
import json
import hashlib
from ota_publisher import publish_ota_message

BASE_DIR = os.path.dirname(__file__)
BUILD_DIR = os.path.join(BASE_DIR, "build")

def calc_checksum(file_path: str) -> str:
    """SHA-256 체크섬 계산"""
    h = hashlib.sha256()
    with open(file_path, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return h.hexdigest()

def update_checksum_in_json(json_path: str) -> str:
    """JSON의 'checksum'을 실제 파일 기준으로 갱신하고, 저장 후 경로 반환"""
    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    # source_path(HTTP URL) 말고, 실제 체크섬 대상 파일 경로 지정
    # (지금 샘플은 build/test_app_v2.sh)
    source_name = os.path.basename(data["target"]["source_path"])
    local_source = os.path.join(BUILD_DIR, source_name)

    checksum = calc_checksum(local_source)
    print(f"[체크섬 계산 완료] {source_name} → {checksum}")

    data["checksum"] = checksum

    # 원본을 직접 덮거나, 별도 파일에 저장
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)

    print(f"[JSON 업데이트 완료] {os.path.basename(json_path)}에 checksum 반영 완료.")
    return json_path

def main():
    if len(sys.argv) < 2:
        print("사용법: python send_ota.py <json파일경로>")
        sys.exit(1)

    json_path = sys.argv[1]
    if not os.path.exists(json_path):
        print(f"[오류] '{json_path}' 파일을 찾을 수 없습니다.")
        sys.exit(1)

    print(f"[OTA 전송 시작] {json_path}")
    updated_json = update_checksum_in_json(json_path)
    publish_ota_message(updated_json)
    print(f"[OTA 전송 완료] {os.path.basename(json_path)} 발행 성공.")

if __name__ == "__main__":
    main()
