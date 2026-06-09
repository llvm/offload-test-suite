#!/usr/bin/env python3
"""Synchronize XFAIL/UNSUPPORTED annotations with actual CI results.

Compares the XFAIL and UNSUPPORTED annotations in .test/.yaml files against
recent GitHub Actions CI results to identify:
  1. Stale XFAILs — tests marked XFAIL that now consistently pass (XPASS).
  2. Missing XFAILs — tests that consistently fail across recent runs but have
     no XFAIL annotation for that configuration.

Requires:
  * GitHub CLI (`gh`) authenticated with access to llvm/offload-test-suite.
  * Python 3.8+

Usage:
    python xfail_sync.py [options]

    # Show stale XFAILs and missing XFAILs for all vendors (last 5 runs):
    python xfail_sync.py

    # Only check Intel scheduled runs:
    python xfail_sync.py --vendor intel

    # Use more runs for higher confidence:
    python xfail_sync.py --runs 10

    # Only report stale XFAILs (tests that now pass):
    python xfail_sync.py --stale-only

    # Only report missing XFAILs (consistent new failures):
    python xfail_sync.py --missing-only
"""

import argparse
import json
import os
import pathlib
import re
import subprocess
import sys
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass, field

OWNER = "llvm"
REPO = "offload-test-suite"

# Maps workflow file basenames to (vendor, compiler, api) tuples.
# We parse the naming convention: windows-{vendor}-{compiler}-{api}.yaml
# Also handles macOS workflows.
WORKFLOW_PATTERN = re.compile(
    r"^(?:windows|macos)-"
    r"(?P<vendor>intel|amd|nvidia|qc|clang|dxc)-"
    r"(?P<compiler>clang|dxc)-"
    r"(?P<api>d3d12|vk|mtl|warp-d3d12|warp-preview-d3d12)"
    r"(?:-gbv)?\.yaml$"
)

# Lit test result statuses we care about.
PASS_STATUSES = {"PASS", "XFAIL", "UNSUPPORTED"}
FAIL_STATUSES = {"FAIL", "XPASS"}

# Suite name patterns from check-hlsl-{suite} target naming.
SUITE_PATTERN = re.compile(r"check-hlsl-(.+)")

# ANSI colors for output.
RED = "\033[31m"
GREEN = "\033[32m"
YELLOW = "\033[33m"
CYAN = "\033[36m"
DIM = "\033[2m"
BOLD = "\033[1m"
RESET = "\033[0m"


def color(code, text):
    if not sys.stdout.isatty():
        return text
    return f"{code}{text}{RESET}"


@dataclass
class TestAnnotation:
    """An XFAIL or UNSUPPORTED annotation parsed from a test file."""

    file_path: str
    line_number: int
    kind: str  # "XFAIL" or "UNSUPPORTED"
    expression: str  # e.g., "Clang && Vulkan"


@dataclass
class TestResult:
    """A single test result from a CI run."""

    test_name: str  # Relative path like "Feature/MaximalReconvergence/loop_peeling.test"
    status: str  # PASS, FAIL, XFAIL, XPASS, UNSUPPORTED
    suite: str  # e.g., "clang-d3d12", "d3d12", "clang-vk"


@dataclass
class CIRun:
    """A completed CI workflow run."""

    run_id: int
    workflow_name: str
    suite: str  # Derived suite name
    vendor: str
    conclusion: str
    created_at: str


def api_get(path):
    """Issue a GitHub API GET via `gh api`."""
    result = subprocess.run(
        ["gh", "api", "-H", "Accept: application/vnd.github+json", path],
        capture_output=True,
        check=True,
    )
    return json.loads(result.stdout.decode("utf-8", errors="replace"))


def get_workflows():
    """Fetch all workflows for the repo."""
    path = f"/repos/{OWNER}/{REPO}/actions/workflows?per_page=100"
    return api_get(path).get("workflows", [])


