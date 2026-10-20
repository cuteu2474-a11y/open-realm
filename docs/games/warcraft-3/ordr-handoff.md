# 원랜디 iPad 포팅 인수인계

갱신: **2026-10-19**. 목표는 사용자 소유 **ORDR S2 2.323[C]의 오프라인 단일 플레이**다.
현재는 초기화와 일부 런타임을 검증한 단계이며 iPad 플레이 완료 상태가 아니다.

## 이어받을 위치

- 저장소: `cuteu2474-a11y/open-realm`
- 작업 브랜치: **`fix/ordr-compat-jass-logging`**
- 기존 초안 PR: [#5](https://github.com/cuteu2474-a11y/open-realm/pull/5), 대상 `ordr-compat-01`
- 이번 작업의 출발점: `8b14d6d6a1ed325561524afa8d91d08b5591c27e`

`main`이나 초기 `ordr-compat-01`만 보고 진행 상태를 판단하지 않는다. 작업 브랜치와
PR의 최신 커밋을 먼저 확인하고 그 위에서 진행한다. 이미 구현된 기능을 다시 작성하지 않는다.

```sh
git fetch origin
git switch fix/ordr-compat-jass-logging
git pull --ff-only origin fix/ordr-compat-jass-logging
git log -12 --oneline
git status --short
```

## 기존 구현과 이번 변경

| 영역 | 현재 상태 |
| --- | --- |
| JASS | 이스케이프 따옴표·주석 처리 수정. 원본 전체 스크립트 파싱 가능 |
| 확장 native | EX 동작, JN 문자열·정규식, TOC 읽기 등 기존 구현 유지 |
| 로컬 동기화 | 동일 프로세스 DzSyncData 등록·큐·발신자·페이로드 전달 구현. 온라인 서비스와 구분 |
| DzFrame | 이름·부모·위치·크기·텍스트·스타일·콜백을 논리 상태로 보관. 실제 화면·터치 연결 미완료 |
| 채팅 | 인증된 입력 → exact/substring 트리거와 이벤트 응답 전달 구현 |
| 초기화 진단 | 잘못된 trigger/timer 핸들 방어, 큰 테스트 월드, iPad 첫 120프레임 진단 구현 |
| 이번 이벤트 수정 | 1024 → 2048 슬롯. region-event 토큰 비트 조정. 저장 형식 89 → 90 |
| 이번 FDF 수정 | 파싱 실패를 성공 캐시에 넣던 결함 수정. 실패 경로를 DzLoadToc 진단에 기록 |
| 이번 FDF 속성 | LayerStyle 클릭 통과, SpriteScale 구문·기본값·로컬 메뉴/MDX 크기 적용. 실제 ORDR FDF 5개 템플릿 검증 |

네트워크 패킷은 변경하지 않았다. 저장 형식 92는 이전 저장을 거부하며 마이그레이션하지 않는다.
[저장 계약](save-load.md), [이벤트 구조](trigger-events.md), [기기 진단](ipad-custom-map-diagnostics.md)을 참고한다.

## 검증 증거

수정 전 회귀 테스트에서 804개 채팅 등록 뒤 250개 타이머를 등록하면 마지막 30개가 누락됐다.
수정 후 높은 슬롯의 event 핸들을 저장·복구하고 실제 `globals.RunFrame()`을 진행하면
마지막 타이머가 정확히 한 번 실행된다. region-event 세대/식별자 변환과 폐기된 핸들도 검증한다.

원본 스크립트와 실제 ROC 자료를 큰 테스트 월드에서 재생한 결과:

| 관찰 | 수정 전 | 이벤트 수정 후 |
| --- | --- | --- |
| main 종료 | triggers=1488, timers=1063, VM 오류 없음 | 동일 |
| 이벤트 한도 경고 | 30회 | 0회 |
| 첫 타이머 처리 | events=1024/1024 | events=1054/2048 |
| 120번째 처리 | events=1024/1024, triggers=1488 | events=1066/2048, triggers=1500 |
| 엔티티 | entities=67, alive=43, p7=29, p15=14 | 동일 |

이 하네스는 `G_RunTimers`, `G_RunEvents`, `jass_runevents`를 호출하며 전체 물리·렌더링·터치
프레임이나 첫 웨이브 완료를 뜻하지 않는다. 맵 MPQ를 마운트하지 않는 하네스의 TOC 누락을
실제 맵 내부 파일 누락으로 해석하지 않는다.

별도 W3X `+map` 제한 실행은 초기화와 120프레임 종료에 도달했지만, UI 초기화에서
`war3mapImported\ORDR.fdf`의 `LayerStyle` 파싱 실패가 드러났다. 종료 코드 0이나
`MAIN PASS`만으로 호환성 성공을 선언하면 안 된다.

후속 수정은 이 실패를 공개 합성 FDF에서 먼저 재현했다. `IGNORETRACKEVENTS`는 기존
passthrough 입력 플래그로 연결했고, `NOSHADING`은 기존 UI의 무조명 렌더링과 일치한다.
`SpriteScale`은 고전 FDF의 3수 구문을 소비하되 첫 값을 균일 배율로 사용한다. 생략 시 1,
명시적 0이면 로컬 메뉴의 모델 그리기를 생략한다. 렌더러가 항상 1로 고정하던 것도 수정했다.
지원하지 않는 `SETSVIEWPORT`는 여전히 실패를 보고한다.

원본 맵에서 추출한 1382바이트 `ORDR.fdf`는 선택형 테스트로 ROC/TFT 설정 모두
5개 템플릿이 생성되고 `0.0005` 배율이 로컬 그리기 호출에 전달됨을 확인했다(18 assertions).
공개 테스트는 입력 판정, 뒤따르는 템플릿, 기본/0 배율, 실제 MDX render entity의 크기를 검사한다.
이는 **동적 DzFrame의 게임 HUD 표시·터치 완료를 뜻하지 않는다**. 추가된 `drawSprite_t.scale`은
이미 계산된 크기를 전달하는 로컬 렌더러 인자이며 네트워크 구조는 바꾸지 않는다. FDF 해석과
MDX 적용은 WC3 디렉터리에 유지한다. 템플릿은 저장 파일에 직렬화되지 않아 추가 저장 버전 변경은 없다.

FDF 캐시 회귀도 수정 전 ROC/TFT 설정 모두 실패를 재현했다. 수정 후 실패 반환,
같은 경로의 올바른 데이터로 재시도, 성공한 파일만 캐시하는 동작을 검증했다.
파싱 실패 시 이미 생성된 프레임의 전체 롤백은 아직 지원하지 않는다.

2026-10-18 오전의 전체 `make test`는 JUnit 32개 보고서, 6733개 테스트가 실패 없이
통과했다(ROC 엔진 2719개, TFT 엔진 2719개 포함). 메뉴 테스트는 111개/1447 assertions,
원본 경로를 제공한 Python ORDR 테스트는 3개 모두 통과했다. 오후 작업공간 정리 후에는
동일 패치를 GitHub 기준 커밋 위에 복구했고 전체 `make test`가 다시 성공했다.
이전 임시 커밋 ID를 재사용하지 않는다.

UI 속성 수정 뒤 전체 `make -j4 test TEST_JOBS=4`는 **32 보고서, 6738 테스트,
실패/오류 0**을 기록했다. 메뉴 115개/1473 assertions, MDX UI 19개/99 assertions다.
선택형 원본 FDF 검사는 별도로 실행해 18/18 assertions를 확인했다. 공유 로컬 렌더러
인자가 바뀌므로 `make -j4 openwarcraft3 openwow opensc2`도 모두 빌드했다.
메뉴 테스트는 `WC3_DEBUG_GLUE=1`로 강제 재빌드한 변형도 115개/1473 assertions를 통과했다.
`engine_boundary_audit.py --base HEAD`, `menu_boundary_audit.py`, `git diff --check`도 통과했다.
새 합성 FDF 테스트의 수정 전 실패(8 assertions)와 MDX 크기 테스트의 수정 전 실패(1 assertion)를
확인한 뒤 구현했다. 저녁 작업공간 자동 정리 후 GitHub 체크포인트 위에 같은 수정을 복구하고
전체 테스트·원본 FDF 검사를 다시 통과시켰다. 정리 중 남은 0바이트 `openwow-tests`는 재빌드했다.
복구 전에 시작한 빌드의 완료 여부를 추정하지 말고, 실제 종료 결과를 확인한다.

## 재실행

일반 Linux 빌드 의존성을 갖춘 환경에서:

```sh
make -j4 test TEST_JOBS=4
make -j4 openwarcraft3-tests test-assets build/bin/mpqtool
build/bin/openwarcraft3-tests -data build/tests +dedicated 1 +test 'wc3_api.ordr_*'
build/bin/openwarcraft3-tests -tft -data build/tests +dedicated 1 +test 'wc3_api.ordr_*'
python3 -m unittest discover -s tests -p 'test_ordr_*.py' -v
```

원본 파일은 저장소 밖에 둔다. `ORDR_DATA`를 자신의 절대 경로로 지정한다.

```sh
ORDR_DATA=/absolute/path/to/private-assets
build/bin/mpqtool -mpq "$ORDR_DATA/Maps/ORDR_S2_2.323[C].w3x" \
  cat war3map.j > "$ORDR_DATA/war3map.j"
ORDR_JASS_PATH="$ORDR_DATA/war3map.j" ORDR_TIMED_FRAMES=120 \
  timeout 180 build/bin/openwarcraft3-tests -data "$ORDR_DATA" +dedicated 1 \
  +test wc3_api.ordr_private_main_from_script > "$ORDR_DATA/main-smoke.log" 2>&1
```

VM 오류, 한도 경고, 1054 → 1066 이벤트 진행을 함께 확인한다. 합성 MPQ의 `Blizzard.j`로
원본 전체를 실행하면 `SetAmbientDaySound` 같은 BJ 래퍼가 없어 실패할 수 있다.
실제 War3.mpq에는 해당 래퍼가 있다. 이를 숨기는 native나 축약 스크립트를 추가하지 않는다.

맵 내부 TOC/FDF·유닛 오버레이 검증이 필요할 때만 아래 제한 실행을 사용한다.

```sh
timeout 120 build/bin/openwarcraft3-tests -data "$ORDR_DATA" +dedicated 1 \
  +set com_fast_forward 1 +map 'Maps/ORDR_S2_2.323[C].w3x' \
  +com_frame_limit 120 > "$ORDR_DATA/map-smoke.log" 2>&1
rg -n 'event slot limit|Failed to parse|DzLoadToc|WC3_JASS_ERROR|Unknown token' \
  "$ORDR_DATA/map-smoke.log"
```

해당 ORDR FDF 오류는 재현 테스트에서 해결했다. 그 이후 전체 맵의 다음 장애는 별도로 확인한다.
실제 TFT 자료 확보 후 `-tft`로도 검증한다.
합성 ROC/TFT 테스트와 실제 TFT 맵 실행을 구분한다.

원본 FDF 오류는 게임 실행 없이 아래 명령으로 재검증한다. 비공개 파일이 없는 CI에서는
선택형 테스트만 명시적으로 생략하며 공개 합성 FDF 회귀는 항상 실행한다.

```sh
build/bin/mpqtool -mpq "$ORDR_DATA/Maps/ORDR_S2_2.323[C].w3x" \
  cat war3mapImported/ORDR.fdf > "$ORDR_DATA/ORDR.fdf"
ORDR_FDF_PATH="$ORDR_DATA/ORDR.fdf" \
  build/bin/test_openwarcraft3_ui menu_fdf.ordr_private_fdf_from_file
```

## 원본 자료·대용량 다운로드

사용자 Drive에서 이름 검색이 빈 결과여도 루트 폴더 목록을 확인한다.
접근 ID·서명 URL·원본 바이너리를 공개 저장소에 기록하지 않는다.

| 파일 | 바이트 | 확인 |
| --- | ---: | --- |
| ORDR_S2_2.323[C].w3x | 133379181 | 제공 원본, 추출·맵 실행 |
| War3.mpq | 557726676 | 웹 다운로드 완료, 크기·해시·MPQ 확인 |
| War3Local.mpq | 620716 | 오전 확보 |
| War3xLocal.mpq | 66809059 | 오전 확보 |
| War3Patch.mpq | 524476 | 오전 확보. placeholder 항목만 있는 파일 |
| War3x.mpq | 381604857 | Drive 존재 확인. 브라우저 완료 경로 이후 실제 파일 전달 미확인 |

원본 맵 SHA-256: `0794996c9bc73eaa3020f20ebc45fb37cfa64a6e98ffe8aa0dfa3b356e77bc5c`.
War3.mpq SHA-256: `b08095c003f657fdc3d82a2f5c1cd1abc8fd4a7c27d428a225a48489012f0e29`.
추출 스크립트는 2503677바이트, 63651줄이다.

Drive 커넥터의 256 MiB 제한은 `include_base64=false`로도 해결되지 않았다.
War3.mpq는 사용자 Drive 웹 다운로드로 해결했다. 완료 이벤트만 믿지 말고
실제 파일 존재·크기·MPQ 읽기를 확인한다. 임시 경로에 의존하지 말고 이름과 해시로 복구한다.

## 2026-10-19 저장·복원 후속 수정

출발 커밋은 `ee07e70c`다. `g_save.c`의 명시적 `level_fields` 목록에 ORDR 프레임과
동기화 등록이 빠져 있었다. 새 VM을 만들고 두 레지스트리를 비운 뒤 기존 저장을 읽으면
`ReadGame`은 성공하지만 UI 이름·텍스트·콜백과 동기화 등록을 복구하지 못했다.
수정 전 재현은 8개 assertions 중 5개 실패, 수정 후 같은 재현은 8/8 통과했다.

이 체크포인트의 저장 형식은 **91**이었다. 프레임 속성은 값으로, 콜백은 `F_FUNCTION` 함수 이름으로,
동기화 대상은 `F_TRIGGER` 슬롯으로 복원한다. 각 레지스트리와 콜백 개수는 기존 counted
schema로 한도를 검사한다. 네트워크 형식과 JASS snapshot 버전은 바꾸지 않았다.
구 버전 저장은 거부하며 변환하지 않는다. 원랜디 온라인 계정 저장 기능과는 별개다.

재현 및 회귀 이름:
- `wc3_api.ordr_frame_and_sync_registries_survive_reload`
- `wc3_api.ordr_frame_attributes_and_empty_registries_roundtrip`
- `wc3_api.ordr_invalid_registry_counts_do_not_replace_valid_save`

첫 검사는 VM을 다시 파싱하고 저장 파일을 읽은 뒤 `DzSyncData`를 호출해
실제 `globals.RunFrame()`으로 콜백 1회 실행을 확인한다. 나머지는 속성 보존,
null 콜백, 빈 등록 복구, 잘못된 개수·trigger 포인터 거부와 이전 저장 보존을 확인한다.
검증: `make -j4 test TEST_JOBS=4` 완료, JUnit 32개 보고서 **6,744 tests / 0 failures / 0 errors**
(ROC/TFT 엔진 포함). `make -j4 openwarcraft3`, `menu_boundary_audit.py`, `git diff --check`도 통과했다.
상용 맵 재실행과 iPad 빌드는 이번 로컬 검사에 포함하지 않았다.
이 수정은 **동적 UI 표시·터치 연결을 구현한 것이 아니다**. 다음 순서는 그대로다.

새 작업 환경에 SDL/GL 개발 패키지가 없으면 기본 빌드부터 실패한다.
이번에는 APT 캐시를 쓰기 가능한 별도 디렉터리로 지정해 개발 패키지 설치를 완료했다.
저장소 테스트 데이터는 `make test-assets`로 생성하며 상용 파일 없이 회귀를 실행했다.

## 2026-10-19 UI 입력 수신 경로

출발 커밋 `f876141c`의 iPad 워크플로 실행 127은 성공했다.
이번 변경은 **서버 입력 수신 → JASS 콜백 → 로컬 DzSyncData** 경로다.
`G_ClientCommand`가 `dzframe <frame-id> <event-id>`를 받으면 `G_OrdrFrameEvent`로 전달한다.
명령에 플레이어 번호를 받지 않고 실제 클라이언트 엔티티에서 송신자를 결정한다.
숨김/비활성 프레임과 그 조상, 없는 부모, 순환 관계, 등록되지 않은 이벤트 및 null 콜백은
실행하지 않는다. 숫자 입력은 부호·공백·접미사·범위 초과를 거부한다.

`jassContext_t.ui_frame_id`와 기존 플레이어 컨텍스트는 코루틴마다 보관한다.
`DzGetTriggerUIEventFrame/Player`는 이벤트 밖에서는 0/null을 반환한다.
`ExecuteFunc`와 명시적인 `TriggerExecute`에서 UI 컨텍스트를 보존하며,
별도의 동기화 이벤트에는 UI 프레임 컨텍스트를 전달하지 않는다.
콜백이 `TriggerSleepAction`으로 대기 중일 때 저장해도 복구하도록 JASS snapshot은 **10**,
게임 저장 형식은 **92**로 올렸다. 네트워크 패킷은 변경하지 않았다.
`sync=true/false` 콜백은 모두 오프라인 로컬 스케줄러로 실행하며 멀티플레이 보장은 아니다.

재현: `+test 'wc3_api.ordr_ui_command*'` (ROC/TFT).
클라이언트 명령을 넣고 실제 `globals.RunFrame()`으로 UI 콜백을 대기시킨 뒤 저장·복구하고,
송신자/프레임 정보 및 동기화 콜백 1회 실행을 검사한다. 잘못된 입력과 숨김/비활성/순환
상태에서 명령을 먼저 보낸 뒤 정상 명령 하나만 실행되는지도 같은 흐름에서 확인한다.

검증: 수정 전 명령 경로 재현 실패를 확인한 뒤 구현했다. 테스트에서는 `level.time`을
`FRAMETIME`만큼 명시적으로 전진시켜 대기 콜백의 재개를 검증한다(테스트의 `gi.GetTime`
자체는 시간을 증가시키지 않는다). 저장 직전에도 콜백 1회 진입·동기화 0회를 확인한다.
`make -j4 test TEST_JOBS=4`: **32 JUnit 보고서 / 6,748 tests / 0 failures / 0 errors**.
`make -j4 openwarcraft3 opensc2`, 메뉴 경계 감사와 `git diff --check` 통과.
이번 변경의 iPad 자동 빌드와 실기기 플레이는 아직 별도 확인 대상이다.

**아직 완료되지 않은 부분:** FDF 템플릿의 동적 HUD 인스턴스 생성·출력,
화면 hit-test에서 위 명령을 발행하는 연결, 실제 iPad 터치. 현재 명령 경로의 검증을
터치/화면 표시 완료로 보고하지 않는다. 원본 맵의 난이도 선택과 첫 웨이브도 별도 검증해야 한다.

## 다음 순서

1. 기존 논리 DzFrame 상태를 템플릿 인스턴스와 HUD 출력에 연결한다. 터치 → 콜백 →
   로컬 DzSyncData로 이어지는 새 명령 경로를 HUD hit-test에 연결한다. 논리 프레임 생성부터 다시 작성하지 않는다.
   공유 네트워크 형식 확장이 필요하면 AGENTS.md의 승인 규칙을 먼저 확인한다.
2. War3x.mpq 전달을 완료하고 전체 TFT 데이터로 비교한다.
3. 최신 IPA로 실제 iPad의 난이도 선택·첫 웨이브·전투·터치·장시간 실행을 확인한다.
   큰 테스트 월드는 이미 수정됐으므로 과거 작은 월드의 CUN CreateUnit null을 기기 결함으로 단정하지 않는다.

핵심 파일은 `games/warcraft-3/` 아래의 `game/g_local.h`, `game/g_utils.c`, `game/g_save.c`,
`game/api/api_ordr.h`, `game/api/api_ordr_frames.h`, `game/g_main.c`, `game/tests/t_api.c`,
`common/stb_fdf.h`다.

기존 `.github/workflows/ordr-ipad-build.yml`의 최신 PR 실행에서
`ordr-openrealm-diagnostic-ipa`를 확인한다. 서명·프로비저닝은 별도다.
이벤트·FDF 캐시 체크포인트 `54e5b3c4`는 JASS 회귀, Linux 합성 런타임, arm64 iPad 빌드와
진단 IPA 패키징을 모두 통과했다([실행 101](https://github.com/cuteu2474-a11y/open-realm/actions/runs/39107574199)).
후속 CI는 TFT ORDR 회귀와 `test-menu`, `test-mdx-ui`도 실행한다.
작업 종료마다 이 문서의 검증 결과·남은 첫 장애를 코드와 함께 커밋·푸시한다.
비공개 맵·추출 스크립트·상용 MPQ·자격 증명은 커밋하지 않는다.
