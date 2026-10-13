#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENDOR_DIR="$ROOT_DIR/vendor"
LOCK_DIR="$VENDOR_DIR/.locks"
MANIFEST="$ROOT_DIR/scripts/dependencies.txt"
PREBUILT_MANIFEST="$ROOT_DIR/scripts/prebuilt.txt"

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

host_platform() {
    local os arch
    case "$(uname -s)" in
        Linux*) os="linux" ;;
        Darwin*) os="macos" ;;
        MINGW*|MSYS*|CYGWIN*) os="windows" ;;
        *) echo "Unsupported operating system: $(uname -s)" >&2; exit 1 ;;
    esac
    case "$(uname -m)" in
        x86_64|amd64) arch="x64" ;;
        arm64|aarch64) arch="arm64" ;;
        *) echo "Unsupported architecture: $(uname -m)" >&2; exit 1 ;;
    esac
    echo "$os-$arch"
}

install_prebuilt_dependency() {
    local name="$1" platform="$2" url="$3"
    local target="$VENDOR_DIR/$name"
    local lock_file="$LOCK_DIR/$name"

    if [[ $FORCE -eq 0 && -d "$target" && -f "$lock_file" && "$(tr -d '\r\n' < "$lock_file")" == "$url" ]]; then
        echo "[skip] $name ($platform)"
        return
    fi

    echo "[get ] $name ($platform)"
    rm -rf "$target"
    mkdir -p "$target"

    local archive="$LOCK_DIR/$name.archive"
    curl -fsSL -o "$archive" "$url"
    case "$url" in
        *.zip) unzip -q -o "$archive" -d "$target" ;;
        *.tar.gz|*.tgz) tar -xzf "$archive" -C "$target" ;;
        *) echo "Unsupported archive format: $url" >&2; rm -f "$archive"; exit 1 ;;
    esac
    rm -f "$archive"

    printf '%s\n' "$url" > "$lock_file"
}

mkdir -p "$LOCK_DIR"

while read -r name url ref submodules; do
    [[ -z "${name:-}" || "$name" == \#* ]] && continue
    is_selected "$name" || continue
    install_dependency "$name" "$url" "$ref" "$submodules"
done < "$MANIFEST"

PLATFORM="$(host_platform)"
while read -r name platform url; do
    [[ -z "${name:-}" || "$name" == \#* ]] && continue
    [[ "$platform" == "$PLATFORM" ]] || continue
    is_selected "$name" || continue
    install_prebuilt_dependency "$name" "$platform" "$url"
done < "$PREBUILT_MANIFEST"

echo "Dependencies are ready in $VENDOR_DIR"