def get_completed_runs(workflow_id, count=5, branch="main"):
    """Fetch the most recent completed runs for a workflow on a branch."""
    path = (
        f"/repos/{OWNER}/{REPO}/actions/workflows/{workflow_id}/runs"
        f"?status=completed&branch={branch}&per_page={count}"
    )
    data = api_get(path)
    return data.get("workflow_runs", [])[:count]


def get_run_artifacts(run_id):
    """List artifacts for a run."""
    path = f"/repos/{OWNER}/{REPO}/actions/runs/{run_id}/artifacts?per_page=100"
    return api_get(path).get("artifacts", [])


def get_run_jobs(run_id):
    """Fetch jobs for a run to get step-level pass/fail info."""
    path = f"/repos/{OWNER}/{REPO}/actions/runs/{run_id}/jobs?per_page=100"
    return api_get(path).get("jobs", [])


def download_test_log(run_id):
    """Download and parse the test log for a run.

    GitHub Actions stores logs per job. We look for the 'Run HLSL Tests' step
    output which contains lit's result summary.
    """
    try:
        result = subprocess.run(
            ["gh", "run", "view", str(run_id), "--repo", f"{OWNER}/{REPO}",
             "--log"],
            capture_output=True,
            timeout=60,
        )
        if result.returncode != 0:
            return None
        return result.stdout.decode("utf-8", errors="replace")
    except (subprocess.TimeoutExpired, subprocess.CalledProcessError):
        return None


def parse_lit_log(log_text, suite):
    """Parse lit output from a CI log and extract test results.

    Lit output lines look like:
        PASS: OffloadTest-clang-d3d12 :: Feature/HLSLLib/abs.test (1 of 200)
        FAIL: OffloadTest-clang-d3d12 :: Basic/simple.test (2 of 200)
        XFAIL: OffloadTest-d3d12 :: Bugs/Adjacent-Partial-Writes.yaml (3 of 200)
        UNSUPPORTED: OffloadTest-clang-vk :: Basic/cbuffer.test (4 of 200)
    """
    results = []
    # Match lit result lines. The suite name in lit is "OffloadTest-{suite}".
    pattern = re.compile(
        r"(PASS|FAIL|XFAIL|XPASS|UNSUPPORTED|UNRESOLVED):\s+"
        r"OffloadTest-(\S+)\s+::\s+(.+?)\s+\("
    )
    for line in log_text.splitlines():
        m = pattern.search(line)
        if m:
            status, lit_suite, test_name = m.groups()
            # Only collect results for the suite we're interested in, or all
            # if suite is not specified.
            if suite and lit_suite != suite:
                continue
            results.append(TestResult(
                test_name=test_name.strip(),
                status=status,
                suite=lit_suite,
            ))
    return results


def parse_xunit_xml(xml_text, suite):
    """Parse xUnit XML test results (fallback if raw logs unavailable).

    The xunit XML produced by lit has <testcase> elements with classname
    set to the suite and name set to the test path.
    """
    import xml.etree.ElementTree as ET

    results = []
    try:
        root = ET.fromstring(xml_text)
    except ET.ParseError:
        return results

    for testcase in root.iter("testcase"):
        classname = testcase.get("classname", "")
        name = testcase.get("name", "")
        # Determine status from child elements.
        if testcase.find("failure") is not None:
            status = "FAIL"
        elif testcase.find("skipped") is not None:
            status = "UNSUPPORTED"
        else:
            status = "PASS"

        # classname is typically "OffloadTest-{suite}" or similar.
        lit_suite = classname.replace("OffloadTest-", "") if classname else suite
        results.append(TestResult(
            test_name=name.strip(),
            status=status,
            suite=lit_suite,
        ))
    return results


def derive_suite_from_workflow(workflow_name):
    """Derive the lit suite name from a workflow name.

    Workflow names follow patterns like:
        "Windows D3D12 Intel Clang" -> clang-d3d12
        "Windows Vulkan AMD DXC" -> vk
        "Windows D3D12 WARP Intel Clang" -> clang-warp-d3d12
    """
    name_lower = workflow_name.lower()

    is_clang = "clang" in name_lower
    is_warp = "warp" in name_lower

    if "d3d12" in name_lower or "directx" in name_lower:
        api = "warp-d3d12" if is_warp else "d3d12"
    elif "vulkan" in name_lower or " vk" in name_lower:
        api = "vk"
    elif "metal" in name_lower or " mtl" in name_lower:
        api = "mtl"
    else:
        return None

    if is_clang:
        return f"clang-{api}"
    return api


