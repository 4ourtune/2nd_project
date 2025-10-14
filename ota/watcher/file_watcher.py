import os, sys, time
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

# publisher 경로 인식
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from publisher.ota_publisher import publish_ota_message

# 절대경로 기반 감시 폴더
WATCH_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "../publisher/build"))

class OTAWatcher(FileSystemEventHandler):
    def on_created(self, event):
        if event.is_directory:
            return

        filename = os.path.basename(event.src_path)
        if filename.endswith(".json") or filename == "main_app":
            print(f"[감지] 새로운 파일 '{filename}' 이 추가되었습니다.")
            answer = input("이 파일을 OTA로 전송하시겠습니까? [y/n] : ").lower()
            if answer == "y":
                publish_ota_message(os.path.abspath(os.path.join(os.path.dirname(__file__), "../publisher/ota_message.json")))

if __name__ == "__main__":
    if not os.path.exists(WATCH_PATH):
        print(f"[에러] 감시 폴더가 존재하지 않습니다: {WATCH_PATH}")
        print("폴더를 생성한 후 다시 실행하세요.")
        sys.exit(1)

    observer = Observer()
    event_handler = OTAWatcher()
    observer.schedule(event_handler, path=WATCH_PATH, recursive=False)
    observer.start()

    print(f"[파일 감시 중] '{WATCH_PATH}' 폴더 내의 변경사항을 감시합니다.")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        observer.stop()
    observer.join()
