
vc(vehicle computer software) 전체 폴더 구조 및 파일 역할
(**systemd 관련은 아래 잘 읽고 수행할 것**)

**현주가 해야할일 : .service 경로 다 수정됨 다시 바꾸기**

vc_software/
├── apps/
│   ├── realtime/                  # 실시간 제어 서비스 (C++ 단일 실행파일)
│   │   ├── main.cpp               # 엔트리포인트
│   │   ├── shared.h               # 공유 데이터 구조체 정의
│   │   ├── sensor_thread.cpp      # ECU → 센서값 수신 (SOME/IP)
│   │   ├── joystick_thread.cpp    # 조종기 입력 처리
│   │   ├── control_thread.cpp     # APS/AEB/HBA 로직
│   │   └── comm_thread.cpp        # ECU 제어 명령 송신 + engine_on 동기화
│   │
│   ├── digital_key/               # 디지털키 서비스
│   │   └── digital_key_service.py # BLE 인증 → VC에 START/STOP 전달
│   │
│   ├── ota/                       # OTA 서비스
│   │   └── ota_service.py         # 원격 서버 → 파일 다운로드 및 교체
│   │
│   └── __init__.py                # (파이썬 패키지 관리용, 비워둬도 됨)
│
├── include/                       # (선택) 공용 헤더 모음
│   ├── vc_common.h                # 상수, enum, 로그 매크로 등
│   └── config.h                   # 설정값 (IP, 포트, 주기)
│
├── systemd/                       # 서비스 정의 (부팅 시 자동 실행)
│   │ **여기 있는 파일들을 /etc/systemd/system에 복사해서 등록하라는 의미임. 실제로 systemd가 읽는 폴더 아님!!**
│   ├── vc-realtime.service        # → apps/realtime/vc_realtime 실행
│   ├── vc-digital-key.service     # → apps/digital_key/digital_key_service.py
│   ├── vc-ota.service             # → apps/ota/ota_service.py
│   └── vc-rfcomm0.service         # (블루투스 직렬 포트용, 선택)
│
└── README.md                      # 프로젝트 설명




/etc/systemd/system → 운영체제 전체(system-wide) 서비스
이게 진짜 systemd가 인식해서 부팅 시 자동 실행하는 위치야.
즉, “서비스로 등록해서 부팅 시 실행하고 싶다”면 반드시 이 경로로!

**실제 systemd 동작시키려면 이렇게 하기**
1️⃣ 프로젝트 안에서 템플릿 작성(예시: **vc-realtime.service** 라는걸 만들었다면)

nano ~/vc_software/systemd/vc-realtime.service


2️⃣ 시스템 등록 (root 권한 필요)

sudo cp ~/vc_software/systemd/vc-realtime.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable vc-realtime
sudo systemctl start vc-realtime


3️⃣ 상태 확인

systemctl status vc-realtime