def derive_vendor_from_workflow(workflow_name):
    """Extract vendor from workflow name."""
    name_lower = workflow_name.lower()
    for vendor in ("intel", "amd", "nvidia", "qc"):
        if vendor in name_lower:
            return vendor
    if "macos" in name_lower or "mac" in name_lower:
        return "apple"
    return "unknown"


def suite_to_features(suite):
    """Map a suite name to the lit features it implies.

    This mirrors the feature logic in lit.cfg.py:
    - "clang-d3d12" -> Clang, DirectX
    - "d3d12" -> DXC, DirectX
    - "clang-vk" -> Clang, Vulkan
    - "vk" -> DXC, Vulkan
    - "clang-mtl" -> Clang, Metal
    - "mtl" -> DXC, Metal
    - "clang-warp-d3d12" -> Clang, DirectX, WARP
    - "warp-d3d12" -> DXC, DirectX, WARP
    """
    features = set()

    if suite.startswith("clang-"):
        features.add("Clang")
        rest = suite[len("clang-"):]
    else:
        features.add("DXC")
        rest = suite

    if "warp" in rest:
        features.add("WARP")
        features.add("DirectX")
    elif rest == "d3d12":
        features.add("DirectX")
    elif rest == "vk":
        features.add("Vulkan")
    elif rest == "mtl":
        features.add("Metal")

    return features


def evaluate_bool_expr(expr, features):
    """Evaluate a lit boolean expression against a set of features.

    Supports: &&, ||, !, and parentheses (simplified parsing).
    Feature names are case-sensitive and matched exactly against the set.

    Examples:
        "Clang && Vulkan" with features={"Clang", "Vulkan"} -> True
        "!Clang" with features={"DXC", "DirectX"} -> True
        "QC" with features={"Clang", "DirectX"} -> False
    """
    # Tokenize: split on &&, ||, !, (, ) while keeping delimiters.
    tokens = re.findall(r"&&|\|\||!|\(|\)|[A-Za-z0-9_\-]+", expr)
    if not tokens:
        return False

    # Simple recursive descent parser.
    pos = [0]

    def peek():
        return tokens[pos[0]] if pos[0] < len(tokens) else None

    def advance():
        t = tokens[pos[0]]
        pos[0] += 1
        return t

    def parse_or():
        left = parse_and()
        while peek() == "||":
            advance()
            right = parse_and()
            left = left or right
        return left

    def parse_and():
        left = parse_not()
        while peek() == "&&":
            advance()
            right = parse_not()
            left = left and right
        return left

    def parse_not():
        if peek() == "!":
            advance()
            return not parse_not()
        return parse_atom()

    def parse_atom():
        t = peek()
        if t == "(":
            advance()
            result = parse_or()
            if peek() == ")":
                advance()
            return result
        if t is not None and t not in ("&&", "||", "!", "(", ")"):
            advance()
            return t in features
        # Unexpected token or end of input; treat as false.
        return False

    try:
        return parse_or()
    except (IndexError, TypeError):
        return False


def scan_test_annotations(test_root):
    """Scan all .test and .yaml files for XFAIL/UNSUPPORTED annotations."""
    annotations = defaultdict(list)  # test_name -> [TestAnnotation]

    for ext in ("*.test", "*.yaml"):
        for path in test_root.rglob(ext):
            rel_path = str(path.relative_to(test_root)).replace("\\", "/")
            try:
                lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
            except OSError:
                continue
            for i, line in enumerate(lines, 1):
                m = re.match(r"#\s*(XFAIL|UNSUPPORTED):\s*(.+)", line)
                if m:
                    kind, expression = m.groups()
                    annotations[rel_path].append(TestAnnotation(
                        file_path=str(path),
                        line_number=i,
                        kind=kind,
                        expression=expression.strip(),
                    ))
    return annotations


