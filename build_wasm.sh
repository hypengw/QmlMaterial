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
    if [[ ! -x "${qt_wasm_path}/bin/qt-cmake" && -x "${source_qt_path}/bin/qt-cmake" ]]; then
        qt_wasm_path="${source_qt_path}"
    fi
fi

if [[ ! -x "${qt_wasm_path}/bin/qt-cmake" ]]; then
    echo "Qt for WebAssembly not found: ${qt_wasm_path}/bin/qt-cmake" >&2
    exit 1
fi
if [[ ! -d "${qt_host_path}/lib/cmake" ]]; then
    echo "Qt host tools not found: ${qt_host_path}/lib/cmake" >&2
    exit 1
fi

export QT_HOST_PATH="${qt_host_path}"
export QT_HOST_PATH_CMAKE_DIR="${QT_HOST_PATH_CMAKE_DIR:-${qt_host_path}/lib/cmake}"

cmake_args=(
    -S "${project_dir}"
    -B "${build_dir}"
    -G Ninja
    -DQM_BUILD_EXAMPLE=ON
    -DQM_BUILD_TESTS=OFF
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE:-MinSizeRel}"
)

chainload_toolchain="${QT_CHAINLOAD_TOOLCHAIN_FILE:-}"
if [[ -z "${chainload_toolchain}" && "${qt_wasm_path}" == *"/qt-everywhere-src-"* ]]; then
    chainload_toolchain="${EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
fi
if [[ -n "${chainload_toolchain}" ]]; then
    cmake_args+=("-DQT_CHAINLOAD_TOOLCHAIN_FILE=${chainload_toolchain}")
fi

"${qt_wasm_path}/bin/qt-cmake" "${cmake_args[@]}" --fresh "$@"
cmake --build "${build_dir}" --target qm_example --parallel

for output in qm_example.html qm_example.js qm_example.wasm qtloader.js; do
    output_path="${build_dir}/example/${output}"
    if [[ ! -s "${output_path}" ]]; then
        echo "Missing WebAssembly output: ${output_path}" >&2
        exit 1
    fi
done

echo "WebAssembly example built in ${build_dir}/example"
