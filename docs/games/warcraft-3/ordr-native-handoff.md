# ORDR S2 2.323[C] — iPad OpenRealm 개발 인수인계

**최종 확인: 2026-10-28 (Asia/Seoul)**  
**작업 저장소:** https://github.com/cuteu2474-a11y/open-realm  
**유일한 작업 브랜치:** feature/ordr-singleplayer-compat  
**기준 엔진 커밋:** 8e09614543e1513e4a440b827789fdf5367bc3aa

> **이 문서가 ORDR iPad 포팅의 단일 현황·TODO·인수인계 기준이다.** 역사적인 커밋이나 개별 기능 설계 문서는 상세 근거이지 현재 완료 판정이 아니다. 다른 프로젝트인 Unity 원랜디 모바일 리메이크 및 Android GrandLineDefense와 혼동하지 않는다.

## 2026-10-28 추가 개발 및 검증 필요 항목

- 플레이어 6 몬스터 이동은 이전 실기기 로그에서 좌표 변화가 확인됨. 사망 이벤트의 플레이어 슬롯 매칭 오류는 커밋 1595288에서 수정했으나 카운트 화면 결과는 아직 실기기 검증이 필요함.
- JASS의 AddItemToStock/AddUnitToStock 및 재고 슬롯 변경 후 선택 중인 상점 명령 카드가 갱신되도록 무효화 경로를 보완함. 상점 상품과 건물 자체 능력의 명령 버튼이 동일 칸에 겹치지 않도록 4x3 슬롯을 재배치함. 원작 스톡·구매 비용·능력 규칙 자체는 변경하지 않음.
- ORDR_BUILDING_COMMAND_CARD의 0은 상점 상품 버튼을 포함하지 않음. 새 ORDR_BUILDING_FINAL_CARD 로그의 final_count가 실제 최종 서버 카드 기준이며, 버튼 터치·보상 효과는 별도로 확인해야 함.
- iPad 체력바는 픽셀 단위 좌표 변환을 수정했으나 실기기 검증 대기. 좌상단 글씨 겹침과 일부 건물 실제 보상 기능은 미해결.

## 2026-10-29 실제 iPad 스크린샷 — 미완료 검증 항목

- 금색 채팅 버튼이 **보이지 않는 이전 빌드 화면**이 보고됨. 패키징의 CFBundleVersion이 고정 1이던 문제를 수정하고 터치 버튼 위치를 재조정했음. 실제 채팅창과 iPad 키보드의 성공 여부는 새 IPA 설치 후 검증할 것.
- 도박소 "돈 도박 하급(Q)"에서 `No eligible purchaser is nearby.`가 표시됨. 엔진의 일반 중립상점 "인벤토리를 가진 구매자 근접" 조건이 적용된 것이 직접적인 원인. 원작 맵의 도박소 거래는 일반 상점인지 JASS 판매 이벤트인지 확인할 때까지 **임의로 재화나 보상을 생성하지 말 것**.
- 오른쪽 명령 카드에 보라색 체크무늬 이미지가 여러 개 보임. 비활성화 아이콘을 존재하지 않는 DIS 이미지로 치환하던 코드는 보정했지만 원본 BTN 이미지 누락이나 경로 해석은 추가 검증이 필요함.
- 좌상단 Quest 바 글씨 및 스코어보드 열 간 글씨가 겹침. 다국어 가변 길이와 실제 텍스트 측정에 맞게 ORDR score column 크기를 보정함.
- 몬스터 카운트, 건물 구매/도박 실효과, 비콘 및 개별 유닛 체력바는 **새 IPA에 대한 실기기 근거가 없음**. 정상 판정하지 말 것.

## 2026-10-29 통합 수정 묶음 및 마지막 검증