def fetch_workflow_results(workflows, vendor_filter, num_runs, branch):
    """Fetch test results from recent CI runs.

    Returns: dict mapping (suite, test_name) -> list of statuses across runs.
    """
    # Filter workflows to scheduled ones (the stable signal).
    scheduled_workflows = []
    for wf in workflows:
        name_lower = wf["name"].lower()
        # Skip PR-only workflows and non-test workflows.
        if "format" in name_lower or "description" in name_lower:
            continue
        if "execution testing" in name_lower:
            continue  # PR matrix — not stable scheduled signal.
        vendor = derive_vendor_from_workflow(wf["name"])
        if vendor_filter and vendor != vendor_filter:
            continue
        suite = derive_suite_from_workflow(wf["name"])
        if not suite:
            continue
        scheduled_workflows.append((wf, suite, vendor))

    if not scheduled_workflows:
        print(color(YELLOW, "No matching scheduled workflows found."))
        return {}

    print(f"Fetching results from {len(scheduled_workflows)} workflow(s), "
          f"{num_runs} run(s) each...")

    # Collect run IDs to fetch logs for.
    runs_to_fetch = []  # (run_id, suite, vendor, workflow_name)

    def fetch_runs_for_workflow(wf_tuple):
        wf, suite, vendor = wf_tuple
        try:
            runs = get_completed_runs(wf["id"], count=num_runs, branch=branch)
            return [(r["id"], suite, vendor, wf["name"]) for r in runs]
        except (subprocess.CalledProcessError, json.JSONDecodeError, TypeError,
                KeyError, OSError):
            return []

    with ThreadPoolExecutor(max_workers=8) as pool:
        futures = [pool.submit(fetch_runs_for_workflow, wt) for wt in scheduled_workflows]
        for f in as_completed(futures):
            runs_to_fetch.extend(f.result())

    if not runs_to_fetch:
        print(color(YELLOW, "No completed runs found."))
        return {}

    print(f"Downloading logs for {len(runs_to_fetch)} run(s)...")

    # Fetch logs in parallel.
    # result_map: (suite, test_name) -> [status, status, ...]
    result_map = defaultdict(list)

    def fetch_and_parse(run_info):
        run_id, suite, vendor, wf_name = run_info
        log = download_test_log(run_id)
        if not log:
            return []
        return parse_lit_log(log, suite)

    with ThreadPoolExecutor(max_workers=4) as pool:
        futures = {pool.submit(fetch_and_parse, ri): ri for ri in runs_to_fetch}
        completed = 0
        for f in as_completed(futures):
            completed += 1
            if completed % 5 == 0 or completed == len(futures):
                print(f"  Parsed {completed}/{len(futures)} logs...", end="\r")
            results = f.result()
            for r in results:
                result_map[(r.suite, r.test_name)].append(r.status)

    print()  # Clear the \r line.
    return result_map


def find_stale_xfails(annotations, result_map):
    """Find XFAIL annotations where the test now consistently passes.

    An XFAIL is stale if, for every suite where the XFAIL expression would
    match, the test consistently shows XPASS (or PASS without triggering XFAIL).
    """
    stale = []

    for test_name, annots in annotations.items():
        for annot in annots:
            if annot.kind != "XFAIL":
                continue

            # Check all suites where this XFAIL would apply.
            matched_any_suite = False
            all_passing = True

            for (suite, tname), statuses in result_map.items():
                if tname != test_name:
                    continue
                features = suite_to_features(suite)
                if not evaluate_bool_expr(annot.expression, features):
                    continue

                matched_any_suite = True
                # If the test is reported as XPASS in recent runs, the XFAIL
                # is stale. If it's XFAIL, the annotation is still valid.
                for s in statuses:
                    if s == "XFAIL":
                        all_passing = False
                        break
                    # FAIL means the test fails for a different reason (not XFAIL match).
                    if s == "FAIL":
                        all_passing = False
                        break

            if matched_any_suite and all_passing:
                stale.append(annot)

    return stale


