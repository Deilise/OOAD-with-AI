# OOAD with AI - Roomba RVC Software Controller

[![C++ CI](https://github.com/Deilise/OOAD-with-AI/actions/workflows/cpp-ci.yml/badge.svg)](https://github.com/Deilise/OOAD-with-AI/actions/workflows/cpp-ci.yml)

This repository contains OOAD documentation and a C++17 implementation skeleton for the Roomba RVC Software Controller.

## C++ Project

The C++ project lives in `cpp/`.

It includes:

- controller classes generated from the class diagram and sequence diagrams
- GoogleTest tests split by class
- a CLI simulator with 30 scripted RVC scenarios
- code coverage support through `gcov` and `lcov`

## Local Build

```powershell
cmake -S cpp -B cpp/build
cmake --build cpp/build
ctest --test-dir cpp/build --output-on-failure
```

Run the simulator:

```powershell
cd cpp
.\run-simulator.ps1
```

## CI

GitHub Actions runs on pushes and pull requests to `main` or `master`.

The workflow:

- runs server-side static analysis with `cppcheck` on RVC headers and source files
- configures the C++ project with CMake and Ninja
- builds the GoogleTest executable
- builds the RVC simulator
- runs all GoogleTest tests
- runs all 30 simulator scenarios as the system-test replacement
- generates a `gcov`/`lcov` coverage report
- uploads coverage and simulator artifacts

See `.github/workflows/cpp-ci.yml`.
