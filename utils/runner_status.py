"""
Show runner job status across all workflows in llvm/offload-test-suite.

Includes queued/in-progress runs and recently completed runs so you
can see what is (or was) occupying the runners.

Requires the GitHub CLI (`gh`) to be installed and authenticated
(`gh auth login`). API calls are issued through `gh api`.

Usage:
    python runner_status.py [vendor]

    vendor: intel | amd | nvidia | qc   (omit to show all vendors)
"""

import sys
import os
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone, timedelta

OWNER = "llvm"
REPO = "offload-test-suite"
VALID_VENDORS = ("intel", "amd", "nvidia", "qc")
COMPLETED_WINDOW_HOURS = 3

# ANSI color codes per vendor
VENDOR_COLORS = {
    "intel": "\033[34m",  # blue
    "amd": "\033[31m",  # red
    "nvidia": "\033[32m",  # green
    "qc": "\033[90m",  # gray
}
RESET = "\033[0m"

# Workflow names that are exclusive to a specific vendor
VENDOR_WORKFLOW_KEYWORDS = {
    "intel": "intel",
    "amd": "amd",
    "nvidia": "nvidia",
    "qc": "qc",
}


def runner_label(vendor):
    return f"hlsl-windows-{vendor}"


def colorize(vendor, text):
    """Wrap text in the vendor's ANSI color."""
    c = VENDOR_COLORS.get(vendor, "")
    return f"{c}{text}{RESET}" if c else text


def api_get(path):
    """Issue a GitHub API GET via `gh api` and return the parsed JSON.

    Raises subprocess.CalledProcessError on non-zero exit (e.g. 403).
    """
    result = subprocess.run(
        ["gh", "api", "-H", "Accept: application/vnd.github+json", path],
        capture_output=True,
        text=True,
        encoding="utf-8",
        check=True,
    )
    return json.loads(result.stdout)


def get_runners(label):
    """Fetch self-hosted runners that have the given label. Returns None on error."""
    try:
        path = f"/repos/{OWNER}/{REPO}/actions/runners?per_page=100"
        runners = api_get(path).get("runners", [])
        return [r for r in runners if label in [l["name"] for l in r.get("labels", [])]]
    except subprocess.CalledProcessError:
        return None


def run_could_match_vendor(run, vendor):
    """Quick heuristic: can this run possibly have jobs for the given vendor?

    Scheduled/dispatch runs whose workflow name contains another vendor's
    keyword are skipped. PR runs (Execution Testing) and ambiguous runs
    are always kept.
    """
    name_lower = run["name"].lower()
    # "Execution Testing" (PR matrix) always includes intel, sometimes others
    if "execution testing" in name_lower or "hlsl test" in name_lower:
        return True
    # If the workflow name mentions a specific vendor, only match that one
    for v, kw in VENDOR_WORKFLOW_KEYWORDS.items():
        if kw in name_lower:
            return v == vendor
    return True


def get_runs(vendors):
    """Fetch runs that are queued, in_progress, or recently completed.

    When a single vendor is requested, only fetches runs whose workflow
    could plausibly contain jobs for that vendor.
    """
    results = []
    for status in ("queued", "in_progress"):
        path = f"/repos/{OWNER}/{REPO}/actions/runs?status={status}&per_page=100"
        results.extend(api_get(path)["workflow_runs"])

    # Also grab recently completed runs (within COMPLETED_WINDOW_HOURS)
    cutoff = datetime.now(timezone.utc) - timedelta(hours=COMPLETED_WINDOW_HOURS)
    path = f"/repos/{OWNER}/{REPO}/actions/runs?status=completed&per_page=50"
    for r in api_get(path)["workflow_runs"]:
        updated = datetime.fromisoformat(r["updated_at"].replace("Z", "+00:00"))
        if updated >= cutoff:
            results.append(r)

    # Deduplicate by run ID
    seen = set()
    unique = []
    for r in results:
        if r["id"] not in seen:
            seen.add(r["id"])
            unique.append(r)

    # Pre-filter: if only one vendor requested, skip runs that clearly
    # belong to a different vendor (avoids fetching their jobs).
    if len(vendors) == 1:
        vendor = vendors[0]
        unique = [r for r in unique if run_could_match_vendor(r, vendor)]

    return unique


