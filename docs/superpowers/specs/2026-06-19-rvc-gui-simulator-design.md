# RVC GUI 시뮬레이터 설계

작성일: 2026-06-19

## 목표

기존 스크립트형 콘솔 시뮬레이터를 `cpp/simul` 아래의 새 시뮬레이터로 대체한다. 새 시뮬레이터는 Windows GUI에서 사진과 동일한 10x10 보드를 표시하고, 각 시뮬레이션을 수동 또는 자동으로 한 단계씩 진행하며, 모든 시나리오의 PASS/FAIL 여부를 확인할 수 있어야 한다.

시뮬레이터는 기존 30개 시나리오 전체와 제공된 보드 이미지 기반 신규 시나리오 1개를 포함한다.

## 현재 맥락

현재 활성 시뮬레이터는 `cpp/simulator/rvc_simulator.cpp`이다. 이 파일은 `rvc::RvcSoftwareController`에 30개 스크립트 시나리오를 입력하고, 기록된 motion command 및 cleaning command를 기대 command 목록과 비교한다.

CMake target 이름은 현재 `rvc_simulator`이며, Ubuntu CI에서도 이 target을 빌드하고 실행한다. 새 구현은 기존 스크립트와 CI가 계속 동작하도록 target 이름을 그대로 유지한다.

사용자가 요청한 새 위치는 `cpp/simul`이다. 기존 `cpp/simulator` 구현은 새 `cpp/simul` 기반 target source로 대체한다.

## 선택한 방식

C++17 기반으로 플랫폼을 분리한다.

- Windows 빌드: Win32/GDI GUI 실행 파일
- 비-Windows 빌드: 동일 시나리오 엔진을 사용하는 콘솔 runner

이 방식은 Windows에서 별도 GUI 프레임워크 없이 그래픽 창을 제공한다. Qt, SDL, SFML 같은 외부 GUI 의존성을 추가하지 않으므로 빌드 환경이 단순하고, Ubuntu CI에서는 display server 없이도 `rvc_simulator`를 계속 빌드하고 실행할 수 있다.

## 보드 그래픽

GUI 보드는 제공된 기준 이미지와 같은 10x10 격자이다.

- 진회색 셀은 장애물이며 이미지의 한글 장애물 라벨을 표시한다.
- 노란 셀은 먼지이며 이미지의 한글 먼지 라벨을 표시한다.
- 밝은 회색 셀은 이동 가능한 바닥이다.
- 로봇은 10행 2열에서 시작하며, 초록색 강조 셀과 숫자 `1`로 표시한다.

승인된 보드 배치는 다음과 같다.

```text
Row 01: W W W W W W W W W W
Row 02: W D . . . . . . D W
Row 03: W D W W W W W W . W
Row 04: W . W . . . . W . W
Row 05: W . W . W W . W . W
Row 06: W . W . W W . W . W
Row 07: W . W . W D D W . W
Row 08: W . W . W W W W . W
Row 09: W . W D D . . . D W
Row 10: W R W W W W W W W W
```

범례:

- `W`: 장애물 또는 벽 셀
- `D`: 먼지 셀
- `R`: 로봇 시작 셀
- `.`: 빈 바닥 셀

## 시뮬레이터 모델

시뮬레이터 코어와 표시 계층을 분리한다.

- 시나리오 정의는 action 목록, 기대 motion command, 기대 cleaning command, 선택적 보드 이벤트, note를 가진다.
- 시나리오 runner는 각 시나리오마다 새 `RvcSoftwareController`를 생성한다.
- 기록용 sink는 motion command와 cleaning command를 캡처한다.
- 각 step은 action 하나를 적용하고, 새로 발생한 command를 캡처하며, 시나리오 상태를 갱신한다.
- 최종 PASS/FAIL은 실제 command trace와 기대 command trace를 비교해서 계산한다.

이 구조를 사용하면 GUI 수동 진행이 결정적으로 동작하고, 콘솔 runner도 같은 동작을 공유할 수 있다.

## 시나리오 범위

새 시뮬레이터는 다음을 포함한다.

- 기존 `TC-01`부터 `TC-30`까지의 모든 action sequence와 기대 command trace
- 승인된 보드 이미지를 기반으로 한 신규 `TC-31`

