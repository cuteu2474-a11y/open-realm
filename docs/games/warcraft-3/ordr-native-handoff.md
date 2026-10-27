# ORDR 네이티브 구현 인수인계 — 2026-10-22

저장소: `cuteu2474-a11y/open-realm`  
브랜치: `feature/ordr-singleplayer-compat`  
기준 커밋: `a247c76c2f47c71184eb59740ec86d6052ee8a9b`

목표는 원본 ORDR S2 2.323[C]의 실제 함수 동작을 구현하고 난이도 유닛 선택이
스크립트 콜백까지 도달하게 하는 것이다. 이 문서는 Unity 리메이크가 아닌
OpenRealm iPad 프로젝트에 해당한다. 등록만 되거나 본문이 비어 있는 함수를 완료로 세지 않는다.

## 구현 및 검증

| 함수/경로 | 실제 변경 |
| --- | --- |
| `GetTriggerEventId` | 실제 이벤트 종류를 조건·액션·중첩 호출에 전달하고 대기/저장/복구 후 유지 |
| `GetHandleId` | enum은 주소 해시 대신 숫자 값 반환. 이벤트 상수와 getter 결과를 동일 배열 키로 사용 |
| `GetLearnedSkill`, `GetLearnedSkillLevel` | 실제 습득 이벤트 발행, 습득 시점의 rawcode와 레벨 보존 |
| `SetTimeOfDayScale`, `GetTimeOfDayScale` | 기본 1배, 배속, 0배 정지, 별도 suspend와 저장/복구 |
| `SetUnitAnimationByIndex` | 모델의 실제 0 기반 시퀀스 선택, 잘못된 번호 무시, 일시정지 중 표시 및 저장/복구 |
| `DestroyTrigger` | 등록 해제, 자체 타이머 정리, 외부 타이머 유지, 중복 삭제·조건 내부 자기 삭제 처리 |
| 중립 유닛 선택 | 실제 `select` 명령 → 프레임 스케줄러 → 이벤트 배열 조회 → 중첩 콜백 테스트 |

원본 맵은 `GetTriggerEventId()`를 `Da[DT]`에 넣고
`TriggerEvaluate(GE[GetHandleId(Da[DT])])`를 호출한다. 등록 시에도 이벤트 상수의
`GetHandleId`를 쓴다. 기존 null getter와 주소 해시 ID는 이 조회 경로를 망가뜨렸다.
해당 회귀 테스트는 수정 전에 이벤트 ID 및 선택 콜백에서 실패했고 수정 후 통과했다.
검증 범위는 입력 명령 이후의 스크립트 경로다. iPad의 물리 터치부터 화면까지의
전체 동작과 실제 원랜디 한 판 완료는 아직 검증하지 않았다.

- 새 집중 테스트: `wc3_native_completion.*`, 5개 테스트 / 37개 C assertion 통과.
  JASS `BJassAssert` 검증도 포함한다.
- 전체 `make test`: 종료 코드 0. RoC/TFT 엔진 및 공유 JASS/Galaxy 회귀 포함.
- 검사 도구 Python 테스트: 4개 통과. `git diff --check` 통과.
- 앞선 테스트 시도는 작업 환경 정리와 0바이트 테스트 실행 파일 때문에 중단되었다.
  깨끗한 체크아웃에서 의존성을 복구한 뒤 위 전체 테스트를 다시 실행해 통과했다.
- IPA 패키징이나 워크플로 수동 실행/취소는 하지 않았다.

게임 저장 형식은 89 → 90, JASS 스냅샷은 8 → 9다. 이전 저장 파일은 거부한다.
네트워크 메시지와 `playerState_t`/`entityState_t` 크기는 바꾸지 않았다.
삭제한 트리거 슬롯은 진행 중 콜백의 주소 안정성을 위해 tombstone으로 유지한다.
트리거/타이머 arena 슬롯 재사용은 별도 작업이다.

## 재현 명령

```bash
git fetch origin
git switch feature/ordr-singleplayer-compat
git pull --ff-only origin feature/ordr-singleplayer-compat
make test
build/bin/openwarcraft3-tests -data build/tests +dedicated 1 +test 'wc3_native_completion.*'
build/bin/openwarcraft3-tests -data build/tests -tft +dedicated 1 +test 'wc3_native_completion.*'
python3 -m unittest discover -s tests -p test_ordr_native_audit.py
```