def prefetch_jobs(runs, jobs_cache):
    """Fetch jobs for all runs in parallel to minimize wall-clock time."""
    to_fetch = [r for r in runs if r["id"] not in jobs_cache]
    if not to_fetch:
        return

    def fetch_one(run_id):
        return run_id, get_jobs(run_id)

    with ThreadPoolExecutor(max_workers=8) as pool:
        futures = {pool.submit(fetch_one, r["id"]): r["id"] for r in to_fetch}
        for future in as_completed(futures):
            run_id, jobs = future.result()
            jobs_cache[run_id] = jobs


def get_jobs(run_id):
    path = f"/repos/{OWNER}/{REPO}/actions/runs/{run_id}/jobs?per_page=100"
    return api_get(path)["jobs"]


def job_sku_vendor(job):
    """Return the vendor implied by the matrix SKU in the job name, or None.

    Matrix jobs embed their SKU (e.g. "windows-intel") in the job name, e.g.
    "Exec-Tests-Windows (windows-intel, check-hlsl-vk) / build". Since the
    split-build feature was enabled, the build phase runs on whatever generic
    self-hosted Windows runner is free (its `runner_name` is empty while
    queued and arbitrary while running), so the runner is no longer a reliable
    vendor signal. The SKU baked into the job name always is.
    """
    name_lower = (job.get("name") or "").lower()
    for v in VALID_VENDORS:
        if f"windows-{v}" in name_lower:
            return v
    return None


def job_matches_vendor(job, vendor, label):
    """Decide whether a job belongs to the given vendor.

    Prefer the SKU embedded in the job name: this correctly attributes
    split-build jobs (which may run on, or be queued for, any runner) and
    avoids misattributing them based on the runner they happen to land on.
    Only when the job name carries no SKU do we fall back to the SKU runner
    label or runner name (covers workflow_dispatch / older workflows that did
    not embed the SKU in the job name).
    """
    sku_vendor = job_sku_vendor(job)
    if sku_vendor is not None:
        return sku_vendor == vendor
    return label in job.get("labels", []) or vendor.lower() in (
        job.get("runner_name") or ""
    ).lower()


def tz_abbrev(dt):
    """Get short timezone abbreviation, e.g. 'PDT' instead of 'Pacific Daylight Time'."""
    name = dt.strftime("%Z")
    if len(name) <= 5:
        return name
    return "".join(w[0] for w in name.split())


def format_time(iso_str):
    """Convert ISO timestamp to short time like '12:40 PM PDT / 7:40 PM UTC'."""
    dt = datetime.fromisoformat(iso_str.replace("Z", "+00:00"))
    h = dt.hour % 12 or 12
    ampm = "AM" if dt.hour < 12 else "PM"
    utc = f"{h}:{dt.minute:02d} {ampm} UTC"
    local = dt.astimezone()
    lh = local.hour % 12 or 12
    lampm = "AM" if local.hour < 12 else "PM"
    tz = tz_abbrev(local)
    return f"{lh}:{local.minute:02d} {lampm} {tz} / {utc}"