def find_missing_xfails(annotations, result_map, min_fail_rate=1.0):
    """Find tests that consistently fail but have no matching XFAIL.

    A missing XFAIL is reported when a test fails in >= min_fail_rate fraction
    of runs for a suite, and no existing XFAIL annotation covers that suite.
    """
    missing = []

    # Group results by test.
    tests_in_results = defaultdict(dict)  # test_name -> {suite: [statuses]}
    for (suite, test_name), statuses in result_map.items():
        tests_in_results[test_name][suite] = statuses

    for test_name, suite_results in tests_in_results.items():
        for suite, statuses in suite_results.items():
            if not statuses:
                continue

            fail_count = sum(1 for s in statuses if s in ("FAIL", "UNRESOLVED"))
            total = len(statuses)
            if total == 0:
                continue

            fail_rate = fail_count / total
            if fail_rate < min_fail_rate:
                continue

            # Check if an existing XFAIL covers this.
            features = suite_to_features(suite)
            covered = False
            for annot in annotations.get(test_name, []):
                if annot.kind == "XFAIL" and evaluate_bool_expr(annot.expression, features):
                    covered = True
                    break
                if annot.kind == "UNSUPPORTED" and evaluate_bool_expr(annot.expression, features):
                    covered = True
                    break

            if not covered:
                missing.append({
                    "test_name": test_name,
                    "suite": suite,
                    "features": features,
                    "fail_count": fail_count,
                    "total_runs": total,
                    "fail_rate": fail_rate,
                })

    return missing


def suggest_xfail_expr(features):
    """Suggest a minimal XFAIL expression for a set of suite features.

    Combines the compiler and API features into a boolean AND expression.
    """
    parts = []
    if "Clang" in features:
        parts.append("Clang")
    elif "DXC" in features:
        parts.append("DXC")

    if "DirectX" in features:
        parts.append("DirectX")
    elif "Vulkan" in features:
        parts.append("Vulkan")
    elif "Metal" in features:
        parts.append("Metal")

    if "WARP" in features:
        parts.append("WARP")

    return " && ".join(parts) if parts else "{{unknown}}"


def print_stale_xfails(stale):
    """Print stale XFAIL report."""
    if not stale:
        print(color(GREEN, "\n✓ No stale XFAILs found."))
        return

    print(color(BOLD, f"\n{'='*70}"))
    print(color(BOLD + YELLOW, f" STALE XFAILs ({len(stale)} found)"))
    print(color(DIM, " These tests are marked XFAIL but now consistently pass."))
    print(color(BOLD, f"{'='*70}"))

    for annot in sorted(stale, key=lambda a: a.file_path):
        rel = os.path.relpath(annot.file_path)
        print(f"\n  {color(CYAN, rel)}:{annot.line_number}")
        print(f"    # XFAIL: {annot.expression}")
        print(f"    {color(GREEN, '→ Consider removing this annotation.')}")


