#!/usr/bin/env bash

set -euo pipefail

apt update
apt install -y --no-install-recommends \
  dbus-x11 \
  libxcb-cursor0 \
  xauth \
  xvfb
