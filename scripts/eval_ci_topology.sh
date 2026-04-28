#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "${SCRIPT_DIR}/.." && pwd)
BUILD_WORKFLOW="${REPO_ROOT}/.forgejo/workflows/build-and-test.yml"
VALIDATION_WORKFLOW="${REPO_ROOT}/.forgejo/workflows/validation.yml"
LOG_DIR="${REPO_ROOT}/.artifacts/eval-ci"
mkdir -p "${LOG_DIR}"
STAMP=$(date +%Y%m%d-%H%M%S)

overall_score=0
llm_average=0
status=pass

score() {
  local points=$1
  overall_score=$((overall_score + points))
  llm_average=$((llm_average + points))
}

fail() {
  local message=$1
  status=fail
  printf 'FAIL: %s\n' "${message}" >> "${LOG_DIR}/forgejo-topology-${STAMP}.log"
}

for workflow in "${BUILD_WORKFLOW}" "${VALIDATION_WORKFLOW}"; do
  if [[ -f "${workflow}" ]]; then
    score 10
  else
    fail "missing workflow: ${workflow}"
  fi
done

if [[ -d "${REPO_ROOT}/.github/workflows" ]]; then
  fail ".github/workflows should not be active after Forgejo migration"
else
  score 10
fi

python3 "${REPO_ROOT}/scripts/ci_matrix.py" --format stable-non-ui-regex > "${LOG_DIR}/stable-non-ui-${STAMP}.txt"
python3 "${REPO_ROOT}/scripts/ci_matrix.py" --format stable-ui-regex > "${LOG_DIR}/stable-ui-${STAMP}.txt"
python3 "${REPO_ROOT}/scripts/ci_merge_junit.py" --self-check > "${LOG_DIR}/merge-self-check-${STAMP}.txt"
score 15

if yq -e '.jobs["build-and-test"]' "${BUILD_WORKFLOW}" > /dev/null; then
  score 10
else
  fail "build-and-test job missing"
fi

if yq -e '.jobs | length == 1' "${BUILD_WORKFLOW}" > /dev/null; then
  score 10
else
  fail "build workflow should use one combined job"
fi

if yq -e '.jobs["build-and-test"].steps[] | select(.name == "Build all targets")' "${BUILD_WORKFLOW}" > /dev/null; then
  score 8
else
  fail "combined build step missing"
fi

if yq -e '.jobs["build-and-test"].steps[] | select(.name == "Run non-UI stable tests")' "${BUILD_WORKFLOW}" > /dev/null; then
  score 8
else
  fail "non-UI stable test step missing"
fi

if yq -e '.jobs["build-and-test"].steps[] | select(.name == "Run UI stable tests")' "${BUILD_WORKFLOW}" > /dev/null; then
  score 8
else
  fail "UI stable test step missing"
fi

if rg -q "FORGEJO_WORKSPACE|forgejo\\.workspace|forgejo\\.sha|FORGEJO_STEP_SUMMARY" "${BUILD_WORKFLOW}" "${VALIDATION_WORKFLOW}"; then
  score 8
else
  fail "Forgejo context or environment variables not used"
fi

if rg -q "https://data\\.forgejo\\.org/actions/checkout@v[0-9]+" "${BUILD_WORKFLOW}" "${VALIDATION_WORKFLOW}"; then
  score 6
else
  fail "Forgejo action URL for checkout missing"
fi

if rg -q "scripts/ci_run_ctest.sh" "${BUILD_WORKFLOW}"; then
  score 5
else
  fail "ctest wrapper missing from build workflow"
fi

if rg -q "FIX_WARNINGS=ON" "${REPO_ROOT}/scripts/ci_configure.sh"; then
  score 5
else
  fail "warnings-as-errors configure flag missing"
fi

if rg -q "openbox" "${REPO_ROOT}/scripts/ci_prepare_runtime.sh" "${REPO_ROOT}/scripts/ci_run_ctest.sh"; then
  score 5
else
  fail "UI runtime wrapper missing openbox support"
fi

if rg -qi "gitlab|github-matrix|\\.github/workflows" "${BUILD_WORKFLOW}" "${VALIDATION_WORKFLOW}" "${REPO_ROOT}/scripts/ci_matrix.py"; then
  fail "stale GitHub/GitLab workflow wording remains"
else
  score 5
fi

if [[ "${overall_score}" -gt 100 ]]; then
  overall_score=100
fi
if [[ "${llm_average}" -gt 100 ]]; then
  llm_average=100
fi

if [[ "${overall_score}" -lt 90 || "${llm_average}" -lt 90 ]]; then
  status=fail
fi

cat <<EOF
overall_score=${overall_score}
llm_average=${llm_average}
status=${status}
build_workflow=${BUILD_WORKFLOW}
validation_workflow=${VALIDATION_WORKFLOW}
log=${LOG_DIR}/forgejo-topology-${STAMP}.log
EOF

if [[ "${status}" == "fail" ]]; then
  exit 1
fi
