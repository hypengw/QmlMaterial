#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
if command -v uv >/dev/null 2>&1; then
    exec uv run --script "$project_dir/scripts/update_icons.py" "$@"
fi
exec "${PYTHON:-python3}" "$project_dir/scripts/update_icons.py" "$@"