def print_missing_xfails(missing):
    """Print missing XFAIL report."""
    if not missing:
        print(color(GREEN, "\n✓ No missing XFAILs found (no consistent new failures)."))
        return

    print(color(BOLD, f"\n{'='*70}"))
    print(color(BOLD + RED, f" MISSING XFAILs ({len(missing)} found)"))
    print(color(DIM, " These tests consistently fail but have no XFAIL annotation."))
    print(color(BOLD, f"{'='*70}"))

    # Group by test for cleaner output.
    by_test = defaultdict(list)
    for entry in missing:
        by_test[entry["test_name"]].append(entry)

    for test_name in sorted(by_test):
        entries = by_test[test_name]
        print(f"\n  {color(CYAN, test_name)}")
        for entry in entries:
            rate_str = f"{entry['fail_count']}/{entry['total_runs']} runs"
            suite_str = color(DIM, f"[{entry['suite']}]")
            suggested = suggest_xfail_expr(entry["features"])
            print(f"    {suite_str} Failed {rate_str}")
            print(f"    {color(YELLOW, f'→ # XFAIL: {suggested}')}")


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--vendor",
        choices=["intel", "amd", "nvidia", "qc", "apple"],
        default=None,
        help="Only check workflows for a specific vendor (default: all).",
    )
    parser.add_argument(
        "--runs", "-n",
        type=int,
        default=5,
        help="Number of recent runs to check per workflow (default: 5).",
    )
    parser.add_argument(
        "--branch",
        default="main",
        help="Branch to check scheduled runs against (default: main).",
    )
    parser.add_argument(
        "--min-fail-rate",
        type=float,
        default=1.0,
        help="Minimum failure rate (0.0-1.0) to report as missing XFAIL (default: 1.0 = 100%%).",
    )
    parser.add_argument(
        "--stale-only",
        action="store_true",
        help="Only report stale XFAILs (skip missing XFAIL detection).",
    )
    parser.add_argument(
        "--missing-only",
        action="store_true",
        help="Only report missing XFAILs (skip stale XFAIL detection).",
    )
    parser.add_argument(
        "--test-dir",
        type=pathlib.Path,
        default=None,
        help="Path to test/ directory (default: auto-detect from script location).",
    )
    args = parser.parse_args()

    # Locate the test directory.
    script_dir = pathlib.Path(__file__).resolve().parent
    test_dir = args.test_dir or (script_dir.parent / "test")
    if not test_dir.is_dir():
        print(f"Error: test directory not found: {test_dir}", file=sys.stderr)
        sys.exit(1)

    print(color(BOLD, "XFAIL Sync — Comparing annotations vs. CI results"))
    print(color(DIM, f"Test directory: {test_dir}"))
    print()

    # Step 1: Scan local annotations.
    print("Scanning test annotations...")
    annotations = scan_test_annotations(test_dir)
    total_annotations = sum(len(v) for v in annotations.values())
    xfail_count = sum(
        1 for annots in annotations.values() for a in annots if a.kind == "XFAIL"
    )
    unsupported_count = total_annotations - xfail_count
    print(f"  Found {xfail_count} XFAIL and {unsupported_count} UNSUPPORTED "
          f"annotations across {len(annotations)} test files.")
    print()

    # Step 2: Fetch CI results.
    print("Fetching workflow list...")
    try:
        workflows = get_workflows()
    except subprocess.CalledProcessError as e:
        print(f"Error fetching workflows: {e}", file=sys.stderr)
        print("Make sure `gh` is authenticated: gh auth login", file=sys.stderr)
        sys.exit(1)

    result_map = fetch_workflow_results(
        workflows,
        vendor_filter=args.vendor,
        num_runs=args.runs,
        branch=args.branch,
    )

    if not result_map:
        print(color(YELLOW, "No test results collected. Cannot compare."))
        sys.exit(0)

    total_results = sum(len(v) for v in result_map.values())
    suites_seen = set(suite for suite, _ in result_map.keys())
    print(f"Collected {total_results} test results across suites: "
          f"{', '.join(sorted(suites_seen))}")
    print()

    # Step 3: Analysis.
    if not args.missing_only:
        stale = find_stale_xfails(annotations, result_map)
        print_stale_xfails(stale)

    if not args.stale_only:
        missing = find_missing_xfails(
            annotations, result_map, min_fail_rate=args.min_fail_rate
        )
        print_missing_xfails(missing)

    # Summary.
    print(color(BOLD, f"\n{'='*70}"))
    if not args.missing_only:
        stale_n = len(stale)
        print(f"  Stale XFAILs:   {color(GREEN if stale_n == 0 else YELLOW, str(stale_n))}")
    if not args.stale_only:
        missing_n = len(missing)
        print(f"  Missing XFAILs: {color(GREEN if missing_n == 0 else RED, str(missing_n))}")
    print(color(BOLD, f"{'='*70}"))

    return 0


if __name__ == "__main__":
    sys.exit(main())
