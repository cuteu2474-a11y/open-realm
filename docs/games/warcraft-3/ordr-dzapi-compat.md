# ORDR DzAPI / FDF UI 호환 계약 (기술 참고)

**현재 완료·미완료와 우선순위는 [ORDR 인수인계](ordr-native-handoff.md)가 유일한 기준이다.** 이 문서는 OpenRealm이 제공하는 UI API와 미검증 계약을 설명하며, 실기기 정상 동작을 주장하지 않는다.

## 설계 원칙

- 원본 W3X의 29 Dz, 12 JN, 2 EX 등의 맵 선언 네이티브를 엔진 게임 모듈에서 처리한다. 이름이 등록된 것만으로 구현 완료는 아니다.
- Dz 프레임 핸들은 games/warcraft-3/game/api/api_ordr_singleplayer.h (ordr_frames 배열, FDF 클론)와 api_ordr_singleplayer_extra.h (anchor, event, focus, rendering)가 관리한다.
- 원본 .toc/.fdf는 **소유한 맵의 MPQ에서 읽고** 게임의 UI_EnsureFDF/클론을 사용한다. 프로젝트 저장소에 대체 이미지/원본 MPQ를 넣지 않는다.
- 프레임은 게임 서버가 LAYER_GAME_3 전용 Dz 계층을 직렬화하고, 클라이언트는 일반 svc_layout/OnClick로 렌더링한다. 점수판은 별도 계층으로 분리되어야 한다.
- FramePoint/absolute point 좌표는 원본 UI 가상 좌표를 유지한다. 실제 iPad 비율과 UI 캔버스 매핑이 어긋나면 엔진의 앵커/클리핑을 수정한다. 원본 화면 좌표를 임의로 덮지 않는다.

## 중요한 구현 계약

| 영역 | 엔진에서 연결된 처리 | 남은 검증 |
| --- | --- | --- |
| TOC/FDF | DzLoadToc; DzCreateFrame/ByTagName, FindByName, 자식/부모 계층 및 템플릿 상태 | 실제 ORDR 임포트가 모두 표시되는지 |
| 프레임 속성 | Show, Enable, SetText, SetSize, SetAlpha, Point/AbsolutePoint/ClearAllPoints, SetFont, SetTextAlignment, SetTexture/SetModel | iPad 텍스트 겹침, 아트 경로/모델, 스케일·앵커 |
| 우선순위 | Priority 기준 형제 프레임 렌더 순서; 일반 HUD와 별도 Dz 레이어 | iPad 클릭 hit-test와 z-order 일치 |
| 이벤트 | SetScriptByCode(1~10) 및 OnClick/uievent 매핑; list/pop-up index와 edit text(9) 처리를 유효한 프레임에 한정 | 실제 iPad 터치·체크박스·스크롤바·텍스트 편집 |
| 키보드 | cl_game_keycmd/dzkey로 실제 물리 키 down/up 전달 | 가상 iPad 키보드/IME 및 복합 키 동작 |
| 싱글플레이 | 로컬 JASS UI 이벤트 컨텍스트, 원본 오프라인 시작 흐름 | 원본 건물·상점·도박·도움소의 모든 액션 |

## 터치 → JASS 연결을 증명하는 방법

1. 원본 데이터에서 선택된 시설이 생성되었는지 확인. H0C4/H05M은 모델상 건물이나 일반 유닛으로 분류될 수 있다.
2. 서버의 G_GetCommandButtons 또는 DzFrameSetScriptByCode가 **활성·가시 버튼**을 실제 생성했는지 확인.
3. 클라이언트 FINGERDOWN/UP 또는 UILayout 이벤트가 해당 버튼을 정확히 hit-test했는지 확인.
4. 네트워크 button/research/upgrade 또는 uievent가 서버에서 해당 객체·플레이어로 검증된 후 원본 JASS에 전달되는지 확인.
5. 버튼의 실제 구매/제작/스킬/보상/자원 변화까지 확인. **명령 접수만으로 성공 처리하지 않는다.**

## 알려진 위험과 금지할 편법

- UI 텍스트가 좌상단에서 겹치는 현상은 현재 미해결. 긴 메시지 영역을 키운 코드가 있지만 Dz/FDF의 실제 레이아웃/폰트/클리핑과 분리 검증 필요.
- 기존 문서의 'UI 완성', '29개 등록', '헤드리스 콜백 통과' 같은 기록은 **원랜디 실기기 버튼·아트·보상 완료 증거가 아니다**.
- 엔진 네이티브 중 TriggerExecuteWait 등의 함수는 아직 완전한 대기/코루틴 호환이 아니며, 원본과 다른 부분이 있을 수 있다.
- 임의의 건물 버튼·가짜 JASS 콜백·원작과 다른 재화/조합으로 겉보기 작동을 만들지 않는다.

**관련 소스:** games/warcraft-3/game/api/api_ordr_singleplayer.h, api_ordr_singleplayer_extra.h, hud/hud_unit.c, g_commands.c, hud/hud_cinematic.c; client/cl_input.c, cl_scrn.c.
