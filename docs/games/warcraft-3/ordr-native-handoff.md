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

## 다음 채팅에 붙여 넣을 문장

> `cuteu2474-a11y/open-realm`의 `feature/ordr-singleplayer-compat`에서 이어서 작업해주세요.
> 먼저 `docs/games/warcraft-3/ordr-native-handoff.md`와 `ordr-native-audit.json`을 읽고,
> 남은 원랜디 네이티브를 실제 동작과 회귀 테스트까지 구현해주세요. 등록/빈 본문을 완료로
> 세지 말고 원본 선택 이벤트 경로를 유지해주세요. 워크플로를 수정마다 시작하거나 취소하지 마세요.