- `63f4690`: IPA `CFBundleVersion`을 Actions run number로 구분; 독립 iPad CHAT 버튼을 기존 상단 크롬 밖으로 이동하고 손가락 터치 보완 및 서버 chat_open 수신 로그 추가; ORDR 비활성 아이콘은 누락 DIS 파일 대신 원본 아트에 회색 틴트 사용.
- `7b94a27`: ORDR 멀티보드의 한국어/긴 셀 문자열을 실제 표시 길이로 측정하여 겹치는 열 너비 보정.
- `b197c93`: `TriggerRegisterPlayerUnitEvent`에서 원본 JASS 필터 인자 등록/실행을 복구, 플레이어 6 사망 이벤트 필터 회귀 테스트 추가. 월드 유닛/원본 JASS 수정 없음.
- `04c83ef`: 퀘스트 F9 버튼을 가리는 ORDR의 손상된 QuestChangedParticles 장식만 숨기고 실제 F9 버튼/퀘스트 기능은 유지.
- 사용자 2026-10-29 스크린샷의 도박소 "No eligible purchaser is nearby."는 일반 상점의 근접 인벤토리 구매자 판정 경로 때문임. 원본 도박 JASS의 판매 콜백 및 소비재 처리와 대조되지 않아 **재화를 임의로 지급하거나 인벤토리 조건을 전체 상점에서 해제하지 않음**. 실기기 기능은 여전히 미검증.
- **최종 검증**: 후속 push에 `[ipad-test] [ipad-build]`를 한번만 넣어 기존 1회 iPad 시뮬레이터 + IPA + 일반 CI 실행. 통과 시에도 iPad 실기기 입력·구매·카운트 실효과 검증 필요.

## 2026-10-30 건물·상점 기능 원본 데이터 바인딩 수정

- 상점/생산 명령카드는 `UnitFunc.ini`의 `FindConfigValue`만으로 생성되지 않는다. 원본 W3U에 정의된 사용자 유닛은 `G_UnitProfile(unit_id)`의 `Art`, `Tip`, `Ubertip`, `Hotkey`, `ButtonposX/Y`에 올바른 정보가 있어도 이전 코드가 보지 않아 `G_BuildCommandButton`이 버튼을 생략했다. 해당 유닛 정보가 우선하도록 수정. 능력·아이템·시스템 명령은 기존 경로 유지.
- 모델 파일이 없는 원작 아이템은 `ItemData`가 존재하면 상점 재고/아이템 핸들/충전량이 유효하다. 이전 `G_InitItemStock`, `G_ShopPurchaseItem`, `SP_SpawnItem`의 모델 필수 조건을 제거하여 실제 인벤토리·판매 이벤트가 실행될 수 있도록 보완. 유효하지 않은 아이템 ID는 차단 유지.
- 회귀: 커스텀 유닛 명령 버튼 원본 아트·핫키, 모델 없는 아이템 생성·핸들·충전량, 모델 없는 원본 아이템의 상점 재고 초기화.
- 기존 건물 자체 기능과 JASS 판매 보상 결과는 iPad 실기기에서 아직 미검증. 임의 보상, 원본 맵, 전투 규칙은 변경하지 않았다.

## 2026-10-30 유닛 카운트/능력 코드 경로 추가 점검

- **실기기 재현:** 몬스터 사망 뒤에도 카운트 감소가 나타나지 않는다는 사용자 보고가 계속됨. 이전 수정이 문제를 해결했다는 증거는 없음.
- 엔진의 `GetUnitAbilityLevel`은 ORDR의 숫자형 big-endian rawcode를 이미 보정하나, `UnitAddAbility`는 보정 없이 `G_ActorAddSkill`에 전달했음. 신규 표지 능력을 추가하는 순간에는 기존 소유 능력이 없어 조회 기반 보정도 불가능함.
- 원본 맵/JASS를 수정하지 않고 `api_unit.h`의 추가 경로가 AbilityData 기반으로 숫자형 능력 코드를 해석하고, ORDR의 `A901/A902` 표지 rawcode는 기존 ORDR 스크립트 식별 시에만 동일하게 해석하도록 보완함. 제거/영구화도 기존 소유 기반 보정을 재사용함.
- 디버그 로그 `ORDR_WAVE_MARKER_ADD`는 `requested`, `ability`, `added`, `level`을 출력한다. `ORDR_WAVE_DEATH_QUEUED` 및 `ORDR_WAVE_DEATH_DISPATCH`와 함께 비교하여 사망 시 원작 트리거가 실제로 표지를 확인할 수 있는지 판정한다.
- 회귀 테스트 `wc3_api.ordr_numeric_added_ability_survives_owner_six_death_event`는 플레이어 6, 숫자형 능력 삽입, 사망 이벤트 및 JASS 조건 카운트 변경을 연쇄 검증한다. 테스트에 쓰는 `AInv`는 stock fixture의 유효 능력이고 원작 표지 `A901` 자체는 비공개 원본 맵에서만 실기기 검증 가능하다.
- 카운트 변수 하드코딩/무조건 감소 금지. 테스트 성공이라도 실제 ORDR 실기기 카운트 갱신은 확인 전까지 **미검증**.

