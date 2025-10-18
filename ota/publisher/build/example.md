## 해당파일을 json 파일로 변환하여 사용하세요

```json
{
  "version": "1.3.0",                                               // OTA 메시지 버전: 실제 배포되는 실행파일이나 서비스의 버전
  "checksum": "a2f4e2b839f1dce09b6d998f4af3d99a",                   // checksum: OTA 파일의 무결성을 확인하기 위한 SHA256 해시 값
  "description": "엔진 제어 알고리즘 개선 및 연비 튜닝 버전입니다.",  // description: 이번 업데이트 내용이나 변경 사항 설명 (로그/사용자 알림용)
  "target": {   // target: OTA가 적용될 대상 파일/서비스 관련 정보
    "process_check": "main_app",                                            // process_check: 실행 중인 프로세스를 식별하기 위한 키워드 (또는 systemd 서비스명)
    "target_path": "/home/pi/vc_software/apps/realtime/build/vc_realtime",  // target_path: 덮어쓸 대상 파일의 절대 경로 (라즈베리파이 쪽)
    "backup_path": "/home/pi/vc_software/apps/ota/backups/realtime/",       // backup_path: 교체 전 기존 파일을 백업할 경로
    "source_path": "http://192.168.137.1:8000/build/vc_realtime"             // source_path: OTA 파일이 호스팅된 URL (파일 서버 주소)
  },
  "actions": {  // actions: OTA 수행 순서를 단계별로 정의한 목록
    "precheck": [   // precheck: 업데이트 전 수행할 단계들 (프로세스 중지, 상태 확인 등)
      "check_process",       // 프로세스가 실행 중인지 확인
      "stop_process"         // 실행 중이면 중지
    ],
    "apply": [  // apply: 실제 파일 교체 과정
      "backup",              // 기존 파일 백업
      "replace_file",        // 새로운 파일로 덮어쓰기
      "set_permission"       // 실행 권한 설정 (chmod)
    ],
    "postcheck": [   // postcheck: 교체 후 수행할 단계들
      "restart_systemd",     // 관련 서비스 재시작
      "verify_checksum",     // 새 파일의 해시 검증
      "update_version"       // versions.json 갱신
    ]
  },
  "require_confirm": true   // require_confirm: true면 OTA 적용 전에 사용자 승인 필요
}
```