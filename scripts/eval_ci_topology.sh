#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "${SCRIPT_DIR}/.." && pwd)
WORKFLOW="${REPO_ROOT}/.github/workflows/build-and-test.yml"
LOG_DIR="${REPO_ROOT}/.artifacts/eval-ci"
mkdir -p "${LOG_DIR}"
STAMP=$(date +%Y%m%d-%H%M%S)
ACTIONLINT_LOG="${LOG_DIR}/actionlint-${STAMP}.log"

overall_score=0
llm_average=0
status=pass

actionlint_status=0
set +e
actionlint -color < "${WORKFLOW}" > "${ACTIONLINT_LOG}" 2>&1
actionlint_status=$?
set -e

overall_score=$((overall_score + 20))
llm_average=$((llm_average + 20))
if [[ "${actionlint_status}" -ne 0 ]]; then
  status=fail
  overall_score=$((overall_score - 20))
  llm_average=$((llm_average - 20))
fi

python3 "${REPO_ROOT}/scripts/ci_matrix.py" --format github-matrix > "${LOG_DIR}/matrix-${STAMP}.json"
overall_score=$((overall_score + 15))
llm_average=$((llm_average + 15))

python3 "${REPO_ROOT}/scripts/ci_merge_junit.py" --self-check > "${LOG_DIR}/merge-self-check-${STAMP}.txt"
overall_score=$((overall_score + 10))
llm_average=$((llm_average + 10))

required_jobs=(plan-ci build-app test-shards aggregate-reports ci-gate)
for job in "${required_jobs[@]}"; do
  if yq -e ".jobs[\"${job}\"]" "${WORKFLOW}" > /dev/null; then
    overall_score=$((overall_score + 8))
    llm_average=$((llm_average + 8))
  else
    status=fail
  fi
done

if yq -e '.jobs["test-shards"].strategy.matrix.shard == "${{ fromJson(needs.plan-ci.outputs.test_shards_json) }}"' "${WORKFLOW}" > /dev/null; then
  overall_score=$((overall_score + 5))
  llm_average=$((llm_average + 5))
else
  status=fail
fi

if yq -e '.jobs["aggregate-reports"].needs[] == "test-shards"' "${WORKFLOW}" > /dev/null; then
  overall_score=$((overall_score + 5))
  llm_average=$((llm_average + 5))
else
  status=fail
fi

if yq -e '.jobs["test-shards"].steps[] | select((.uses // "") | test("^actions/upload-artifact@v[0-9]+$"))' "${WORKFLOW}" > /dev/null; then
  overall_score=$((overall_score + 5))
  llm_average=$((llm_average + 5))
else
  status=fail
fi

if yq -e '.jobs["aggregate-reports"].steps[] | select((.uses // "") | test("^actions/download-artifact@v[0-9]+$"))' "${WORKFLOW}" > /dev/null; then
  overall_score=$((overall_score + 5))
  llm_average=$((llm_average + 5))
else
  status=fail
fi

if yq -e '.defaults.run.shell == "bash"' "${WORKFLOW}" > /dev/null; then
  overall_score=$((overall_score + 5))
  llm_average=$((llm_average + 5))
else
  status=fail
fi

if rg -q "FIX_WARNINGS=ON" "${WORKFLOW}" "${REPO_ROOT}/scripts/ci_configure.sh"; then
  overall_score=$((overall_score + 5))
  llm_average=$((llm_average + 5))
else
  status=fail
fi

if rg -q "scripts/ci_run_ctest.sh" "${WORKFLOW}"; then
  overall_score=$((overall_score + 5))
  llm_average=$((llm_average + 5))
else
  status=fail
fi

if rg -q "openbox" "${REPO_ROOT}/scripts/ci_prepare_runtime.sh" "${REPO_ROOT}/scripts/ci_run_ctest.sh"; then
  overall_score=$((overall_score + 5))
  llm_average=$((llm_average + 5))
else
  status=fail
fi

if rg -q "ninja -j\\$\\(nproc\\) all install" "${WORKFLOW}"; then
  status=fail
else
  overall_score=$((overall_score + 5))
  llm_average=$((llm_average + 5))
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
workflow=${WORKFLOW}
actionlint_log=${ACTIONLINT_LOG}
EOF

if [[ "${status}" == "fail" ]]; then
  exit 1
fi
