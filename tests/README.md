# Tests

## Layout

- `carousel/`: carousel strategies, geometry, model changes, and index synchronization.
- `layout/`: control, page, toolbar, and layout integration tests, with their QML hosts.
- `integration/`: attached-property propagation and dynamic object creation.
- `model/`: AppStream release model tests.
- `visual/`: PNG capture tool, helper script, and QML scenes in `visual/scenes/`.

Targets and CTest entries are registered in `tests/CMakeLists.txt`. Executables remain
under `build/tests/`; visual test PNGs are written to `build/tests/out/`.

## Build and Run

From the repository root:

```sh
cmake -S . -B build -DQM_BUILD_TESTS=ON
cmake --build build --target toolbar_dynamic_visibility qm_grab --parallel
ctest --test-dir build -R '^toolbar_dynamic_visibility$' --output-on-failure
ctest --test-dir build -R '^visual_button$' --output-on-failure
```

## Capture a Scene

`grab.sh` captures any scene, including files not registered with CTest:

```sh
./tests/visual/grab.sh tests/visual/scenes/button.qml /tmp/button.png
./tests/visual/grab.sh tests/visual/scenes/button.qml /tmp/button.png 800 600 500
```

It locates the default build directory relative to the script, while scene and output
paths remain relative to the caller. Set `QM_BUILD_DIR` to an absolute path for another
build directory. Extra arguments are forwarded to `qm_grab` as width, height, and delay
in milliseconds.

The helper sets `LIBGL_ALWAYS_SOFTWARE=1`, defaults `QT_QPA_PLATFORM` to `eglfs`, and
adds the build's `qml_modules` to `QML_IMPORT_PATH`. Do not use `offscreen` or
`QT_QUICK_BACKEND=software` for shader/elevation verification.

## Add a Visual Scene

- Put the scene in `tests/visual/scenes/` and add its name to `VISUAL_SCENES`.
- Use an `Item` or `Rectangle` root with explicit width and height, not a `Window`,
  `ApplicationWindow`, or `Page`.
- Import the library as `import Qcm.Material as MD`.
- Keep each scene focused on one feature.

CTest checks that a non-empty PNG was produced; it does not compare pixels with a
reference image. Inspect the PNG separately for visual correctness.

Increase the capture delay when animations have not settled. On AMDGPU hosts,
`amdgpu_device_initialize failed` may be printed before Mesa falls back to llvmpipe.