원본 맵과 워크래프트 파일은 저장소에 넣지 않는다. 사용자가 보유한 데이터에서 추출한다.

```bash
mkdir -p /tmp/ordr-audit
build/bin/mpqtool -mpq '/path/to/ORDR_S2_2.323[C].w3x' cat war3map.j > /tmp/ordr-audit/war3map.j
build/bin/mpqtool -mpq '/path/to/War3.mpq' cat Scripts/Blizzard.j > /tmp/ordr-audit/Blizzard.j
python3 tools/ordr/audit_natives.py /tmp/ordr-audit/war3map.j \
  --support /tmp/ordr-audit/Blizzard.j --output /tmp/ordr-audit/report.json
```

`ordr-native-audit.json`은 함수명·호출 수·코드 위치와 입력 원문의 정규화 SHA256을
기록한다. `common.txt` 선언도 대조하므로 등록되지 않은 표준 함수가 빠지지 않는다.
`needs_review`는 구현 완료가 아니다. 동적 함수명, 실행 가능성, 데이터·렌더링·입력
동작까지 증명하는 도구는 아니다. 모든 함수가 구현됐다는 백분율을 만들지 않는다.

## 남은 함수와 우선순위

원본 직접 참조 네이티브 453개, Blizzard 래퍼 포함 514개. 감사 당시 등록표는 1,289개였다.
확인된 명백한 빈 본문은 22개이고, 등록 누락은 2개였다. 이미 본문이 있어도 부분 구현인
`UnitDamageTarget`, Dz UI 등은 이 숫자 외에 남아 있다.

| 구분 | 함수 |
| --- | --- |
| 과거 감사에서 등록 누락 | `UnitDamagePoint`, `IsItemIdSellable` — 둘 다 2026-10-23 등록·부분 기능 구현 |
| 수명/게임 규칙 | `RemoveLocation`, `UnitRemoveBuffs`, `UnitRemoveBuffsEx`, `UnitSuspendDecay`, `Cheat`, `GetEventDetectingPlayer` |
| 선택/화면/자세 | `EnableSelect`, `EnableDragSelect`, `EnableOcclusion`, `SetWaterBaseColor`, `SetUnitLookAt`, `ResetUnitLookAt` |
| 오디오 | `StopSound`, `SetSoundPitch`, `SetSoundChannel`, `SetSoundDistances`, `SetSoundDistanceCutoff`, `SetSoundConeAngles`, `SetSoundConeOrientation`, `NewSoundEnvironment`, `CreateMIDISound` |
| 원본 온라인 서비스 | `JNUse` |

1. **범위 피해부터:** 원본 `UnitDamagePointLoc`는 `UnitDamagePoint`를 수백 곳에서
   호출한다. 지연 시간, 범위, 실제 피해 이벤트와 저장/복구의 1차 경로를 구현했다.
   기존 `UnitDamageTarget`은 attack/ranged/type 인수를 무시하며, `UnitDamagePoint`도
   공격/사거리/무기 유형별 이벤트 및 모든 피해 타입별 면역/저항까지 구현된 것은 아니다.
   원작 피해 공식 전체가 완료되었다고 하지 않는다.
2. **핸들 수명:** `RemoveLocation`/`RemoveRect`를 단순 `free`로 바꾸지 않는다.
   `SaveLocationHandle`/`LoadLocationHandle`은 VM 참조를 유지하지 않는 원시 포인터다.
   먼저 VM·해시테이블의 소유권/무효화와 별칭·중복 삭제·저장/복구를 고쳐야 한다.
3. **버프:** 원본 `UnitRemoveBuffs(unit,false,true)`를 정확히 처리한다. 삭제는
   `unit_removestatus`의 능력별 정리 경로를 쓴다. 슬롯을 `memset`하면 보너스가 남는다.
   Ex 함수의 polarity/magic/physical/expiration/aura/auto-dispel 분류 근거가 필요하다.
