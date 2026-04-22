#!/usr/bin/env bash

set -euo pipefail

repo_url="https://github.com/indilib/indi-3rdparty.git"
repo_ref="7e5ff545cf588739df697c67bbdfbf7ed6b5a2f0"
dest_dir="${PWD}/.artifacts/vendor/indi-3rdparty"
subdir=""

usage() {
    cat <<'EOF'
Usage: bootstrap_indi_3rdparty.sh [--dest DIR] [--ref REF] [--subdir PATH]

Clones a pinned indi-3rdparty checkout outside the tracked source tree so it can
be used as an optional build-time input.

Examples:
  scripts/bootstrap_indi_3rdparty.sh
  scripts/bootstrap_indi_3rdparty.sh --subdir indi-eqmod
  cmake -S . -B build \
    -DKSTARS_BUILD_VENDORED_INDI_3RDPARTY_RUNTIME=ON \
    -DKSTARS_INDI_3RDPARTY_SOURCE_DIR=$PWD/.artifacts/vendor/indi-3rdparty \
    -DKSTARS_INDI_3RDPARTY_SUBDIR=indi-eqmod
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --dest)
            dest_dir="$2"
            shift 2
            ;;
        --ref)
            repo_ref="$2"
            shift 2
            ;;
        --subdir)
            subdir="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

mkdir -p "$(dirname "$dest_dir")"

if [[ ! -d "$dest_dir/.git" ]]; then
    git clone --filter=blob:none --no-checkout "$repo_url" "$dest_dir"
fi

git -C "$dest_dir" fetch --depth 1 origin "$repo_ref"

if [[ -n "$subdir" ]]; then
    git -C "$dest_dir" sparse-checkout init --cone
    git -C "$dest_dir" sparse-checkout set cmake_modules "$subdir"
fi

git -C "$dest_dir" checkout --detach FETCH_HEAD

cat <<EOF
Bootstrapped indi-3rdparty at:
  $dest_dir

Use it from CMake with:
  -DKSTARS_INDI_3RDPARTY_SOURCE_DIR=$dest_dir
EOF

if [[ -n "$subdir" ]]; then
    cat <<EOF
  -DKSTARS_INDI_3RDPARTY_SUBDIR=$subdir
EOF
fi