## 2026-10-30 도박소 판매 이벤트 원본 검증 및 엔진 복원

원본 사용자 소유의 `ORDR_S2_2.323[C](3).w3x`에서 **war3map.j 및 Units/UnitBalance.slk**를 읽어 다음을 확인했다. 원본은 절대 변경하지 않는다.

- `CJD`가 `BC5(KF, $68303646, function CJB)` 및 `BC5(KF, $4830415A, function CJC)`를 등록한다. 이 rawcode는 각각 `h06F`(골드 10)와 `H0AZ`(골드 500)이다.
- `BDF`는 `EVENT_PLAYER_UNIT_SELL` 등록, `BDE`는 `GetUnitTypeId(GetSoldUnit())`으로 등록된 기능을 호출한다. `CJB`는 임시 판매 유닛을 지우고 원본 확률에 따라 10~50 혹은 3~5 골드를 지급한다. `CJC`는 500~4,500 혹은 300~400 골드를 지급하고 원작의 별도 누적치/업그레이드도 처리한다.
- 기존 엔진 `G_GetShopUnitButtons`/`G_ShopPurchaseUnit`가 **플레이어 소유 건물에도 근접 구매자 조건을 적용**했고, 성공 시에도 **SELL 이벤트를 전혀 발송하지 않음**. 원본 판매 트리거 실행 불가가 직접 원인이다.
- 수정: 플레이어 소유 건물의 유닛 상품은 근접 구매자가 필요 없고, 중립 상점은 기존 반경 조건 유지. 성공 시 판매 이벤트 `EVENT_UNIT_SELL`/`EVENT_PLAYER_UNIT_SELL`, 구매자 player 및 판매 유닛 ID 문맥 제공. 기존 아이템 판매 이벤트도 인스턴스별 판매 ID를 제공한다.
- 이전 진단에서 `h06Q`, `h06R`은 UI 원본 능력이 각각 `AInv,Avul`/ `Avul`이고 **원본 설정에 명령 카드 능력이 없음**. 이 두 건물에 임의 커맨드를 추가하지 말 것. `h07A`에는 동적 유닛 재고용 `Asud`가 있음.
- **미확인**: 각 건물의 화면 아이콘·버튼 터치, 원작의 판매 이벤트 후 결과는 실기기 검증 필요. 컴파일/회귀 통과는 최종 게임 검증과 다르다.

## 1. 목표와 절대 제약

- 원작 ORDR S2 2.323[C]를 OpenRealm 엔진에서 **iPad 싱글플레이**로 실행한다. 원작 게임 규칙·유닛 스펙·맵 동작을 임의로 바꾸지 않는다.
- **원본 W3X 및 war3map.j 수정 금지.** 원인과 수정 지점은 OpenRealm의 C/JASS VM, 게임 모듈, 렌더러, 일반 입력 처리 및 엔진 자산 바인딩이다.
- 오프라인 모드를 기본으로 유지한다. 멀티플레이/온라인 기능을 새로 만들지 않는다.
- 브랜치 신규 생성, 워크플로 신규 생성, 소규모 수정마다 시뮬레이터·IPA 재실행, 실행 후 반복 취소를 하지 않는다. 기존 브랜치에서 **문제들을 묶어 수정 → 단위/CI 검사 → 마지막에 필요 시 1회 시뮬레이터 → 필요할 때 IPA** 순서로 처리한다.
- 실제 몹 카운트나 이동을 임의로 조작하지 않는다. 원작 JASS 이벤트 및 소유자, 주문·길찾기, 명령 카드, 데이터 테이블을 복원한다.
- "컴파일 성공", "CI 성공", "명령 접수", "화면에 모션 있음"은 각각 **게임 정상 동작을 증명하지 않는다**. 아래 표의 검증 수준을 엄격히 구분한다.

## 2. 2026-10-28 현재 관찰된 상태

