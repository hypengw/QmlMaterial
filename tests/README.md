# Visual Verification Harness

Closed-loop headless rendering for QML scenes. Agents render a scene to a PNG
via `QQuickRenderControl` + QRhi, then inspect the result visually or via ctest
liveness checks.

## Quick Start

```sh
# 1. Build (from repo root)
cmake -S . -B build -DQM_BUILD_TESTS=ON
cmake --build build -j

# 2. Grab any scene
./tests/grab.sh example/About.qml /tmp/out.png

# 3. Run the full visual suite
ctest --test-dir build -R '^visual_' --output-on-failure
```

Output PNGs land in `build/tests/out/visual_<scene>.png`.

## Why QQuickRenderControl, not grabWindow

The earlier harness used `QQuickWindow::grabWindow()` with
`QT_QPA_PLATFORM=offscreen`. This **silently fails for custom materials**:
the offscreen QPA activates `QSGSoftwareRenderer`, which cannot execute
`QSGMaterialShader` GLSL programs. Custom scenegraph nodes — such as
`MD.Elevation` (shadow via `shadow_material.cpp`) and SDF shapes
(`rectangle_material.cpp`, `blur_mask_material.cpp`) — are silently dropped.
The window renders but all shader-based effects are invisible.

The current harness uses `QQuickRenderControl`, which drives the scenegraph
through QRhi (OpenGL, Mesa llvmpipe in headless environments). This runs every
`QSGMaterialShader` correctly, so shadows, glows, and rounded rectangles
appear in the captured PNG.

**`QT_QUICK_BACKEND=software` has the same limitation as the offscreen
platform** — it is also `QSGSoftwareRenderer` and cannot render custom shaders.
Do not use it for shadow or elevation verification.

## Writing a New Scene

Rules for scenes in `tests/scenes/`:

- Top-level `Item` or `Rectangle` with explicit `width` and `height` — no `Window`.
- No `Page` or `ApplicationWindow`; those require a Window host.
- `import Qcm.Material as MD` works because `grab.sh` injects the import path.
- One feature per scene — keeps visual diffs meaningful.

Example minimal scene:

```qml
import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 400; height: 300
    color: MD.Token.color.surface

    MD.Button { anchors.centerIn: parent; text: "Hello" }
}
```

Register it in `tests/CMakeLists.txt` to get a `visual_<name>` ctest entry.

## Environment Variables

| Variable | Purpose |
|----------|---------|
| `QT_QPA_PLATFORM=eglfs` | EGL headless QPA — required; `offscreen` activates QSGSoftwareRenderer which drops custom shaders |
| `LIBGL_ALWAYS_SOFTWARE=1` | Force Mesa llvmpipe software rasteriser; no GPU needed, deterministic output |
| `QSG_VISUALIZE=batches\|clip\|overdraw\|changes` | Scenegraph debug overlays |
| `QML_IMPORT_TRACE=1` | Verbose import path resolution |
| `QT_LOGGING_RULES='qt.qml.*=true'` | QML engine logging |

`grab.sh` sets `LIBGL_ALWAYS_SOFTWARE=1`, `QT_QPA_PLATFORM=eglfs`, and
`QML_IMPORT_PATH` automatically.

## How Agents Use PNGs

Subagents with multimodal `Read` can open a PNG directly and judge the pixels.
ctest handles liveness (file exists + non-zero size). The QRhi path via
`eglfs` + Mesa software renderer is required for correct shadow and elevation
output — `QSGSoftwareRenderer` (used by `offscreen` platform and
`QT_QUICK_BACKEND=software`) silently discards custom `QSGMaterialShader` nodes.

## Known Limits

- `QT_QPA_PLATFORM=eglfs` on AMDGPU hosts prints harmless init errors
  (`amdgpu_device_initialize failed`); Mesa automatically falls back to llvmpipe.
- Increase `delay_ms` if animations haven't settled:
  `qm_grab scene.qml out.png 800 600 500`.
