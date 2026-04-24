#!/bin/sh
set -e
export LIBGL_ALWAYS_SOFTWARE=1
export QT_QPA_PLATFORM=eglfs
export QML_IMPORT_PATH="$PWD/build/qml_modules${QML_IMPORT_PATH:+:$QML_IMPORT_PATH}"
exec "$PWD/build/tests/qm_grab" "$1" "$2"
