#!/usr/bin/env python3
"""
Merge shard JUnit reports and verify that every expected stable case ran.
"""

from __future__ import annotations

import argparse
import json
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

import ci_matrix  # noqa: E402


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--input-dir",
        type=Path,
        help="Directory containing shard JUnit XML reports.",
    )
    parser.add_argument(
        "--output-json",
        type=Path,
        help="Write a machine-readable aggregate summary to this path.",
    )
    parser.add_argument(
        "--output-md",
        type=Path,
        help="Write a Markdown aggregate summary to this path.",
    )
    parser.add_argument(
        "--self-check",
        action="store_true",
        help="Validate imports and shard metadata without reading reports.",
    )
    return parser.parse_args()


def iter_suites(root: ET.Element) -> list[ET.Element]:
    if root.tag == "testsuite":
        return [root]
    if root.tag == "testsuites":
        return [suite for suite in root if suite.tag == "testsuite"]
    raise ValueError(f"Unsupported JUnit root element: {root.tag}")


def parse_report(path: Path) -> tuple[dict[str, float], dict[str, str]]:
    root = ET.parse(path).getroot()
    totals = {"tests": 0.0, "failures": 0.0, "errors": 0.0, "skipped": 0.0, "time": 0.0}
    cases: dict[str, str] = {}

    for suite in iter_suites(root):
        totals["tests"] += float(suite.attrib.get("tests", 0))
        totals["failures"] += float(suite.attrib.get("failures", 0))
        totals["errors"] += float(suite.attrib.get("errors", 0))
        totals["skipped"] += float(suite.attrib.get("skipped", 0))
        totals["time"] += float(suite.attrib.get("time", 0.0))
        for case in suite.findall("testcase"):
            name = case.attrib.get("name", "").strip()
            if not name:
                continue
            if case.find("failure") is not None:
                status = "failure"
            elif case.find("error") is not None:
                status = "error"
            elif case.find("skipped") is not None:
                status = "skipped"
            else:
                status = "passed"
            cases[name] = status

    return totals, cases


def choose_worse_status(existing: str | None, candidate: str) -> str:
    severity = {
        "error": 4,
        "failure": 3,
        "skipped": 2,
        "passed": 1,
    }
    if existing is None:
        return candidate
    if severity[candidate] > severity[existing]:
        return candidate
    return existing


def collect_reports(input_dir: Path) -> tuple[dict[str, float], dict[str, str], list[str]]:
    totals = {"tests": 0.0, "failures": 0.0, "errors": 0.0, "skipped": 0.0, "time": 0.0}
    cases: dict[str, str] = {}
    reports = sorted(input_dir.rglob("*.xml"))
    if not reports:
        raise SystemExit(f"No JUnit XML reports found under {input_dir}")

    for report in reports:
        report_totals, report_cases = parse_report(report)
        for key, value in report_totals.items():
            totals[key] += value
        for name, status in report_cases.items():
            cases[name] = choose_worse_status(cases.get(name), status)

    return totals, cases, [str(report) for report in reports]


def to_markdown(summary: dict[str, object]) -> str:
    missing = summary["missing_cases"]
    failed = summary["failed_cases"]
    lines = [
        "# CI Shard Summary",
        "",
        f"- Reports: {summary['report_count']}",
        f"- Tests: {summary['tests']}",
        f"- Failures: {summary['failures']}",
        f"- Errors: {summary['errors']}",
        f"- Skipped: {summary['skipped']}",
        f"- Time: {summary['time_seconds']:.2f}s",
        f"- Covered stable cases: {summary['covered_cases']}/{summary['expected_cases']}",
        "",
    ]
    if missing:
        lines.append("## Missing Stable Cases")
        lines.extend(f"- {name}" for name in missing)
        lines.append("")
    if failed:
        lines.append("## Failed Cases")
        lines.extend(f"- {name}: {status}" for name, status in failed.items())
        lines.append("")
    if not missing and not failed:
        lines.append("All expected stable cases were covered and passed.")
        lines.append("")
    return "\n".join(lines)


def main() -> None:
    args = parse_args()
    if args.self_check:
        ci_matrix.shard_by_name("unit-core")
        ci_matrix.shard_by_name("unit-astro")
        ci_matrix.shard_by_name("ui-capture")
        ci_matrix.shard_by_name("ui-ops")
        print("self_check=ok")
        return

    if not args.input_dir:
        raise SystemExit("--input-dir is required unless --self-check is used")

    totals, cases, reports = collect_reports(args.input_dir)
    expected = set(ci_matrix.STABLE_TEST_NAMES)
    covered = set(cases)
    missing = sorted(expected - covered)
    failed = {
        name: status
        for name, status in sorted(cases.items())
        if name in expected and status not in {"passed", "skipped"}
    }
    summary = {
        "report_count": len(reports),
        "tests": int(totals["tests"]),
        "failures": int(totals["failures"]),
        "errors": int(totals["errors"]),
        "skipped": int(totals["skipped"]),
        "time_seconds": totals["time"],
        "covered_cases": len(expected & covered),
        "expected_cases": len(expected),
        "missing_cases": missing,
        "failed_cases": failed,
        "reports": reports,
    }

    if args.output_json:
        args.output_json.parent.mkdir(parents=True, exist_ok=True)
        args.output_json.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    if args.output_md:
        args.output_md.parent.mkdir(parents=True, exist_ok=True)
        args.output_md.write_text(to_markdown(summary) + "\n", encoding="utf-8")

    print(json.dumps(summary, indent=2))
    if missing or failed or summary["failures"] or summary["errors"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
