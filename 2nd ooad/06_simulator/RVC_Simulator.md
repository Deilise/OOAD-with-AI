# Roomba RVC - 시뮬레이터 요약

이 문서는 [코드 요약](../04_code/RVC_Code.md), [단위 테스트 요약](../05_unit_tests/RVC_Unit_Tests.md), [SRS](../01_requirements/RVC_SW_Controller_SRS.md), [sequence diagram](../03_diagrams/sd/RVC_SD_Index.md)의 companion 문서이다.

활성 시뮬레이터 source는 [`../../cpp/simul`](../../cpp/simul)이다. 이 폴더의 [`rvc_simulator.cpp`](rvc_simulator.cpp)는 새 platform entry point의 산출물 스냅샷이다. 실행 스크립트는 이 OOAD 산출물 폴더에서도 `../../cpp`의 실제 target을 빌드하고 실행한다.

## 동작 방식

- 시뮬레이터 target 이름은 기존과 같은 `rvc_simulator`이다.
- Windows에서 인자 없이 실행하면 Win32/GDI GUI가 열린다.
- GUI는 승인된 사진과 동일한 10x10 보드를 표시한다.
- 사용자는 31개 시나리오 중 하나를 선택하고 `Previous`, `Next`, `Auto`, `Reset`, `Run All`로 수동/자동 진행을 제어한다.
- GUI는 선택된 시나리오의 PASS/FAIL, 현재 step, 기대/실제 motion command trace, 기대/실제 cleaning command trace를 보여준다.
- `--verbose`를 주거나 비-Windows에서 빌드하면 동일 시나리오 코어를 사용하는 콘솔 PASS/FAIL runner로 실행된다.
- 모든 시나리오가 통과할 때만 exit code `0`을 반환한다.

## 시뮬레이터 테스트

### Positive / Negative Index

| Positive | Negative |
|----------|----------|
| `TC-01` | `TC-04` |
| `TC-02` | `TC-05` |
| `TC-03` | `TC-09` |
| `TC-06` | `TC-10` |
| `TC-07` | `TC-14` |
| `TC-08` | `TC-20` |
| `TC-11` | `TC-21` |
| `TC-12` | `TC-22` |
| `TC-13` | `TC-23` |
| `TC-15` | `TC-25` |
| `TC-16` | |
| `TC-17` | |
| `TC-18` | |
| `TC-19` | |
| `TC-24` | |
| `TC-26` | |
| `TC-27` | |
| `TC-28` | |
| `TC-29` | |
| `TC-30` | |
| `TC-31` | |

### Detailed Simulator Tests

| Test | Type | What It Tests |
|------|------|---------------|
| `TC-01` | Positive | Initialization enters inactive safe state with `stop` and `suspend`. |
| `TC-02` | Positive | Start plus safe forward state commands forward cleaning. |
| `TC-03` | Positive | Dust boost turns on and returns to normal while active. |
| `TC-04` | Negative | Dust boost is deferred when no session is active. |
| `TC-05` | Negative | Invalid dust signal keeps normal cleaning. |
| `TC-06` | Positive | Stop session stops motion and suspends cleaning. |
| `TC-07` | Positive | Resume after stop can re-enter forward cleaning. |
| `TC-08` | Positive | Service/reset request returns to inactive safe state. |
| `TC-09` | Negative | Invalid obstacle data triggers full fault stop. |
| `TC-10` | Negative | Partial stale obstacle data uses gradual stop behavior. |
| `TC-11` | Positive | Recovered obstacle snapshot is accepted without extra command. |
| `TC-12` | Positive | Forward blocked, right probe clear, then right turn. |
| `TC-13` | Positive | Forward blocked, probed right blocked, then left turn. |
| `TC-14` | Negative | Probe timeout uses `stopOrFallbackTBD`. |
| `TC-15` | Positive | Front + left + probed right blocked starts reverse escape. |
| `TC-16` | Positive | Reverse readings continue backing up. |
| `TC-17` | Positive | Reverse cycle sample can request right-side probe and continue reverse. |
| `TC-18` | Positive | Lateral opening escapes right. |
| `TC-19` | Positive | Left opening escapes left. |
| `TC-20` | Negative | Max backup without opening uses fallback. |
| `TC-21` | Negative | No lateral opening within limits uses fallback. |
| `TC-22` | Negative | Dropout during reverse enters UC-08 style stop. |
| `TC-23` | Negative | Ambiguous obstacle snapshot waits for later policy. |
| `TC-24` | Positive | Front/left sample builds consistency snapshot only. |
| `TC-25` | Negative | Ambiguous probe alignment waits for later policy. |
| `TC-26` | Positive | Forward safe after maneuver resumes cleaning. |
| `TC-27` | Positive | Forward blocked after maneuver requests right-side probe. |
| `TC-28` | Positive | Reverse occurs before right lateral escape. |
| `TC-29` | Positive | Reverse occurs before left lateral escape. |
| `TC-30` | Positive | Initialize, start, forward cleaning, dust boost, trap escape, resume, and stop. |
| `TC-31` | Positive | Approved 10x10 photo-board route with dust detection and top-wall obstacle decision. |

## 변경 사항

- 기존 단일 파일 콘솔 시뮬레이터를 `cpp/simul` 기반 구조로 대체했다.
- 시뮬레이터 코어를 GUI와 콘솔 runner가 공유하도록 분리했다.
- Windows 기본 실행 경로에 Win32/GDI GUI를 추가했다.
- 비-Windows 및 `--verbose` 실행 경로에는 콘솔 PASS/FAIL runner를 유지했다.
- 사진과 동일한 10x10 보드 배치를 `referenceBoard()`로 추가했다.
- 사진 기반 신규 시나리오 `TC-31`을 추가했다.
- `SimulatorCoreTest.cpp`로 보드 배치, 시나리오 개수, 전체 PASS/FAIL, step replay를 검증한다.

## 최신 실행 결과

새 시뮬레이터는 구현 후 콘솔 검증 경로로 실행되었다.

- Total scenarios: `31`
- Passed: `31`
- Failed: `0`
- Status: passed