4. **선택 제어:** `EnableSelect`/`EnableDragSelect`를 입력과 표시의 실제 소비 경로에
   연결한다. `uiflags`는 UI 레이어 마스크이므로 임의 입력 비트를 넣지 않는다.
5. **오디오:** 개별 재생 인스턴스의 stop/fade 경로가 필요하다. 전체 사운드를 중단하거나
   `gsound_t` 값만 저장하고 완료로 처리하지 않는다. pitch/distance/cone도 믹서에 연결한다.
6. **Dz/JN:** FDF 템플릿 생성, 렌더링, 실제 입력 콜백은 아직 미완료다. 로컬 오프라인
   프로필을 원본 온라인 계정·결제 서비스와 동일하다고 취급하지 않는다.

실기 로그는 `CHOICE_SELECT_COMMAND`, `CHOICE_USER_SELECTION`과 JASS 콜백을 함께
확인한다. 함수 오류를 가리려고 자동 난이도 선택이나 임의 HP 보정을 추가하지 않는다.

검토 순서는 (1) 이벤트/VM/저장, (2) 시간·모델·트리거 수명, (3) 테스트·감사 문서다.
추가 네트워크/렌더러 변경은 별도 의존 단계로 검토해야 한다.

## 2026-10-23 — 원본 맵 대조 및 아이템 네이티브 5종

- 사용자가 보유한 `ORDR_S2_2.323[C](3).w3x`에서 `war3map.j`를 추출했다. Python 텍스트 정규화 SHA256은
  `f59f0f059605a24b2360b9f7c493160f1b8776ec5dae621dfe9ccbde0d68f5d5`로 기존 감사 JSON과 **일치**한다.
  원본 맵 파일 자체와 추출된 JASS는 저장소에 추가하지 않는다.
- `CUN`은 `EnableDragSelect(false,false)`와 `EnableSelect(true,false)`로 드래그만 금지하고
  단일 유닛 선택은 유지한다. `BC6`은 `EVENT_PLAYER_UNIT_SELECTED`에서 원본 선택 유닛의
  `A900` 능력을 검사하여 `J8` 또는 `J9`로 분기한다. `J8`에는 난이도 `CUK`, 모드 `CUL`,
  게임 시작 `CUM`이 등록되어 있다. 자동으로 `CUM`을 호출하거나 입력 경로를 대체하지 않는다.
- 원본 JASS에 `UnitDamagePointLoc(` 호출이 **199회** 있다. 0 지연뿐 아니라 `.42`, `.7` 지연과
  `ATTACK_TYPE_NORMAL`/`ATTACK_TYPE_CHAOS`, `DAMAGE_TYPE_NORMAL`/`DAMAGE_TYPE_UNIVERSAL`이 쓰인다.
  `UnitDamagePoint` 미등록은 전투 정확도에 직접 영향을 주는 최우선 과제다. 지연·피해 유형을 무시한
  단순 반경 `T_Damage` 구현을 완료로 세지 않는다.
- `IsItemIdPowerup`, `IsItemIdSellable`, `IsItemIdPawnable`, `IsItemSellable`, `IsItemPawnable`를
  원본 `ItemData` 행과 `SetItemPawnable` 개별 인스턴스 오버라이드에 연결했다. 전부 JASS 네이티브
  5종이며, 데이터 플래그 읽기 이상의 상점 플레이 결과까지 원작과 대조된 것은 아니다.
- 새 회귀 테스트는 등록 이전 `unknown function: IsItemIdSellable`로 실패했고, 수정 후 통과했다.
  클래식/TFT 집중 테스트, `test_ordr_native_audit.py` 4개, 전체 `make test -j4` 모두 통과했다.
- 현재 등록표는 **1,294개**. 기존 `ordr-native-audit.json`은 **변경 이전의 1,289개 등록 상태를 기록한
  기준 스냅샷**이므로, 진짜 `Blizzard.j`를 사용해 다시 생성하기 전에는 최신 감사 결과로 인용하지 않는다.
  원본 `war3map.j`만으로 재검사하면 직접 참조 네이티브 453개를 검토할 수 있지만
  `Blizzard.j` 래퍼의 `UnitDamagePoint` 경로를 놓친다.

