#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)

root_cmake="$repo_root/CMakeLists.txt"
pybind_cmake="$repo_root/kstars/python/CMakeLists.txt"

standard_score=0
required_score=0
extensions_score=0
legacy_flag_score=0

if rg -q 'set\s*\(\s*CMAKE_CXX_STANDARD\s+26\s*\)' "$root_cmake"; then
    standard_score=40
fi

if rg -q 'set\s*\(\s*CMAKE_CXX_STANDARD_REQUIRED\s+ON\s*\)' "$root_cmake"; then
    required_score=20
fi

if rg -q 'set\s*\(\s*CMAKE_CXX_EXTENSIONS\s+OFF\s*\)' "$root_cmake"; then
    extensions_score=20
fi

legacy_hits=$(rg -n 'std=c\+\+(98|03|11|14|17|20|23)|std=gnu\+\+(98|03|11|14|17|20|23)' "$repo_root/CMakeLists.txt" "$pybind_cmake" || true)
if [[ -z "$legacy_hits" ]]; then
    legacy_flag_score=20
fi

overall_score=$((standard_score + required_score + extensions_score + legacy_flag_score))
llm_average=$overall_score

echo "standard_score=$standard_score"
echo "required_score=$required_score"
echo "extensions_score=$extensions_score"
echo "legacy_flag_score=$legacy_flag_score"
echo "llm_average=$llm_average"
echo "overall_score=$overall_score"
if [[ -n "$legacy_hits" ]]; then
    printf '%s\n' "$legacy_hits"
fi
