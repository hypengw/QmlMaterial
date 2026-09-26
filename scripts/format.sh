#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: scripts/format.sh [--format|--check]"
    echo "Format tracked C/C++ files, excluding third_party (default: --format)."
    echo "Requires: python3 -m pip install clang-format==22.1.8"
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
        echo "$mode passed for ${#files[@]} files."
    }