여전히 미검증: 실제 iPad 물리 터치, 선택 결과 UI 표시, 강제 종료 해결, 난이도-모드-전투의 전체 플레이.
GitHub Actions 실행/취소나 IPA 빌드는 이번 작업에서 하지 않았다.

## 2026-10-23 — `UnitDamagePoint` 범위·지연 피해 1차 구현

- `UnitDamagePoint` 네이티브를 등록했다. 즉시 호출은 중심점/반경에 있는 살아 있는
  유닛을 열거해 실제 `T_Damage`·피해 이벤트 경로로 전달한다. 무적 유닛은 공격하지 않는다.
  동맹 여부를 임의로 필터링하지 않으며, 피해 발생 시점의 현재 위치를 판정한다.
- 지연 호출은 서버 프레임에서 실행되는 숨겨진 엔티티에 시전 위치·반경·피해량·피해 유형·
  출처 유닛 세대를 저장한다. 같은 슬롯이 다른 유닛으로 재사용되면 새 유닛을 출처로
  잘못 판단하지 않는다. 기존 저장 형식의 엔티티 필드와 C 함수 저장 목록 끝에 등록하여
  저장/복구 후에도 지연 피해를 처리한다. 기존 함수 목록 인덱스는 바꾸지 않았다.
- `DAMAGE_TYPE_NORMAL`은 기본 공격 유형별 방어 상성/방어력 처리 함수를 이용하고,
  `DAMAGE_TYPE_UNIVERSAL`은 현재의 원시 `T_Damage` 경로를 이용한다.
  그 외 피해 유형은 동일한 원시 경로를 공유하는 **부분 구현**이다. 실제 소수점 피해,
  `attack`/`ranged` 플래그와 무기 유형별 효과, 각종 피해 면역/트리거 분류는 추가 대조가 필요하다.
- 새 회귀 테스트는 수정 전 `unknown function: UnitDamagePoint`로 실패했고,
  수정 후 즉시 범위/무적/범위 밖, 지연 위치 재평가, 저장/복구 후 지연 실행을 통과했다.
- 등록 네이티브는 이제 **1,295개**다. 감사 JSON은 여전히 이전 시점의 기준 스냅샷이다.
- 원본 `war3map.j`의 `UnitDamagePointLoc` 199회 직접 호출 및 지연 `.42`, `.7`,
  `ATTACK_TYPE_NORMAL`/`CHAOS`, `DAMAGE_TYPE_NORMAL`/`UNIVERSAL` 근거는 위 1차 분석과 동일하다.

## 2026-10-25 — 빈 인간 슬롯과 누락된 계산 HP 수정

- 원본 맵 `Units/UnitBalance.slk`는 `e0IX`/`e0HP`에 `HP=10`, `u000`에
  `HP=125`를 저장하지만 `realHP` 셀을 생략한다. 10월 23일 16:15:36
  iPad 로그는 `e0IX`의 실제 HP가 `0/0`임을 확인한다. 함수 누락만의
  문제가 아니라 계산 셀 생략을 처리하지 못한 데이터 로딩 결함이다.
- 누락된 `realHP`만 원본 HP와 기존 힘 보너스 공식으로 계산한다.
  명시적인 계산 HP(0 포함)는 보존한다. 회귀 테스트는 수정 전 실패했고,
  누락 셀/힘 보너스/명시적 0/명시적 계산 HP 및 실제 유닛 생성 경로를 검사한다.
- `GetPlayerSlotState`는 실제 연결된 인간 슬롯만 PLAYING으로 반환한다.
  맵의 컴퓨터 슬롯은 연결 없이 PLAYING, 제거된 슬롯은 LEFT다.
  원본 `BJF`/`Bcv`는 이 native와 `GetPlayerController`로 참가자를 판별한다.
  미접속 슬롯을 세는 회귀를 수정 전 재현했다. 파티 모드 실기 해결은 미확인이다.
- `wc3_api.beacon_enter_consumes_wisp_and_creates_owned_reward`는 실제 이동
  → 영역 이벤트 → 위습 숨김/사망 → 소유자 유지 보상 생성 경로를 검증한다.
  원본 CRw/CRy의 소비 순서를 사용하되 fixture 유닛과 고정 보상으로
  랜덤 테이블/원본 맵 전체 실행을 대체하지 않는다. 비콘 실기 해결을 주장하지 않는다.
