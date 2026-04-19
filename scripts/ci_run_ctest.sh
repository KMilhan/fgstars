#!/usr/bin/env bash

set -euo pipefail

BUILD_DIR=
REGEX=
JUNIT=
LOG_FILE=
JOBS=1
USE_DISPLAY=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir)
      BUILD_DIR=$2
      shift 2
      ;;
    --regex)
      REGEX=$2
      shift 2
      ;;
    --junit)
      JUNIT=$2
      shift 2
      ;;
    --log-file)
      LOG_FILE=$2
      shift 2
      ;;
    --jobs)
      JOBS=$2
      shift 2
      ;;
    --ui)
      USE_DISPLAY=1
      shift
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 2
      ;;
  esac
done

if [[ -z "${BUILD_DIR}" || -z "${REGEX}" || -z "${JUNIT}" || -z "${LOG_FILE}" ]]; then
  echo "Usage: $0 --build-dir DIR --regex REGEX --junit FILE --log-file FILE [--jobs N] [--ui]" >&2
  exit 2
fi

mkdir -p "$(dirname -- "${JUNIT}")" "$(dirname -- "${LOG_FILE}")"

ctest_cmd=(
  ctest
  --test-dir "${BUILD_DIR}"
  --output-on-failure
  --output-junit "${JUNIT}"
  --output-log "${LOG_FILE}"
  --no-tests=error
  -R "${REGEX}"
  -j "${JOBS}"
)

if [[ "${USE_DISPLAY}" -eq 1 ]]; then
  for command_name in dbus-run-session openbox xvfb-run xdpyinfo; do
    if ! command -v "${command_name}" >/dev/null 2>&1; then
      echo "UI runtime dependency not found: ${command_name}" >&2
      exit 1
    fi
  done
  dbus-run-session -- xvfb-run -a -s "-screen 0 1280x1024x24" \
    bash -lc '
      set -euo pipefail
      export LANG=C.UTF-8
      export LC_ALL=C.UTF-8

      openbox >/tmp/kstars-ci-openbox.log 2>&1 &
      wm_pid=$!

      cleanup() {
        kill "${wm_pid}" >/dev/null 2>&1 || true
      }
      trap cleanup EXIT

      sleep 2
      exec "$@"
    ' bash "${ctest_cmd[@]}"
else
  "${ctest_cmd[@]}"
fi
