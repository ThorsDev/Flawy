#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENDOR_DIR="$ROOT_DIR/vendor"
LOCK_DIR="$VENDOR_DIR/.locks"
MANIFEST="$ROOT_DIR/scripts/dependencies.txt"

FORCE=0
ONLY=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --force) FORCE=1 ;;
        --only) shift; ONLY+=("$1") ;;
        *) echo "Unknown argument: $1" >&2; exit 1 ;;
    esac
    shift
done

command -v git >/dev/null 2>&1 || { echo "git is required but was not found on PATH." >&2; exit 1; }

is_selected() {
    [[ ${#ONLY[@]} -eq 0 ]] && return 0
    local candidate
    for candidate in "${ONLY[@]}"; do
        [[ "$candidate" == "$1" ]] && return 0
    done
    return 1
}

install_dependency() {
    local name="$1" url="$2" ref="$3" submodules="$4"
    local target="$VENDOR_DIR/$name"
    local lock_file="$LOCK_DIR/$name"
    local lock_value="$url $ref"

    if [[ $FORCE -eq 0 && -d "$target" && -f "$lock_file" && "$(tr -d '\r\n' < "$lock_file")" == "$lock_value" ]]; then
        echo "[skip] $name @ $ref"
        return
    fi

    echo "[get ] $name @ $ref"
    rm -rf "$target"
    mkdir -p "$target"

    git -C "$target" init --quiet
    git -C "$target" remote add origin "$url"
    git -C "$target" fetch --quiet --depth 1 origin "$ref"
    git -C "$target" -c advice.detachedHead=false checkout --quiet FETCH_HEAD
    if [[ "$submodules" == "yes" ]]; then
        git -C "$target" submodule update --init --recursive --depth 1 --quiet
    fi

    printf '%s\n' "$lock_value" > "$lock_file"
}

mkdir -p "$LOCK_DIR"

while read -r name url ref submodules; do
    [[ -z "${name:-}" || "$name" == \#* ]] && continue
    is_selected "$name" || continue
    install_dependency "$name" "$url" "$ref" "$submodules"
done < "$MANIFEST"

echo "Dependencies are ready in $VENDOR_DIR"
