#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "${SCRIPT_DIR}/.." && pwd)
DOC_EVAL="${REPO_ROOT}/scripts/eval_fgstars_workspace_doc.sh"
SPEC_FILE="${REPO_ROOT}/docs/specs/fgstars-v1-ekos-image-centric-workspace.md"
CAPTURE_TEST_CPP="${REPO_ROOT}/Tests/kstars_ui/test_ekos_capture.cpp"
CAPTURE_TEST_H="${REPO_ROOT}/Tests/kstars_ui/test_ekos_capture.h"
CAPTURE_PREVIEW_CPP="${REPO_ROOT}/kstars/ekos/capture/capturepreviewwidget.cpp"
CAPTURE_PREVIEW_H="${REPO_ROOT}/kstars/ekos/capture/capturepreviewwidget.h"
MANAGER_H="${REPO_ROOT}/kstars/ekos/manager.h"
KCFG_FILE="${REPO_ROOT}/kstars/kstars.kcfg"
OPS_MISC_UI="${REPO_ROOT}/kstars/ekos/capture/opsmiscsettings.ui"

contains()
{
    local file="$1"
    local pattern="$2"
    grep -Eq -- "$pattern" "$file"
}

not_contains()
{
    local file="$1"
    local pattern="$2"
    ! grep -Eq -- "$pattern" "$file"
}

overall_pass=0
overall_total=0
quality_pass=0
quality_total=0

declare -a overall_failures=()
declare -a quality_failures=()

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

check_not_contains()
{
    local bucket="$1"
    local desc="$2"
    local file="$3"
    local pattern="$4"

    if not_contains "${file}" "${pattern}"; then
        record_check "${bucket}" "${desc}" 0
    else
        record_check "${bucket}" "${desc}" 1
    fi
}

for required_file in \
    "${DOC_EVAL}" \
    "${SPEC_FILE}" \
    "${CAPTURE_TEST_CPP}" \
    "${CAPTURE_TEST_H}" \
    "${CAPTURE_PREVIEW_CPP}" \
    "${CAPTURE_PREVIEW_H}" \
    "${MANAGER_H}" \
    "${KCFG_FILE}" \
    "${OPS_MISC_UI}"
do
    if [[ ! -f "${required_file}" ]]; then
        echo "Missing required file: ${required_file}" >&2
        exit 1
    fi
done

doc_output=""
doc_status=0
set +e
doc_output=$("${DOC_EVAL}" 2>&1)
doc_status=$?
set -e

doc_overall=$(awk -F= '/^overall_score=/{print $2}' <<<"${doc_output}" | tail -n 1)
doc_llm=$(awk -F= '/^llm_average=/{print $2}' <<<"${doc_output}" | tail -n 1)

if [[ -z "${doc_overall}" ]]; then
    doc_overall=0
fi

if [[ -z "${doc_llm}" ]]; then
    doc_llm=0
fi

check_contains overall "Spec records the frozen Setup-tab-local architecture baseline." \
    "${SPEC_FILE}" "Setup-tab-local"
check_contains overall "Manager now delegates summary preview access to CapturePreviewWidget." \
    "${MANAGER_H}" "capturePreview->summaryFITSView\\(\\)"
check_contains overall "CapturePreviewWidget initializes the embedded SummaryFITSView itself." \
    "${CAPTURE_PREVIEW_CPP}" "initializeSummaryFITSView\\(\\);"
check_contains overall "CapturePreviewWidget owns SummaryFITSView creation inside previewWidget." \
    "${CAPTURE_PREVIEW_CPP}" "SummaryFITSView\\(previewWidget\\)"
check_contains overall "Embedded routing stays guarded by useSummaryPreview when loading in-memory data." \
    "${CAPTURE_PREVIEW_CPP}" "Options::useSummaryPreview\\(\\) && trainSelectionCB->currentText\\(\\) == trainname"
check_contains overall "Embedded routing stays guarded by useSummaryPreview when loading files." \
    "${CAPTURE_PREVIEW_CPP}" "if \\(m_fitsPreview != nullptr && Options::useSummaryPreview\\(\\)\\)"
check_contains overall "The rollout flag remains in persisted settings." \
    "${KCFG_FILE}" "<entry name=\"useSummaryPreview\" type=\"Bool\">"
check_contains overall "The rollout flag remains user-accessible in capture settings UI." \
    "${OPS_MISC_UI}" "name=\"kcfg_useSummaryPreview\""

check_contains quality "Workspace-host ownership test remains declared in the capture UI suite." \
    "${CAPTURE_TEST_H}" "void testEmbeddedWorkspaceHost\\(\\);"
check_contains quality "Workspace-host ownership test validates Manager delegation." \
    "${CAPTURE_TEST_CPP}" "ekos->getSummaryPreview\\(\\)"
check_contains quality "Workspace-host ownership test validates visual dominance of the embedded workspace." \
    "${CAPTURE_TEST_CPP}" "Embedded workspace pane should dominate the contextual side panel by default\\."
check_contains quality "Summary preview persistence test is declared." \
    "${CAPTURE_TEST_H}" "void testSummaryPreviewSettingPersists\\(\\);"
check_contains quality "Summary preview persistence test saves settings through Options." \
    "${CAPTURE_TEST_CPP}" "Options::self\\(\\)->save\\(\\);"
check_contains quality "Summary preview persistence test reloads settings through Options." \
    "${CAPTURE_TEST_CPP}" "Options::self\\(\\)->load\\(\\);"
check_not_contains quality "The older ad-hoc setSummaryFITSView injection path is gone." \
    "${CAPTURE_PREVIEW_H}" "setSummaryFITSView"

code_overall=0
code_llm=0

if (( overall_total > 0 )); then
    code_overall=$((overall_pass * 100 / overall_total))
fi

if (( quality_total > 0 )); then
    code_llm=$((quality_pass * 100 / quality_total))
fi

overall_score=$(((doc_overall + code_overall) / 2))
llm_average=$(((doc_llm + code_llm) / 2))
status="pass"

if (( doc_status != 0 || overall_score < 90 || llm_average < 90 )); then
    status="fail"
fi

echo "doc_overall=${doc_overall}"
echo "doc_llm_average=${doc_llm}"
echo "code_overall=${code_overall}"
echo "code_llm_average=${code_llm}"
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
