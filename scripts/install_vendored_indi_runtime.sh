#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 3 ]]; then
    echo "Usage: install_vendored_indi_runtime.sh <indi-source-dir> <indi-build-dir> <install-prefix>" >&2
    exit 1
fi

source_dir="$1"
build_dir="$2"
prefix="$3"

mkdir -p \
    "$prefix/bin" \
    "$prefix/lib" \
    "$prefix/include/libindi" \
    "$prefix/include/libindi/connectionplugins" \
    "$prefix/include/libindi/dsp" \
    "$prefix/include/libindi/mounts" \
    "$prefix/share/indi/cmake_modules"

copy_binary() {
    local src="$1"
    local dst="$2"
    cp "$src" "$dst"
}

copy_library_family() {
    local pattern="$1"
    compgen -G "$pattern" >/dev/null || return 0
    cp -a $pattern "$prefix/lib/"
}

copy_binary "$build_dir/indiserver/indiserver" "$prefix/bin/indiserver"
copy_binary "$build_dir/drivers/telescope/indi_simulator_telescope" "$prefix/bin/indi_simulator_telescope"
copy_binary "$build_dir/drivers/ccd/indi_simulator_ccd" "$prefix/bin/indi_simulator_ccd"
copy_binary "$build_dir/drivers/focuser/indi_simulator_focus" "$prefix/bin/indi_simulator_focus"
copy_binary "$build_dir/drivers/filter_wheel/indi_simulator_wheel" "$prefix/bin/indi_simulator_wheel"
copy_binary "$build_dir/drivers/ccd/indi_simulator_guide" "$prefix/bin/indi_simulator_guide"
copy_binary "$build_dir/drivers/weather/indi_simulator_weather" "$prefix/bin/indi_simulator_weather"
copy_binary "$build_dir/drivers/rotator/indi_simulator_rotator" "$prefix/bin/indi_simulator_rotator"
copy_binary "$build_dir/drivers/dome/indi_simulator_dome" "$prefix/bin/indi_simulator_dome"
copy_binary "$build_dir/drivers/auxiliary/indi_simulator_lightpanel" "$prefix/bin/indi_simulator_lightpanel"

copy_library_family "$build_dir/libs/indibase/libindidriver.so*"
copy_library_family "$build_dir/libs/alignment/libindiAlignmentDriver.so*"
copy_library_family "$build_dir/drivers/telescope/libindilx200.so*"

rsync -a \
    --prune-empty-dirs \
    --include='*/' \
    --include='*.h' \
    --exclude='*' \
    "$source_dir/libs/indibase/" \
    "$prefix/include/libindi/"

cp "$source_dir/libs/inicpp.h" "$prefix/include/libindi/inicpp.h"

if [[ -f "$source_dir/libs/httplib/httplib.h" ]]; then
    cp "$source_dir/libs/httplib/httplib.h" "$prefix/include/libindi/httplib.h"
fi

if [[ -f "$source_dir/libs/indijson.hpp" ]]; then
    cp "$source_dir/libs/indijson.hpp" "$prefix/include/libindi/indijson.hpp"
fi

cp "$build_dir/indiversion.h" "$prefix/include/libindi/indiversion.h"
cp "$source_dir/drivers/telescope/lx200driver.h" "$prefix/include/libindi/mounts/lx200driver.h"
cp "$source_dir/drivers/telescope/lx200telescope.h" "$prefix/include/libindi/mounts/lx200telescope.h"

rsync -a \
    --prune-empty-dirs \
    --include='FindINDI.cmake' \
    --exclude='*' \
    "$source_dir/cmake_modules/" \
    "$prefix/share/indi/cmake_modules/"