- 원본은 `SetTerrainFog`/`SetUnitFog`를 호출하지 않는다. 초기 안개는
  `SetTerrainFogEx(0,4000,30000,0.5,1,1,1)`이고, CUC의 2초 필터는
  검정 불투명 → 흰색 투명이다. 밝아지는 연출 자체는 원본 동작이며,
  지형 소실 원인은 아직 확인되지 않았다. 안개를 끄거나 맵을 수정하지 않는다.
- 남은 범위: 실제 비콘과 모든 보상 rawcode/데이터, 누락 `realM`/`realdef`,
  실제 iPad 지형·카메라·조명, 피해 유형/면역. 전체 구현 완료로 취급하지 않는다.
- 검증: 수정 전 전체 `make test -j4` 통과. 새 슬롯/HP 회귀는 수정 전
  실패를 확인했고, 수정 후 ROC/TFT를 포함한 전체 `make test -j4`가
  종료 코드 0으로 통과했다. 네트워크/저장 형식 변경은 없다.

## 다음 채팅에 붙여 넣을 문장


> `cuteu2474-a11y/open-realm`의 `feature/ordr-singleplayer-compat`에서 이어서 작업해주세요.
> 먼저 `docs/games/warcraft-3/ordr-native-handoff.md`와 `ordr-native-audit.json`을 읽고,
> 남은 원랜디 네이티브를 실제 동작과 회귀 테스트까지 구현해주세요. 등록/빈 본문을 완료로
> 세지 말고 원본 선택 이벤트 경로를 유지해주세요. 워크플로를 수정마다 시작하거나 취소하지 마세요.

## 2026-10-26 — iPad 시뮬레이터 로딩 정체 진단

