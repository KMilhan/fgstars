#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "${SCRIPT_DIR}/.." && pwd)
SPEC_FILE="${REPO_ROOT}/docs/specs/fgstars-v1-ekos-image-centric-workspace.md"
SUBISSUES_FILE="${REPO_ROOT}/ekos-image-centric-workspace-initiative-sub-issues.md"

overall_pass=0
overall_total=0
quality_pass=0
quality_total=0

declare -a overall_failures=()
declare -a quality_failures=()

contains()
{
    local file="$1"
    local pattern="$2"
    grep -Eq -- "$pattern" "$file"
}

record_check()
{
    local bucket="$1"
    local desc="$2"
    local ok="$3"

    if [[ "${bucket}" == "overall" ]]; then
        overall_total=$((overall_total + 1))
        if [[ "${ok}" == "0" ]]; then
            overall_pass=$((overall_pass + 1))
        else
            overall_failures+=("${desc}")
        fi
    else
        quality_total=$((quality_total + 1))
        if [[ "${ok}" == "0" ]]; then
            quality_pass=$((quality_pass + 1))
        else
            quality_failures+=("${desc}")
        fi
    fi
}

check_contains()
{
    local bucket="$1"
    local desc="$2"
    local file="$3"
    local pattern="$4"

    if contains "${file}" "${pattern}"; then
        record_check "${bucket}" "${desc}" 0
    else
        record_check "${bucket}" "${desc}" 1
    fi
}

if [[ ! -f "${SPEC_FILE}" ]]; then
    echo "Missing spec file: ${SPEC_FILE}" >&2
    exit 1
fi

if [[ ! -f "${SUBISSUES_FILE}" ]]; then
    echo "Missing sub-issues file: ${SUBISSUES_FILE}" >&2
    exit 1
fi

check_contains overall "Spec states the current workspace is Setup-tab-local, not yet Ekos-wide." \
    "${SPEC_FILE}" "Setup-tab-local"
check_contains overall "Spec acknowledges the existing useSummaryPreview capture path." \
    "${SPEC_FILE}" "useSummaryPreview"
check_contains overall "Spec reframes capture routing around the canonical default capture surface." \
    "${SPEC_FILE}" "canonical default capture surface"
check_contains overall "Spec splits shared-workspace work into a bridge step before continuity." \
    "${SPEC_FILE}" "workspace bridge"
check_contains overall "Spec calls out viewport continuity as a later follow-on." \
    "${SPEC_FILE}" "viewport continuity"
check_contains overall "Spec distinguishes current baseline regressions from new fgstars v1 acceptance coverage." \
    "${SPEC_FILE}" "baseline Ekos regressions"
check_contains overall "Spec uses anchored stable-label filtering in the validation command." \
    "${SPEC_FILE}" "\\^stable\\$"
check_contains overall "Spec describes eval_tests.sh as a pass-rate wrapper, not a two-signal evaluator." \
    "${SPEC_FILE}" "pass-rate wrapper"
check_contains overall "Sub-issues plan initial Focus/Align/Guide bridge work instead of full takeover." \
    "${SUBISSUES_FILE}" "Bridge Focus, Align, and Guide image outputs into the shared workspace sink"
check_contains overall "Sub-issues require workspace-specific acceptance coverage before default-on rollout." \
    "${SUBISSUES_FILE}" "workspace-specific UI acceptance coverage"

check_contains quality "Spec preserves a concrete Existing Architecture section." \
    "${SPEC_FILE}" "^## Existing Architecture$"
check_contains quality "Spec preserves explicit Non-Goals." \
    "${SPEC_FILE}" "^## Non-Goals$"
check_contains quality "Spec keeps milestone-based delivery." \
    "${SPEC_FILE}" "^## Milestones$"
check_contains quality "Spec keeps rollout planning." \
    "${SPEC_FILE}" "^## Rollout Plan$"
check_contains quality "Spec ends with open questions and deliverables." \
    "${SPEC_FILE}" "^## Open Questions$"
check_contains quality "Sub-issues remain dependency-ordered and milestone grouped." \
    "${SUBISSUES_FILE}" "dependency-ordered"

overall_score=0
llm_average=0

if (( overall_total > 0 )); then
    overall_score=$((overall_pass * 100 / overall_total))
fi

if (( quality_total > 0 )); then
    llm_average=$((quality_pass * 100 / quality_total))
fi

status="pass"
if (( overall_score < 90 || llm_average < 90 )); then
    status="fail"
fi

echo "overall_score=${overall_score}"
echo "llm_average=${llm_average}"
echo "status=${status}"

if (( ${#overall_failures[@]} > 0 )); then
    printf 'overall_failures=%s\n' "${overall_failures[*]}"
fi

if (( ${#quality_failures[@]} > 0 )); then
    printf 'quality_failures=%s\n' "${quality_failures[*]}"
fi

if [[ "${status}" == "fail" ]]; then
    exit 1
fi
