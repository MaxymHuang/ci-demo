# XRI Demo — CI Proof of Concept for a Qt C++ X-Ray Inspection App

A demo application proving that **continuous integration is practical for a large
Windows C++ application with a complex UI**. The vehicle is a dummy x-ray
inspection tool (Qt 6 Widgets) with the workflow of real industrial NDT
software:

1. **Scan** — mock 2D x-ray acquisition with a procedurally generated,
   deterministic radiograph (seeded synthetic objects, optional injected defect)
2. **Viewer** — result image with zoom/pan and a pixel readout
3. **Inspection Setup** — draw, move, resize, rename inspection boxes (ROIs)
   directly on the image
4. **Algorithms** — assign pass/fail algorithms (mean intensity band, density
   range, dark blob count, edge strength) per ROI and run the inspection
5. **Export** — raw + annotated PNG, plus JSON / CSV / HTML inspection reports

## What this proves about CI

| Claim | How it's demonstrated |
| --- | --- |
| A complex Qt UI builds in CI | GitHub Actions Windows job (MSVC + Qt via `install-qt-action`) |
| Core logic is unit-testable without a display | `xri_core` static lib never links QtWidgets; tests use `QTEST_GUILESS_MAIN` |
| Even the UI is testable headlessly | `tst_smoke_mainwindow` drives the real `MainWindow` (clicks, mouse-drag ROI drawing, full workflow) on the `offscreen` platform plugin |
| CI produces a distributable | `windeployqt` + zip uploaded as a build artifact, runs on a Qt-less Windows machine |
| Fast feedback is possible | Parallel Linux job finishes in minutes with cached Qt |

## Architecture

```
src/core/   xri_core (static lib, QtCore + QtGui only — headless-safe)
            image synthesis, scan simulator, ROI model, algorithm engine,
            report exporter, shared session state
src/app/    xri_app_lib (QtWidgets UI: MainWindow, 4 pages, graphics-view
            ROI editor) + the thin xri_demo executable
tests/      QtTest: 5 core unit test suites + 1 headless UI smoke test
```

The hard rule that makes the testing story work: `xri_core` must never include
QtWidgets. All tests set `QT_QPA_PLATFORM=offscreen` via CTest properties, so
`ctest` behaves identically on a developer Mac and a headless CI runner.

## Building locally (macOS)

```sh
brew install qt cmake ninja

cmake --preset mac-debug
cmake --build build/mac-debug
ctest --test-dir build/mac-debug --output-on-failure

./build/mac-debug/src/app/xri_demo        # run the app
```

Run a single test:

```sh
ctest --test-dir build/mac-debug -R tst_algorithms --output-on-failure
# or directly, exactly as CI runs it:
QT_QPA_PLATFORM=offscreen ./build/mac-debug/tests/tst_smoke_mainwindow
```

## Building on Windows

```bat
cmake --preset windows-msvc
cmake --build --preset windows-msvc
ctest --preset windows-msvc
```

Requires Visual Studio 2022 and Qt 6 (set `CMAKE_PREFIX_PATH` to your Qt
install if it is not found automatically).

## CI pipeline

`.github/workflows/ci.yml` runs on every push/PR:

- **linux-fast** — Ninja build + full test suite for quick signal
- **windows** — MSVC build, full test suite (including the headless UI smoke
  test), then packages `xri_demo_win64.zip` with `windeployqt` and uploads it
  as an artifact

Qt installs are cached (`install-qt-action` with `cache: true`), cutting warm
runs to seconds for the Qt setup step.