| 대상 | 확인된 사실 | 판정 |
| --- | --- | --- |
| iPad 기기 실행·원본 맵 로딩 | 실기기에서 실제 게임이 실행되고 유닛·건물·공격 모션이 보였다는 사용자 보고. 이전 시뮬레이터에서도 난이도·모드 선택 후 게임 화면에 진입함 | **부분 성공** |
| 난이도/이세계 선택 | 이전 시뮬레이터에서 자동 선택·시작 명령 수락 기록 | **시작 흐름 확인, 전 게임 미검증** |
| 전투 및 피해 | 2026-10-28 실기기 로그에서 적 HP 0 및 사망이 관찰되었음. 공격 시점 0 처리도 이전 커밋에서 수정 | **피해/일부 처치 확인, 모든 유닛·스킬 미검증** |
| **적 이동** | 사용자가 적이 정지해 있다고 반복 보고. 과거 추적기가 잘못된 플레이어 5를 보던 문제가 있음. 실제 몬스터는 **플레이어 6** | **미해결 — P0** |
| **죽은 몬스터 수 카운트** | 플레이어 6 소속 몬스터 사망 기록이 있는데 게임 UI의 카운트가 줄지 않는다는 실기기 보고 | **미해결 — P0** |
| **건물 기능** | 건물 선택은 일부 가능, 명령 카드가 0개인 시설도 존재. 일부 버튼의 서버 명령 접수만 확인. 실제 구매/도박/조합/도움소 보상은 확인되지 않음 | **미해결 — P0** |
| **좌상단 글자 겹침** | 실기기에서 겹쳐 보인다는 사용자 보고. 메시지 영역 크기 변경 코드가 있으나 최신 결과 사진과 대조되지 않음 | **미해결 — P1** |
| 위습 및 비콘 | 위습 선택/이동과 e0IX → CH → CRy 보상 흐름의 실제 완주가 입증되지 않음 | **미검증 — P1** |
| 지형·물·아이콘 | 물/지형 표현, 일부 폰트/이모지와 UI 배치 문제가 시뮬레이터/실기기에서 반복 보고됨 | **미해결 또는 검증 부족 — P1** |
| 성능 | 과거 시뮬레이터는 약 30분 관찰에 게임 시간 약 41.5초로 매우 느렸음. 실기기 성능과 혼동하지 말 것 | **시뮬레이터 병목 — P1** |

### 최근 수정 코드와 그 한계

- **8e09614 (2026-10-28):** JASS로 생성한 유닛이 첫 웨이포인트 영역 안에서 나타났을 때 **최초 진입(region enter) 이벤트**가 발생하도록 수정. 플레이어 6의 이동 관찰·사망 이벤트 큐/디스패치·A901/A902 표지 추적을 추가. 건물처럼 생긴 일반 유닛의 Birth 모션 중 명령 카드 차단 조건을 완화. 긴 메시지 영역 확장. **코드/일반 CI 통과이나 이 커밋의 새 실기기·시뮬레이터 검증은 없음.**
- **ae5a592 (2026-10-28 이전):** 공격 damagePoint=0 시 최소 한 게임 프레임을 예약해 타격이 유실되지 않도록 함. 기존 양수 damagePoint는 보존. 공격 명중/실제 HP 변화/회피·무적 원인을 별도 기록. 일반 CI 통과.
- **f92f676:** 소스 변경 없는 [ipad-build] IPA 빌드 트리거 커밋. **8e09614보다 오래된 바이너리**이므로 최신 지역진입·사망·건물 카드 수정은 포함하지 않음.
- 모델이 없는 유닛의 생성·상점 재고 처리와 ORDR 특수 시설 H0C4/H05M의 터치/진단 경로 등은 앞선 커밋에서 수정했지만, 기능의 최종 효과까지 입증한 것은 아님.

## 3. 검증된 GitHub Actions 결과 — 이력과 범위

