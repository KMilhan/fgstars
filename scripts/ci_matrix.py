#!/usr/bin/env python3
"""
Emit the shard metadata used by GitHub Actions and local CI tooling.
"""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import asdict, dataclass
from typing import Iterable


@dataclass(frozen=True)
class TestShard:
    name: str
    description: str
    build_targets: tuple[str, ...]
    test_names: tuple[str, ...]
    ctest_jobs: int
    needs_display: bool
    needs_indi_runtime: bool
    timeout_minutes: int

    @property
    def ctest_regex(self) -> str:
        escaped = [name.replace("+", r"\+") for name in self.test_names]
        return "^(" + "|".join(escaped) + ")$"

    def github_matrix_entry(self) -> dict[str, object]:
        return {
            "name": self.name,
            "description": self.description,
            "build_targets": " ".join(self.build_targets),
            "ctest_regex": self.ctest_regex,
            "ctest_jobs": self.ctest_jobs,
            "needs_display": self.needs_display,
            "needs_indi_runtime": self.needs_indi_runtime,
            "timeout_minutes": self.timeout_minutes,
        }


TEST_SHARDS: tuple[TestShard, ...] = (
    TestShard(
        name="unit-core",
        description="Auxiliary, tools, skyobjects, time, and datahandlers stable tests.",
        build_targets=(
            "testcsvparser",
            "testfwparser",
            "testdms",
            "testcachingdms",
            "testcolorscheme",
            "testbinhelper",
            "testfov",
            "testgeolocation",
            "testksuserdb",
            "testtrixelcache",
            "testrectangleoverlap",
            "testksalmanac",
            "testartificialhorizon",
            "testgreatcircle",
            "testdeltara",
            "test_skypoint",
            "test_starobject",
            "test_kstarsdatetime",
            "test_catalogsdb",
        ),
        test_names=(
            "CSVParserTest",
            "FixedWidthParserTest",
            "DMSTest",
            "TestCachingDms",
            "TestColorscheme",
            "TestBinHelper",
            "TestFOV",
            "TestGeolocation",
            "TestKSUserDB",
            "TestTrixelCache",
            "TestRectangleOverlap",
            "TestKSAlmanac",
            "ArtificialHorizonTest",
            "GreatCircleTest",
            "DeltaRATest",
            "TestSkyPoint",
            "TestStarobject",
            "TestKStarsDateTime",
            "test_catalogsdb",
        ),
        ctest_jobs=0,
        needs_display=False,
        needs_indi_runtime=False,
        timeout_minutes=45,
    ),
    TestShard(
        name="unit-astro",
        description="Fitsviewer and non-UI Ekos stable tests.",
        build_targets=(
            "testfitsdata",
            "test_ekos_defects",
            "test_ekos_subtraction",
            "test_placeholderpath",
            "testschedulerunit",
            "testfocus",
            "testfocusstars",
            "test_poleaxis",
            "test_polaralign",
            "testguidestars",
            "teststarcorrespondence",
            "testcalibrationprocess",
        ),
        test_names=(
            "FitsDataTest",
            "DefectTest",
            "SubtractionTest",
            "TestPlaceholderPath",
            "SchedulerunitTest",
            "FocusTest",
            "FocusStarsTest",
            "TestPoleAxis",
            "TestPolarAlign",
            "GuideStarsTest",
            "StarCorrespondenceTest",
            "CalibrationProcessTest",
        ),
        ctest_jobs=0,
        needs_display=False,
        needs_indi_runtime=False,
        timeout_minutes=45,
    ),
    TestShard(
        name="ui-capture",
        description="Stable UI capture and workspace tests under xvfb.",
        build_targets=(
            "test_ekos_capture",
            "test_ekos_capture_workflow",
            "test_ekos_align",
        ),
        test_names=(
            "TestEkosCapture",
            "TestEkosWorkspaceCapture",
            "TestEkosCaptureWorkflow",
            "TestEkosWorkspaceAlign",
        ),
        ctest_jobs=1,
        needs_display=True,
        needs_indi_runtime=True,
        timeout_minutes=60,
    ),
    TestShard(
        name="ui-ops",
        description="Stable UI orchestration tests under xvfb.",
        build_targets=(
            "test_ekos_meridianflipstate",
            "test_ekos_scheduler",
            "test_ekos_scheduler_ops",
            "test_artificial_horizon",
            "test_catalog_download",
        ),
        test_names=(
            "TestEkosMeridianFlipState",
            "TestEkosScheduler",
            "TestEkosSchedulerOps",
            "TestArtificialHorizon",
            "TestCatalogDownload",
        ),
        ctest_jobs=1,
        needs_display=True,
        needs_indi_runtime=True,
        timeout_minutes=60,
    ),
)

STABLE_TEST_NAMES: tuple[str, ...] = tuple(
    test_name for shard in TEST_SHARDS for test_name in shard.test_names
)


def shard_by_name(name: str) -> TestShard:
    for shard in TEST_SHARDS:
        if shard.name == name:
            return shard
    available = ", ".join(shard.name for shard in TEST_SHARDS)
    raise SystemExit(f"Unknown shard '{name}'. Expected one of: {available}")


def emit_lines(values: Iterable[str]) -> None:
    for value in values:
        print(value)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--format",
        required=True,
        choices=(
            "github-matrix",
            "stable-tests",
            "ctest-regex",
            "build-targets",
            "shard-json",
        ),
    )
    parser.add_argument("--shard", help="Shard name for shard-specific formats.")
    parser.add_argument(
        "--pretty",
        action="store_true",
        help="Pretty-print JSON outputs.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    if args.format == "github-matrix":
        payload = [shard.github_matrix_entry() for shard in TEST_SHARDS]
        json.dump(payload, sys.stdout, indent=2 if args.pretty else None)
        sys.stdout.write("\n")
        return

    if args.format == "stable-tests":
        emit_lines(STABLE_TEST_NAMES)
        return

    if not args.shard:
        raise SystemExit(f"--shard is required for --format {args.format}")

    shard = shard_by_name(args.shard)
    if args.format == "ctest-regex":
        print(shard.ctest_regex)
        return
    if args.format == "build-targets":
        emit_lines(shard.build_targets)
        return
    if args.format == "shard-json":
        payload = asdict(shard) | {"ctest_regex": shard.ctest_regex}
        json.dump(payload, sys.stdout, indent=2 if args.pretty else None)
        sys.stdout.write("\n")
        return

    raise SystemExit(f"Unsupported format: {args.format}")


if __name__ == "__main__":
    main()
