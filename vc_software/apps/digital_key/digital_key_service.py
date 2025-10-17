"""
디지털 키 서비스를 위한 실행 스크립트.
블루투스 연동 로직은 아직 개발 중이지만, 서비스는 부팅 이후에도
지속적으로 동작해야 하므로 기본 하트비트 루프를 제공한다.
"""

import json
import signal
import sys
import time
from pathlib import Path


BASE_DIR = Path(__file__).resolve().parent
LOG_DIR = BASE_DIR / "logs"
STATE_PATH = BASE_DIR / "state.json"
CONFIG_PATH = BASE_DIR / "config.json"

LOG_DIR.mkdir(parents=True, exist_ok=True)


def log(message: str) -> None:
    """
    간단한 파일 기반 로그 함수.
    systemd의 `journalctl`에서도 stdout/stderr를 수집하므로
    파일과 표준 출력에 모두 남긴다.
    """
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
    line = f"[{timestamp}] {message}"
    log_file = LOG_DIR / "digital_key.log"
    with log_file.open("a", encoding="utf-8") as fp:
        fp.write(line + "\n")
    print(line, flush=True)


class DigitalKeyService:
    """
    실제 블루투스 키 교환 로직이 추가될 때까지의 자리표시자 구현.
    구성 파일을 주기적으로 확인하고, 상태 파일에 마지막 동작 시각을 저장한다.
    """

    def __init__(self) -> None:
        self._running = True
        self._config = {}

    def stop(self, *_: object) -> None:
        self._running = False

    def load_config(self) -> None:
        if not CONFIG_PATH.exists():
            log("경고: config.json 파일이 없어 기본 설정으로 동작합니다.")
            self._config = {}
            return

        try:
            with CONFIG_PATH.open("r", encoding="utf-8") as fp:
                self._config = json.load(fp)
        except Exception as exc:  # pylint: disable=broad-except
            log(f"설정 파일 로드 실패: {exc}")
            self._config = {}

    def write_state(self) -> None:
        payload = {
            "last_heartbeat": time.time(),
            "config_version": self._config.get("version", "unknown"),
        }
        try:
            with STATE_PATH.open("w", encoding="utf-8") as fp:
                json.dump(payload, fp, indent=2, ensure_ascii=False)
        except Exception as exc:  # pylint: disable=broad-except
            log(f"상태 기록 실패: {exc}")

    def heartbeat(self) -> None:
        log("디지털 키 서비스 하트비트 — 활성 상태 확인 완료")
        self.write_state()

    def run(self) -> None:
        log("디지털 키 서비스 시작")
        self.load_config()

        while self._running:
            self.heartbeat()
            time.sleep(5)

        log("디지털 키 서비스 종료")


def main() -> int:
    service = DigitalKeyService()
    signal.signal(signal.SIGTERM, service.stop)
    signal.signal(signal.SIGINT, service.stop)

    try:
        service.run()
    except Exception as exc:  # pylint: disable=broad-except
        log(f"치명적 오류 발생: {exc}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
