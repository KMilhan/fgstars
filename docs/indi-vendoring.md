# INDI vendoring layout

`submodules/indi` is now a tracked source directory intended to be updated as a subtree.

Recommended update flow after committing the initial import:

```bash
git subtree pull --prefix=submodules/indi https://github.com/indilib/indi.git <ref> --squash
```

`indi-3rdparty` is no longer tracked in the repository. It is treated as an optional build-time input.

## Bootstrap indi-3rdparty

```bash
scripts/bootstrap_indi_3rdparty.sh
```

To keep the checkout lean, you can bootstrap only the subdirectory you plan to build:

```bash
scripts/bootstrap_indi_3rdparty.sh --subdir indi-eqmod
```

## Build an optional indi-3rdparty project

Point CMake at the external checkout and enable the optional runtime:

```bash
cmake -S . -B build \
  -DKSTARS_BUILD_VENDORED_INDI_3RDPARTY_RUNTIME=ON \
  -DKSTARS_INDI_3RDPARTY_SOURCE_DIR=$PWD/.artifacts/vendor/indi-3rdparty \
  -DKSTARS_INDI_3RDPARTY_SUBDIR=indi-eqmod
```

The vendored INDI runtime exports a minimal developer layout under the build tree install prefix so the optional
`indi-3rdparty` build can resolve `find_package(INDI REQUIRED)` without a tracked checkout inside this repository.
