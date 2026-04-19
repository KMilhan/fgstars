#!/usr/bin/env bash

set -euo pipefail

SUDO=
if [[ "${EUID}" -ne 0 ]]; then
  if command -v sudo >/dev/null 2>&1; then
    SUDO=sudo
  else
    echo "ci_prepare_runtime.sh requires root or sudo to install UI runtime packages." >&2
    exit 1
  fi
fi

missing_commands=()
for command_name in dbus-run-session openbox xvfb-run xdpyinfo; do
  if ! command -v "${command_name}" >/dev/null 2>&1; then
    missing_commands+=("${command_name}")
  fi
done

needs_cursor_lib=0
if ! dpkg-query -W -f='${Status}\n' libxcb-cursor0 2>/dev/null | grep -q '^install ok installed$'; then
  needs_cursor_lib=1
fi

if [[ "${#missing_commands[@]}" -gt 0 || "${needs_cursor_lib}" -eq 1 ]]; then
  ${SUDO} apt-get update
  ${SUDO} apt-get install -y --no-install-recommends \
    dbus \
    dbus-x11 \
    libxcb-cursor0 \
    openbox \
    x11-utils \
    xauth \
    xvfb
fi

for command_name in dbus-run-session openbox xvfb-run xdpyinfo; do
  if ! command -v "${command_name}" >/dev/null 2>&1; then
    echo "Required UI runtime command is missing after installation: ${command_name}" >&2
    exit 1
  fi
done

if ! dpkg-query -W -f='${Status}\n' libxcb-cursor0 2>/dev/null | grep -q '^install ok installed$'; then
  echo "Required UI runtime package is missing after installation: libxcb-cursor0" >&2
  exit 1
fi