def print_vendor_table(vendor, runs, jobs_cache, runners_cache):
    """Print the status table for a single vendor. Returns True if any rows."""
    label = runner_label(vendor)

    # Fetch and cache runners for this label
    if label not in runners_cache:
        runners_cache[label] = get_runners(label)
    runners = runners_cache[label]
    if runners is not None:
        online = len([r for r in runners if r.get("status") == "online"])
        runner_info = f", {online}/{len(runners)} online"
    else:
        runner_info = ""

    rows = []
    active_details = []

    for run in runs:
        run_id = run["id"]
        if run_id not in jobs_cache:
            jobs_cache[run_id] = get_jobs(run_id)
        jobs = jobs_cache[run_id]

        # Match by SKU in the job name (reliable for split-build jobs), with
        # a fallback to the SKU runner label / runner name for jobs that don't
        # embed the SKU in their name.
        vendor_jobs = [j for j in jobs if job_matches_vendor(j, vendor, label)]
        if not vendor_jobs:
            continue

        title = run["display_title"]
        workflow = run["name"]
        created = format_time(run["created_at"])

        done = len([j for j in vendor_jobs if j["status"] == "completed"])
        active = [j for j in vendor_jobs if j["status"] == "in_progress"]
        queued = len([j for j in vendor_jobs if j["status"] == "queued"])

        # Skip runs where all jobs are done (nothing active or queued)
        if not active and queued == 0:
            continue

        if run["event"] == "schedule":
            prefix = "[Scheduled]"
        elif run["event"] == "pull_request":
            prefix = "[PR]"
        else:
            prefix = f"[{run['event']}]"
        run_label = f"{prefix} {title} ({created})"
        rows.append((run_label, workflow, done, len(active), queued))

        for j in active:
            short = j["name"].split(",")[-1].strip().rstrip(") / build")
            active_details.append((title, short, j.get("runner_name", "?")))

    header_text = f"=== {vendor.upper()} (runner: {label}{runner_info}) ==="
    print(colorize(vendor, header_text))
    print()

    if not rows:
        print(f"No runs with {vendor} jobs found.\n")
        return False

    now_local = datetime.now().astimezone()
    local_str = now_local.strftime("%#I:%M %p ") + tz_abbrev(now_local)
    utc_str = now_local.astimezone(timezone.utc).strftime("%#I:%M %p UTC")
    timestamp = f"as of {local_str} / {utc_str}"

    col1_w = max(len(r[0]) for r in rows)
    col2_w = max(max(len(r[1]) for r in rows), len("Workflow"))
    run_col_header = f"Run ({timestamp})"
    col1_w = max(col1_w, len(run_col_header))
    header = (
        f"{run_col_header:<{col1_w}}  {'Workflow':<{col2_w}}"
        f"  {'Done':>6}  {'Active':>6}  {'Queued':>6}"
    )
    sep = "=" * len(header)

    print(colorize(vendor, sep))
    print(header)
    print(colorize(vendor, sep))
    for run_label, workflow, done, active, queued in rows:
        active_str = str(active) if active == 0 else f"*{active}*"
        print(
            f"{run_label:<{col1_w}}  {workflow:<{col2_w}}"
            f"  {done:>6}  {active_str:>6}  {queued:>6}"
        )
    print(colorize(vendor, sep))

    total_done = sum(r[2] for r in rows)
    total_active = sum(r[3] for r in rows)
    total_queued = sum(r[4] for r in rows)
    print(
        f"{'TOTAL':<{col1_w}}  {'':<{col2_w}}"
        f"  {total_done:>6}  {total_active:>6}  {total_queued:>6}"
    )
    print()

    if active_details:
        print(colorize(vendor, f"Currently running on {vendor}:"))
        for title, job, runner_name in active_details:
            print(f"  -> {job}  (runner: {runner_name}, run: {title})")
    else:
        print(f"No {vendor} jobs currently running.")
    print()
    return True


def main():
    if len(sys.argv) >= 2:
        vendor = sys.argv[1].lower()
        if vendor not in VALID_VENDORS:
            print(f"Unknown vendor '{vendor}'. Choose from: {', '.join(VALID_VENDORS)}")
            print(f"Usage: python {os.path.basename(__file__)} [vendor]")
            sys.exit(1)
        vendors = [vendor]
    else:
        vendors = list(VALID_VENDORS)

    runs = get_runs(vendors)
    if not runs:
        print("No queued, in-progress, or recently completed runs found.")
        return

    runs.sort(key=lambda r: r["created_at"])

    # Fetch all jobs in parallel upfront
    jobs_cache = {}
    prefetch_jobs(runs, jobs_cache)

    runners_cache = {}

    for v in vendors:
        print_vendor_table(v, runs, jobs_cache, runners_cache)


if __name__ == "__main__":
    main()
