#!/bin/sh
set -e
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 scene.qml out.png [width height delay_ms]" >&2
    exit 2
fi
repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
build_dir=${QM_BUILD_DIR:-"$repo_dir/build"}
export LIBGL_ALWAYS_SOFTWARE=1
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-eglfs}"
export QML_IMPORT_PATH="$build_dir/qml_modules${QML_IMPORT_PATH:+:$QML_IMPORT_PATH}"
exec "$build_dir/tests/qm_grab" "$@"
