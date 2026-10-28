# ORDR iPad 시뮬레이터 — 비공개 테스트 자산

**운영 상태/실행 정책:** [최신 인수인계](ordr-native-handoff.md) 참조. 이 문서는 저작권 보호 및 테스트 자산 반입 방법만 설명한다.

- 공개 GitHub 저장소·커밋·Actions Artifact에는 **원본 W3X, Warcraft III MPQ, 개인 인증 토큰을 절대로 포함하지 않는다**.
- 기존 워크플로 .github/workflows/ordr-ipad-simulator.yml은 비공개 자산을 macOS Runner의 임시 시뮬레이터 Documents/Maps에 내려받은 다음, 화면·진단 로그만 남긴다. Assets가 없으면 메뉴만 실행될 수 있으므로 '실행 성공'을 맵 로딩 성공으로 해석하지 않는다.
- 소스는 tools/ipad/fetch_private_drive_assets.py. 지원하는 Actions secrets: ORDR_DRIVE_REFRESH_TOKEN, ORDR_DRIVE_CLIENT_ID, ORDR_DRIVE_CLIENT_SECRET. 기존 ORDR_DRIVE_ACCESS_TOKEN이나 ORDR_TEST_ASSETS_URL도 대체 경로로 제공된다. **값·파일 ID는 문서에 남기지 않는다.**
- 비공개 Drive에는 원본 소유자의 War3*.mpq와 분할 원본 ORDR_S2_2.323[C].w3x 자산이 있으며, Runner에서 조립·검증 후 임시 위치에 사용한다. 자산의 실제 존재와 해시는 실행 시 verified-private-assets.json 및 asset-status.txt에서 확인한다. 오래된 파일 목록/크기만으로 최신 상태를 추정하지 않는다.

## 운영 절차

1. 사용자가 명시적으로 요청한 검사 시점에만 기존 워크플로를 사용한다. 기본 push로는 시뮬레이터를 구동하지 않고 **[ipad-test] 명시 커밋**이 있는 경우에만 실행하도록 설정되어 있다. 새 워크플로 생성·반복 실행·취소 금지.
2. 최근 수정 건을 묶어서 headless/CI에 통과시킨 뒤 한 번 실행한다. 워크플로는 앱 설치, 원본 맵 열기, 난이도/모드/위습/건물 입력 시도, 최대 약 1800초 관찰, 이미지와 로그 보존을 포함한다. **자동 입력이 대상을 선택하지 못할 수 있다**.
3. 성공 판정은 실행 콘솔이 아니라 캡처와 engine-stage-summary.json에서 **첫 라운드 이후 몹 실제 좌표 변화, 사망에 따른 카운트 갱신, 건물 실효과**로 판단한다.
4. 실기기 로그 경로: Files → On My iPad → OpenWarcraft3 → ordr-jass.log. 맵을 다시 시작하면 로그가 덮어써질 수 있다. 사용자의 실기기 화면·버튼 터치와 함께 대조한다.

**기록해야 할 실패:** 자산 없음 / 앱 시작 불가 / 난이도 선택 불가 / 첫 웨이브 미도달 / 몹 미이동 / 건물 선택 불가 / 버튼 명령 누락 / 명령 실행 후 무효 / 카운트 미갱신. 단계가 막히면 다음 단계는 '미검증'으로 남긴다.
