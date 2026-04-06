#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT
build_dir="$tmpdir/build-qt6-check"

cmake -S "$repo_root" -B "$build_dir" -DCMAKE_BUILD_TYPE=Debug >"$tmpdir/configure.log" 2>&1

ref_paths=(
  "$repo_root/CMakeLists.txt"
  "$repo_root/kstars/CMakeLists.txt"
  "$repo_root/datahandlers/CMakeLists.txt"
  "$repo_root/Tests/CMakeLists.txt"
  "$repo_root/Tests/kstars_ui/CMakeLists.txt"
  "$repo_root/kstars/python/CMakeLists.txt"
  "$repo_root/config-kstars.h.cmake"
  "$repo_root/snapcraft.yaml"
  "$repo_root/README.md"
  "$repo_root/INSTALL"
  "$repo_root/cmake/modules/FindINDI.cmake"
  "$repo_root/packaging/android"
  "$repo_root/kstars/indi/clientmanager.h"
  "$repo_root/kstars/indi/blobmanager.h"
  "$repo_root/kstars/ekos/manager.h"
  "$repo_root/kstars/kstarsinit.cpp"
  "$repo_root/kstars/kstarsactions.cpp"
  "$repo_root/kstars/tools/whatsinteresting/skyobjlistmodel.cpp"
  "$repo_root/kstars/tools/moonphasetool.cpp"
  "$repo_root/kstars/tools/imagingplanner.cpp"
  "$repo_root/kstars/dialogs/locationdialog.h"
  "$repo_root/kstars/dialogs/locationdialog.cpp"
)

refs_output=$(rg -n "BUILD_WITH_QT6|Qt5|KF5|USE_QT5_INDI" "${ref_paths[@]}" || true)
ref_count=0
if [[ -n "$refs_output" ]]; then
    ref_count=$(printf '%s\n' "$refs_output" | wc -l | tr -d ' ')
fi

configure_score=100
ref_score=$((100 - ref_count * 10))
if (( ref_score < 0 )); then
    ref_score=0
fi
overall_score=$(((configure_score + ref_score) / 2))
llm_average=$overall_score

echo "configure_ok=1"
echo "forbidden_refs_count=$ref_count"
echo "configure_score=$configure_score"
echo "ref_score=$ref_score"
echo "llm_average=$llm_average"
echo "overall_score=$overall_score"
if [[ -n "$refs_output" ]]; then
    printf '%s\n' "$refs_output"
fi