대상 실행: [40295825289](https://github.com/cuteu2474-a11y/open-realm/actions/runs/40295825289),
엔진 커밋 `8e09b0a`. 이 실행의 `ORDR-iPad-simulator-smoke` 아티팩트를 실제 확인했다.

- `ordr-renderer.log`: 맵 등록 완료 (`R_REGISTER_MAP_COMPLETE`), 등록 시간 27,081ms.
- `ordr-client.log`: `CLIENT_ACTIVATED_LOADING_CLEARED` 기록. 실제 화면은 240초에도
  로딩 이미지이며, 관측된 한 번의 `CL_FRAME_BEFORE_DRAW` → `AFTER_DRAW`는 20,867ms.
- `ordr-server.log`: `SERVER_FRAME_COMPLETE`는 8번째 게임 프레임(800ms)까지 기록.
- `ordr-jass.log`: 2,000ms의 `BB4` 코루틴 ordinal 2가 `CUX_CONDITION_AFTER`까지
  도달하지만, 해당 `EVENT_AFTER_RESUME`/`FLOW_RETURN`/서버 프레임 완료는 기록되지 않음.
- 360·480초 스크린샷은 iPad 홈 화면. 이전 `crash-report-status.json`은 앱 관련
  보고서를 한 개 찾았으나 종료 종류를 분류하지 못했다. **JASS 무한 루프·메모리 부족·
  렌더러 크래시 중 어느 원인인지 아직 확정할 수 없다.**

당시 네이티브 호출 샘플러는 6~32번째 코루틴에만 활성화되었다. 실제 문제 코루틴은
두 번째이므로 `CUX` 직후 호출을 전혀 포착하지 못했다. 이를 2번째부터 활성화했고,
조건 평가기에 `CONDITION_ENTER` → `CONDITION_RESULT` → `CONDITION_ADVANCE`와
`CONDITION_LIST_DONE`의 제한된 추적을 추가했다. `CUX` 이후 최대 32개 조건만 추적하며
원본 JASS 실행 순서나 게임 동작은 변경하지 않는다.

시뮬레이터의 진단은 다음 명령으로 로컬에서도 재분석할 수 있다.

```bash
python3 tools/ordr/check_simulator_runtime.py /path/to/ORDR-iPad-simulator-smoke
python3 -m unittest discover -s tests -p 'test_ordr_simulator_runtime.py'
python3 -m unittest discover -s tools/ipad/tests -p 'test_crash_report_summary.py'
```

이 도구는 클라이언트 활성화·렌더러 등록과 `BB4` 코루틴의 반환 여부를 별개로
보고하고, 실제 게임 진행 검증은 항상 `false`로 유지한다. 시뮬레이터 실행 워크플로는
또한 **현재 앱 실행 이후**의 `.ips` 보고서에서 예외명·종료 네임스페이스·보고서 유형만
집계한다. 원시 크래시 보고서와 개인 로그는 아티팩트에 추가하지 않는다.

다음 로그 판독 순서: (1) `CONDITION_RESULT`가 없다면 CUX 반환 직후 검사 경로,
(2) `CONDITION_ADVANCE`가 있고 다음 `CONDITION_ENTER`가 없다면 리스트 이동,
(3) 다음 조건에 진입했으나 반환하지 않으면 그 함수의 `NATIVE_*` 및 콜체인,
(4) `CONDITION_LIST_DONE`이면 `BB4` 나머지 동기 호출과 코루틴 반환 경로.
별도로 `crash-report-status.json`의 종류와 클라이언트 프레임 소요 시간을 대조한다.
실제 난이도 화면 표시나 플레이 성공은 이 진단 패치만으로 주장하지 않는다.

## 2026-10-26 — `CV5` 진입 직후 산술 예외와 JASS 나눗셈 보호

이전 패치의 시뮬레이터 실행 [40315353749](https://github.com/cuteu2474-a11y/open-realm/actions/runs/40315353749)
아티팩트를 내려받아 로그·화면·종료 진단을 대조했다.

- 2,000ms의 `BB4` 이벤트 안에서 조건 `CUX`는 `TimerStart(CUW, 2000ms)` 이후
  `returns=0`으로 돌아왔고, 다음 조건 `CUl`도 반환했다. 세 번째 조건 `CV5`의
  `CONDITION_ENTER` 이후 `CONDITION_RESULT`와 `EVENT_AFTER_RESUME`가 없다.
- 종료 진단의 일치하는 보고서는 1개이고 `EXC_ARITHMETIC`, `SIGFPE`, `SIGNAL`
  유형이다. `CV5` 안의 정확한 표현식은 아직 확인하지 못했다.
- JASS 엔진의 정수 `__div`가 C의 `/` 연산자를 검사 없이 호출하는 문제를 찾았다.
  0으로 나눗셈 및 `INT32_MIN / -1`을 최소 JASS 프로그램으로 실행하면 수정 전
  두 경우 모두 Linux에서 종료 코드 136 (`SIGFPE`)로 재현됐다. 따라서 이 코드가
  iPad 종료 원인이라는 설명은 **강한 추정**이지만, `CV5`의 실제 피연산자와
  호출 지점은 수정 후 시뮬레이터 로그로 확정해야 한다.
- 두 가지 오류 입력을 JASS 런타임 오류로 보고해 현재 코루틴의 오류 경계에서
  중단하도록 바꿨다. `ARITHMETIC_ERROR`에 피연산자를 남기며, 임의로 0을 반환해
  원본 게임 규칙을 바꾸지 않는다. 유효 정수/실수 나눗셈은 이전대로 수행한다.
- 직접 만든 최소 JASS 실행기는 수정 후 두 경우 모두 종료 코드 0으로 실행됐고,
  각각 `integer division by zero`, `integer division overflow`를 정상 보고했다.
  JASS 맵 회귀 테스트 3종도 추가했으나 로컬에는 SDL2/EGL/GL 개발 라이브러리가
  없어 `make test-wc3-engine` 실행은 불가능했다. CI 결과와 다음 시뮬레이터의
  실제 진행은 별도로 확인해야 한다.
- 화면은 240초에도 로딩 중이고 360초에는 홈 화면이다. 렌더러 맵 등록은
  27,337ms, 관찰된 최장 클라이언트 draw 호출은 23,552ms였다. 게임 프레임은
  여전히 8번째(800ms)까지만 완료되었다. 렌더링 병목은 별도 과제로 남는다.

다음 확인: 새 로그에 `ARITHMETIC_ERROR`의 좌·우 피연산자와 `RUNTIME_ERROR`가
기록되는지 살피고, 해당 값이 원작 데이터·선행 네이티브 결과 중 어디에서 왔는지
추적한다. `BB4`와 난이도 선택 흐름이 완료되기 전에는 게임 동작 성공을 주장하지 않는다.

## 2026-10-26 — 실제 나눗셈 오류 확인 및 원인 표현식 추적

수정된 JASS 엔진을 사용한 시뮬레이터 실행
[40321683540](https://github.com/cuteu2474-a11y/open-realm/actions/runs/40321683540)의
진단 아티팩트와 240·360·480초 스크린샷을 대조했다.

- `BB4` 조건 `CV5` 안에서 `[ARITHMETIC_ERROR] left=360 right=0` 및
  `[RUNTIME_ERROR] integer division by zero`가 발생했다. 따라서 이전 `SIGFPE`의
  원인은 0으로 나누는 정수 연산임이 실제 실행 기록으로 확인됐다.
- `BB4`의 `[EVENT_AFTER_RESUME]`는 출력됐지만 이는 오류로 코루틴이 중단됐다는
  뜻이며 조건·액션 정상 완료가 아니다. 요약기의 상태명을
  `bb4_resume_aborted_runtime_error`로 분리했다.
- 이번 종료 진단에서 앱 크래시 보고서 0개였고, 앱은 480초에도 실행 중이었다.
  게임 화면은 열렸지만 대부분 물 지형과 기본 HUD만 보였고 난이도 선택창은 없었다.
  카메라 설정 미완료와 지형 렌더링 오류를 화면만으로 구분할 수 없다.
- 8번째 게임 프레임까지만 정상 완료됐고, 맵 등록 33,540ms 및 최장 클라이언트
  draw 21,462ms가 관찰됐다. 렌더링 역시 성능 병목이다.
- 다음 실행에는 `DIVIDE_SOURCE`를 추가해 나눗셈의 실제 분자·분모 **표현식
  식별자**를 제한된 길이로 기록한다. 원본 맵 텍스트나 문자열 리터럴은 로그에
  복사하지 않는다. `CV5`가 왜 분모 0을 받았는지 확인하기 전까지 임의로 값을
  대입하거나 원본 JASS 코드를 변경하지 않는다.

## 2026-10-27 — 원본 맵 대조로 `CV5` 0 나누기 근본 원인 확인

- 사용자가 제공한 원본 `ORDR_S2_2.323[C](6).w3x`의 `war3map.j`를 **읽기 전용**으로
  추출했다. 추출된 스크립트는 2,503,677바이트이며 저장소에 추가하지 않는다.
- `CV5`의 고리형 유닛 배치식은 실제로 `PolarProjectionBJ(It,500,360/vP*EK)`다.
  직전 `set vP=10`, `set EK=0`에서 Warcraft/JASS의 좌결합 규칙대로라면
  `(360 / 10) * 0 == 0`이며 0 나누기가 아니다.
- OpenRealm의 `parse_multiplicative_expression`이 우재귀를 사용하여
  `360 / (vP * EK)`로 **잘못 해석한 것이 직접 원인**이었다. 같은 우재귀는
  `BFg(Hv,Hw)`의 `Hv-Hv/Hw*Hw`(그리드 나머지 계산)와 연속 덧셈/뺄셈도
  왜곡한다. 변수 스코프나 게임 규칙에 임의 값을 대입할 문제가 아니었다.
- 파서를 같은 우선순위에서 왼쪽부터 결합하도록 수정했다. 별도로 JASS의
  `and`가 `or`보다 먼저 결합하도록 했고 단락 평가를 추가하여, 평가할 필요가
  없는 오른쪽 호출이 실행되지 않게 했다. 원본 맵 소스는 변경하지 않았다.
- `t_jass_map.c`에 원본 고리형 배치식·`BFg` 나머지·연속 계산·논리 연산
  우선순위·단락 평가 회귀 테스트를 추가했다. **시뮬레이터 재실행 결과와
  실제 난이도 화면·위습·비콘·전투 확인은 아직 없다.**
