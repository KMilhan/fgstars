#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "${SCRIPT_DIR}/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"${REPO_ROOT}/build"}
LOG_DIR="${REPO_ROOT}/.artifacts/eval-tests"
mkdir -p "${LOG_DIR}"
STAMP=$(date +%Y%m%d-%H%M%S)
CONFIGURE_LOG="${LOG_DIR}/configure-${STAMP}.log"
BUILD_LOG="${LOG_DIR}/build-${STAMP}.log"
LOG_FILE="${LOG_DIR}/ctest-${STAMP}.log"

overall_score=0
llm_average=0
status="pass"

cmake -S "${REPO_ROOT}" -B "${BUILD_DIR}" 2>&1 | tee "${CONFIGURE_LOG}"

cmake --build "${BUILD_DIR}" -- -j"$(nproc)" 2>&1 | tee "${BUILD_LOG}"

ctest_status=0
set +e
ctest --test-dir "${BUILD_DIR}" "$@" --output-on-failure | tee "${LOG_FILE}"
ctest_status=${PIPESTATUS[0]}
set -e

summary_line=$(grep -E "^[[:space:]]*[0-9]+% tests passed, [0-9]+ tests? failed out of [0-9]+" "${LOG_FILE}" | tail -n 1 || true)

if [[ -z "${summary_line}" ]]; then
    status="fail"
else
  if [[ "${summary_line}" =~ ([0-9]+)%\ tests\ passed,\ ([0-9]+)\ tests?\ failed\ out\ of\ ([0-9]+) ]]; then
    overall_score="${BASH_REMATCH[1]}"
    failed="${BASH_REMATCH[2]}"
    total="${BASH_REMATCH[3]}"
    passed=$((total - failed))
    if (( total > 0 )); then
      llm_average=$((passed * 100 / total))
    fi
  else
    status="fail"
  fi
fi

if [[ "${overall_score}" -lt 90 ]] || [[ "${llm_average}" -lt 90 ]]; then
    status="fail"
fi

if [[ "${ctest_status}" -ne 0 ]]; then
    status="fail"
fi

cat <<EOF
overall_score=${overall_score}
llm_average=${llm_average}
status=${status}
configure_log=${CONFIGURE_LOG}
build_log=${BUILD_LOG}
log=${LOG_FILE}
EOF

if [[ "${status}" == "fail" ]]; then
  exit 1
fi
