#!/usr/bin/env bash

set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
qt_version="${QT_VERSION:-6.9.1}"
emsdk_root="${EMSDK_ROOT:-${EMSDK:-${HOME}/.local/share/emsdk}}"
qt_host_path="${QT_HOST_PATH:-${HOME}/.local/share/Qt/${qt_version}/linux_gcc}"
build_dir="${BUILD_DIR:-${project_dir}/build_wasm}"

if [[ "${build_dir}" != /* ]]; then
    build_dir="${project_dir}/${build_dir}"
fi

if [[ ! -f "${emsdk_root}/emsdk_env.sh" ]]; then
    echo "Emscripten environment not found: ${emsdk_root}/emsdk_env.sh" >&2
    exit 1
fi

source "${emsdk_root}/emsdk_env.sh" >/dev/null

if [[ -n "${QT_WASM_PATH:-}" ]]; then
    qt_wasm_path="${QT_WASM_PATH}"
else
    qt_wasm_path="${HOME}/.local/share/Qt/${qt_version}/wasm_singlethread"
    source_qt_path="${HOME}/.local/share/Qt/${qt_version}/qt-everywhere-src-${qt_version}/qtbase"
    if [[ ! -f "${qt_wasm_path}/bin/qt-cmake" && -f "${source_qt_path}/bin/qt-cmake" ]]; then
        qt_wasm_path="${source_qt_path}"
    fi
fi

qt_cmake="${qt_wasm_path}/bin/qt-cmake"
qt_toolchain="${qt_wasm_path}/lib/cmake/Qt6/qt.toolchain.cmake"

if [[ ! -f "${qt_cmake}" ]]; then
    echo "Qt for WebAssembly not found: ${qt_wasm_path}/bin/qt-cmake" >&2
    exit 1
fi
if [[ ! -f "${qt_toolchain}" ]]; then
    echo "Qt WebAssembly toolchain not found: ${qt_toolchain}" >&2
    exit 1
fi
if [[ ! -f "${qt_host_path}/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
    echo "Qt host tools not found: ${qt_host_path}" >&2
    exit 1
fi

export QT_HOST_PATH="${qt_host_path}"

cmake_args=(
    -S "${project_dir}"
    -B "${build_dir}"
    -G Ninja
    -DQM_BUILD_EXAMPLE=ON
    -DQM_BUILD_TESTS=OFF
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE:-MinSizeRel}"
    -DQT_HOST_PATH="${qt_host_path}"
)

chainload_toolchain="${QT_CHAINLOAD_TOOLCHAIN_FILE:-}"
if [[ -z "${chainload_toolchain}" && "${qt_wasm_path}" == *"/qt-everywhere-src-"* ]]; then
    chainload_toolchain="${EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
fi
if [[ -n "${chainload_toolchain}" ]]; then
    cmake_args+=("-DQT_CHAINLOAD_TOOLCHAIN_FILE=${chainload_toolchain}")
fi

sh "${qt_cmake}" "${cmake_args[@]}" --fresh "$@"
cmake --build "${build_dir}" --target qm_example --parallel

outputs=(qm_example.html qm_example.js qm_example.wasm qtloader.js qtlogo.svg)
for output in "${outputs[@]}"; do
    output_path="${build_dir}/example/${output}"
    if [[ ! -s "${output_path}" ]]; then
        echo "Missing WebAssembly output: ${output_path}" >&2
        exit 1
    fi
done

wasm_output="${build_dir}/example/qm_example.wasm"
qml_plugins=(
    Qcm_MaterialPlugin
    Qcm_Material_LayoutsPlugin
    QmlShapesPlugin
    QtQuickEffectsPlugin
)
for qml_plugin in "${qml_plugins[@]}"; do
    if ! grep -aFq "${qml_plugin}" "${wasm_output}"; then
        echo "Missing static QML plugin in WebAssembly output: ${qml_plugin}" >&2
        exit 1
    fi
done

site_dir="${build_dir}/site"
cmake -E make_directory "${site_dir}"
cmake -E copy_if_different "${build_dir}/example/qm_example.html" "${site_dir}/index.html"
for output in qm_example.js qm_example.wasm qtloader.js qtlogo.svg; do
    cmake -E copy_if_different "${build_dir}/example/${output}" "${site_dir}/${output}"
done

echo "WebAssembly site built in ${site_dir}"