`TC-31`은 사진과 동일한 보드를 시각 상태로 사용하고, 장애물 및 먼지 셀이 포함된 경로를 따라가는 신규 테스트이다. 이 시나리오도 기존 시나리오와 동일하게 PASS/FAIL을 보고하며, GUI에서 수동 진행과 자동 진행을 모두 지원한다.

## Windows GUI 동작

Windows GUI는 다음 요소를 포함한다.

- 사진과 동일한 10x10 보드
- 31개 전체 시나리오 선택 UI
- 선택된 시나리오의 PASS/FAIL 상태와 전체 실행 요약
- 현재 step 번호와 action 이름
- 기대 motion command trace와 실제 motion command trace
- 기대 cleaning command trace와 실제 cleaning command trace
- `Previous`, `Next`, `Auto`, `Reset`, `Run All` 컨트롤

수동 진행은 action 하나씩 전진한다. 자동 진행은 타이머를 사용해 선택된 시나리오를 순서대로 진행한다. `Run All`은 모든 시나리오를 끝까지 실행하고 PASS/FAIL 요약을 갱신한다.

보드 renderer는 고정된 셀 크기, 일관된 색상, 중앙 정렬 라벨을 사용해 제공된 이미지와 최대한 동일하게 보이도록 한다. GUI는 일반 데스크톱 창 크기에서 별도 리사이징 없이 사용할 수 있어야 한다.

## 콘솔 fallback 동작

비-Windows 빌드에서는 같은 `rvc_simulator` target이 콘솔 실행 파일로 컴파일된다. 콘솔 runner는 31개 전체 시나리오를 실행하고, PASS/FAIL 줄을 출력하며, `--verbose`를 지원한다. 모든 시나리오가 통과할 때만 exit code `0`을 반환한다.

이 경로는 CI용이며, `.github/workflows/cpp-ci.yml`이 기대하는 동작을 유지한다.

## 빌드 및 스크립트 연동

`cpp/CMakeLists.txt`는 simulator source path를 기존 `simulator/rvc_simulator.cpp`에서 새 `simul` 구현으로 변경한다. 실행 파일 target 이름은 `rvc_simulator`로 유지한다.

기존 실행 스크립트는 계속 유효해야 한다.

- `cpp/run-simulator.ps1`
- `cpp/run-simulator.bat`
- `2nd ooad/06_simulator/run-simulator.ps1`
- `2nd ooad/06_simulator/run-simulator.bat`

`cpp/README.md`와 `2nd ooad/06_simulator/RVC_Simulator.md`는 새 GUI/fallback 동작과 31개 시나리오 구성을 설명하도록 갱신한다.

## 오류 처리

시나리오 action이 기대하지 않은 command를 발생시키면 결과는 FAIL이며, GUI는 기대 trace와 실제 trace를 함께 표시한다.

사용자가 자동 진행 중 다른 시나리오를 선택하면 자동 진행을 중지하고 선택된 시나리오를 초기 상태로 되돌린다.

`Previous`를 사용할 때는 선택된 시나리오를 처음부터 요청 step 직전까지 다시 실행해 상태를 재구성한다. 이렇게 하면 mutable rollback 버그를 피하고 시뮬레이션을 결정적으로 유지할 수 있다.

## 테스트 전략

테스트는 플랫폼 중립 시뮬레이터 코어에 집중한다.

- 승인된 기준 보드가 정확히 10행 10열인지 검증한다.
- 승인된 보드의 시작 셀, 장애물 셀, 먼지 셀이 기대 위치에 있는지 검증한다.
- 전체 31개 시나리오가 존재하는지 검증한다.
- 시나리오 runner가 알려진 기대 trace에 대해 PASS를 보고하는지 검증한다.
- step-by-step 실행으로 누적된 trace가 full 실행 trace와 같은지 검증한다.

기존 CMake/CTest 기반 controller 테스트는 유지한다. 비-Windows 콘솔 runner는 CI에서 simulator target의 smoke coverage를 계속 제공한다.

## 범위 제외

- 제3자 GUI 프레임워크 설치
- 웹 기반 시뮬레이터 생성
- 시뮬레이터 시각화에 맞추기 위한 controller 비즈니스 로직 변경
- 기존 unit test 대체
- 지도 편집 또는 사용자 작성 시나리오 추가