| 종류 | 커밋 | 실행 결과 | 증명하지 못하는 것 |
| --- | --- | --- | --- |
| **최신 일반 CI** | 8e09614 | [40613218799 — Success](https://github.com/cuteu2474-a11y/open-realm/actions/runs/40613218799) | 실제 iPad 몹 이동·카운트·건물 기능 |
| **최근 SideStore IPA** | f92f676 | [40608245689 — Success](https://github.com/cuteu2474-a11y/open-realm/actions/runs/40608245689) | **8e09614 수정은 들어 있지 않음** |
| **마지막 실제 iPad 시뮬레이터 관찰** | c9c5049 | [40554917815 — Success](https://github.com/cuteu2474-a11y/open-realm/actions/runs/40554917815) | 첫 라운드 후 정상 이동·건물 버튼 적용. Green은 앱 빌드/실행/캡처의 성공 |
| **8e09614 IPA/시뮬레이터** | 8e09614 | Push 워크플로 **skipped** (실행 조건 없음) | 새 수정의 실제 플레이 결과 없음 |

**운영:** 일반 CI는 PR 변경에 따라 실행된다. iPad 시뮬레이터는 기존 ordr-ipad-simulator.yml의 [ipad-test]가 명시된 push에서만 실행되고, IPA는 기존 ordr-ipad-build.yml에서 [ipad-build] 또는 workflow_dispatch로 실행된다. 평범한 문서 커밋으로 iPad 실행을 강제로 유발하지 않는다. 기존 Actions를 취소·재실행하거나 새로운 Actions 파일을 만들지 않는다.

## 4. 다음 AI/개발자가 해야 할 일 — 우선순위

### P0-A. 적 생성 이후 실제 이동

1. 원본 스크립트에서 **플레이어 6** 몬스터의 최초 스폰 영역 진입 → 웨이포인트 지역 이벤트 → IssuePointOrder/GroupPointOrder → 유닛 AI/경로 계산으로 이어지는 호출을 추적한다.
2. 최신 8e09614의 G_UnitSpawnedRegionEvents가 이벤트를 실제 등록/디스패치하고 있는지, order 문자열·목표 좌표·accepted·pathing 및 매 게임틱 위치 변화까지 입증한다.
3. 진입 이벤트가 누락되면 **엔진 트리거 등록/스폰 이벤트** 수정, 주문이 거부되면 **명령/제어권 라우팅** 수정, 주문 수락 후 제자리면 **길찾기·지면 충돌** 수정. 몬스터에게 강제 이동/텔레포트를 넣어 증상을 감추지 않는다.

### P0-B. 몬스터 처치 수 갱신

1. 실제 **플레이어 6** 유닛의 HP 0 → unit_die → EVENT_PLAYER_UNIT_DEATH → 트리거 조건/액션 → ORDR 변수·그룹 → UI 텍스트 갱신으로 이어지는 경로 확인.
2. 8e09614의 ORDR_WAVE_DEATH_QUEUED / ORDR_WAVE_DEATH_DISPATCH 로그에서 registration/owner_matched 및 A901/A902 표지를 비교. 로그에 누락된 구간을 추가하면 되는지 검토.
3. 카운트는 원본 맵이 계산하도록 유지. 사망 시 숫자를 직접 1 빼는 하드코딩/표시만 속이는 패치 금지. 사망 이벤트 중첩·그룹 생존 필터와 실제 목표 대조.

### P0-C. 건물 기능과 버튼

1. H0C4, H05M과 사용자가 터치한 상점/도박/조합/도움소 시설을 각각 조사. **외형만 건물이고 실제 UnitData 분류는 일반 유닛일 수 있음**.
2. 건물 선택 → G_GetCommandButtons의 명령 수/슬롯/아트/활성화 상태 → 실제 HUD 프레임 → 손가락 터치 → CLIENTCOMMAND(Button/Research/Upgrade) 또는 Dz UI 콜백 → 자원/유닛/아이템/보상 변화까지 각 단계 분리.
3. 명령 카드가 0개면 원본 UnitAbilities/UnitProfile, Birth 차단, 스킬 구현 매핑, missing art 스킵을 확인. 명령이 수신되더라도 효과가 없으면 이벤트/업적/인벤토리/판매 로직을 원본에 맞춰 연결. 임의로 새 버튼/재화를 생성하지 않는다.

### P1. UI/성능/위습

- **좌상단 글자:** hud_cinematic.c의 LAYER_MESSAGE 및 Dz/FDF, multiboard, leaderboard의 실제 화면 좌표·클리핑·우선순위 확인. 8e09614의 길이별 메시지 영역 확장은 임시 수단일 뿐 스크린샷으로 효과 검증 필요. 원작 텍스트를 일괄 삭제하지 않는다.
- **위습 비콘:** e0IX가 CH(Rect -3584,-9504,-3328,-9216)에 정상 진입하면 CRy가 원작대로 소비·보상 처리하는지 실기기 확인.
- **시뮬레이터 성능:** 시간이 30분 흘러도 라운드 시작 이후까지 가지 못할 수 있음. 캡처 길이를 늘리는 것만으로 해결하지 말고 서버 frame 진척, 게임 렌더 시간/terrain/path 워크로드를 구분하여 병목을 줄인다.
- 물/지형, emoji/아이콘, 점수판 레이어는 원본 데이터와 실제 렌더링을 비교하여 수정.

## 5. 주요 코드 경로

| 기능 | 주된 파일 |
| --- | --- |
| 게임 루프, ORDR 상태·적 이동 진단 | games/warcraft-3/game/g_main.c |
| 지역 enter/leave·트리거 디스패치·사망 이벤트 | games/warcraft-3/game/g_events.c |
| JASS CreateUnit/죽음/주문·상태 | games/warcraft-3/game/m_unit.c, games/warcraft-3/game/api/api_unit.h |
| JASS 이벤트/코루틴 의미 | games/warcraft-3/jass/jdo.c, games/warcraft-3/game/api/api_trigger.h |
| 물리/길찾기/전투 | games/warcraft-3/game/g_monster.c, g_phys.c, skills/s_move.c, skills/s_attack.c |
| 명령 카드 생성·스톡 버튼/아트 | games/warcraft-3/game/hud/hud_unit.c, games/warcraft-3/game/g_stock.c |
| 버튼 입력/구매/연구·강화/건물 선택 | games/warcraft-3/game/g_commands.c, games/warcraft-3/game/g_building.c |
| Dz/FDF UI 스크립트/이벤트 | games/warcraft-3/game/api/api_ordr_singleplayer.h, api_ordr_singleplayer_extra.h |
| 좌상단 메시지/다른 UI 레이어 | games/warcraft-3/game/hud/hud_cinematic.c, hud_multiboard.c, hud_leaderboard.c |
| iPad 터치·선택/시뮬레이터 입력 | client/cl_input.c, client/cl_main.c, client/cl_scrn.c |
| ORDR 시뮬레이터 증거 분석 | tools/ordr/check_simulator_runtime.py, tests/test_ordr_simulator_runtime.py |
| IPA·시뮬레이터 기존 워크플로 | .github/workflows/ordr-ipad-build.yml, .github/workflows/ordr-ipad-simulator.yml |
| iPad 패키징/파일 복사 | packaging/ipad/README.md |

## 6. 다음 검사 절차 / 통과 조건

1. **먼저 코드를 묶어서 수정**, 기존 헤드리스 회귀 테스트 추가. make test 및 Python 진단 테스트, CI를 확인. 실패할 경우 CI 오류를 먼저 해결.
2. iPad 시뮬레이터 1회로 **실제로 라운드가 시작된 이후** 플레이어 6 몬스터의 위치가 시간에 따라 바뀌는지 확인. 단순 몹 스폰이나 명령 accepted만으로 성공 판정 금지.
3. 몹을 처치해 HP가 0이 되는 것과 **생존 유닛 카운트가 실제로 줄어드는 것**을 함께 확인. 스코어/원본 변수 결과로 판정.
4. 최소 2~3개 시설을 각각 눌러 버튼 표시/명령 수신/**실제 자원 및 보상 변화**를 확인. 기능마다 별도 PASS/FAIL.
5. 좌상단 및 점수판 스크린샷, 위습 비콘 진입과 보상, 물/지형/아이콘을 기록. 성능 로그는 실제 게임 시간과 wall-clock을 분리.
6. 최종적으로 필요한 경우 최신 HEAD를 SideStore용 IPA로 만들고 실기기 검사. 원본 게임 데이터는 GitHub에 업로드하지 않는다.

**사용자에게 요청할 재현 자료:** iPad의 Files → On My iPad → OpenWarcraft3 → ordr-jass.log (앱 재실행 시 덮어쓸 수 있음), 첫 웨이브/건물 터치 전후 영상 또는 스크린샷, 건물 이름/누른 버튼, 카운트의 전후 값. 비밀번호/토큰/비공개 MPQ는 첨부·커밋하지 않는다.

## 7. 검증 수준과 참고 문서

- 코드 작성/등록 ≠ 구현 ≠ headless 테스트 ≠ iPad 실제 작동. 항상 마지막 검증 범위를 명시.
- 원본 JASS의 자세한 Dz/FDF 기술 계약은 [ordr-dzapi-compat.md](ordr-dzapi-compat.md), private asset 준비는 [ordr-simulator-private-assets.md](ordr-simulator-private-assets.md) 참고.
- 엔진 전반 상세 정보는 [AGENTS.md](../../../AGENTS.md), [jass-native-coverage.md](../../../games/warcraft-3/jass-native-coverage.md), [pathfinding.md](pathfinding.md), [trigger-events.md](trigger-events.md) 참고.
- 오래된 진행 일지/완료 주장들은 **이 문서의 판정을 덮어쓰지 않는다**. 과거 내용을 확인해야 한다면 Git 커밋 기록을 참고한다.
