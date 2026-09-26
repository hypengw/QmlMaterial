#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: scripts/format.sh [--format|--check]"
    echo "Format tracked C/C++ and QML files, excluding third_party (default: --format)."
    echo "Requires: python3 -m pip install clang-format==22.1.8"
    echo "Requires Qt 6.8.3 qmlformat; set QMLFORMAT or QT_ROOT_DIR, or add it to PATH."
}

mode="${1:---format}"
if [[ $# -gt 1 ]]; then
    usage >&2
    exit 2
fi
case "$mode" in
    --format|--check) ;;
    --help|-h) usage; exit 0 ;;
    *) usage >&2; exit 2 ;;
esac

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"
formatter="${CLANG_FORMAT:-clang-format}"
if ! command -v "$formatter" >/dev/null 2>&1; then
    echo "clang-format not found. Install with: python3 -m pip install clang-format==22.1.8" >&2
    exit 1
fi
version="$("$formatter" --version)"
if [[ ! "$version" =~ clang-format\ version\ 22\.1\.8([[:space:]]|$) ]]; then
    echo "Expected clang-format 22.1.8, found: $version" >&2
    exit 1
fi

qml_formatter="${QMLFORMAT:-${QT_ROOT_DIR:+${QT_ROOT_DIR}/bin/}qmlformat}"
if ! command -v "$qml_formatter" >/dev/null 2>&1; then
    echo "qmlformat not found. Set QMLFORMAT to Qt 6.8.3 bin/qmlformat." >&2
    exit 1
fi
version="$("$qml_formatter" --version)"
if [[ "$version" != "qmlformat 6.8.3" ]]; then
    echo "Expected qmlformat 6.8.3, found: $version" >&2
    exit 1
fi

options=(--style=file --fallback-style=none --fail-on-incomplete-format)
if [[ "$mode" == --check ]]; then
    options+=(--dry-run --Werror)
else
    options+=(-i)
fi

git ls-files -z -- '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx' \
    '*.inl' '*.ipp' '*.m' '*.mm' ':!:third_party/**' |
    {
        files=()
        while IFS= read -r -d '' file; do
            [[ -f "$file" && ! -L "$file" ]] && files+=("$file")
        done
        if [[ ${#files[@]} -eq 0 ]]; then
            echo "No tracked C/C++ files to format."
            exit 0
        fi
        "$formatter" "${options[@]}" -- "${files[@]}"
        echo "$mode passed for ${#files[@]} C/C++ files."
    }

options=(--ignore-settings --indent-width 4 --newline unix)
scratch_dir="$(mktemp -d)"
trap 'rm -f -- "$scratch_dir/formatted.qml" "$scratch_dir/files"; rmdir -- "$scratch_dir"' EXIT
git ls-files -z -- '*.qml' ':!:third_party/**' > "$scratch_dir/files"
count=0
failed=0
while IFS= read -r -d '' file; do
    [[ -f "$file" && ! -L "$file" ]] || continue
    count=$((count + 1))
    if ! "$qml_formatter" "${options[@]}" "$project_dir/$file" > "$scratch_dir/formatted.qml"; then
        echo "Failed to format: $file" >&2
        failed=1
        continue
    fi
    if ! cmp -s -- "$file" "$scratch_dir/formatted.qml"; then
        if [[ "$mode" == --check ]]; then
            diff -u --label "$file" --label "$file (formatted)" -- "$file" "$scratch_dir/formatted.qml" || true
            failed=1
        else
            cp -- "$scratch_dir/formatted.qml" "$file"
        fi
    fi
done < "$scratch_dir/files"

if [[ "$failed" -ne 0 ]]; then
    echo "QML formatting failed. Run scripts/format.sh --format to fix formatting differences." >&2
    exit 1
fi
echo "$mode passed for $count QML files."
