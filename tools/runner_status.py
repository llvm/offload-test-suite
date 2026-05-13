"""
Show runner job status across all workflows in llvm/offload-test-suite.

Includes queued/in-progress runs and recently completed runs so you
can see what is (or was) occupying the runners.

Usage:
    python runner_status.py <GITHUB_TOKEN> [vendor]

    vendor: intel | amd | nvidia | qc   (omit to show all vendors)
"""

import sys
import os
import urllib.request
import json
from datetime import datetime, timezone, timedelta

OWNER = "llvm"
REPO = "offload-test-suite"
VALID_VENDORS = ("intel", "amd", "nvidia", "qc")
API = "https://api.github.com"
COMPLETED_WINDOW_HOURS = 3


def runner_label(vendor):
    return f"hlsl-windows-{vendor}"


def api_get(path, token):
    url = f"{API}{path}"
    req = urllib.request.Request(url)
    req.add_header("Authorization", f"Bearer {token}")
    req.add_header("Accept", "application/vnd.github+json")
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read())


def get_runners(label, token):
    """Fetch self-hosted runners that have the given label. Returns None on 403."""
    try:
        path = f"/repos/{OWNER}/{REPO}/actions/runners?per_page=100"
        runners = api_get(path, token).get("runners", [])
        return [r for r in runners if label in [l["name"] for l in r.get("labels", [])]]
    except urllib.error.HTTPError:
        return None


def get_runs(token):
    """Fetch runs that are queued, in_progress, or recently completed."""
    results = []
    for status in ("queued", "in_progress"):
        path = (
            f"/repos/{OWNER}/{REPO}/actions/runs"
            f"?status={status}&per_page=100"
        )
        results.extend(api_get(path, token)["workflow_runs"])

    # Also grab recently completed runs (within COMPLETED_WINDOW_HOURS)
    cutoff = datetime.now(timezone.utc) - timedelta(hours=COMPLETED_WINDOW_HOURS)
    path = (
        f"/repos/{OWNER}/{REPO}/actions/runs"
        f"?status=completed&per_page=50"
    )
    for r in api_get(path, token)["workflow_runs"]:
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
    return unique


def get_jobs(run_id, token):
    path = f"/repos/{OWNER}/{REPO}/actions/runs/{run_id}/jobs?per_page=100"
    return api_get(path, token)["jobs"]


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


def print_vendor_table(vendor, runs, jobs_cache, runners_cache, token):
    """Print the status table for a single vendor. Returns True if any rows."""
    label = runner_label(vendor)

    # Fetch and cache runners for this label
    if label not in runners_cache:
        runners_cache[label] = get_runners(label, token)
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
            jobs_cache[run_id] = get_jobs(run_id, token)
        jobs = jobs_cache[run_id]

        # Match by label OR by runner name containing the vendor (covers
        # workflow_dispatch runs that didn't pin a vendor SKU label).
        vendor_jobs = [
            j for j in jobs
            if label in j.get("labels", [])
            or vendor.lower() in (j.get("runner_name") or "").lower()
        ]
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

    print(f"=== {vendor.upper()} (runner: {label}{runner_info}) ===\n")

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
    sep = "-" * len(header)

    print(sep)
    print(header)
    print(sep)
    for run_label, workflow, done, active, queued in rows:
        active_str = str(active) if active == 0 else f"*{active}*"
        print(
            f"{run_label:<{col1_w}}  {workflow:<{col2_w}}"
            f"  {done:>6}  {active_str:>6}  {queued:>6}"
        )
    print(sep)

    total_done = sum(r[2] for r in rows)
    total_active = sum(r[3] for r in rows)
    total_queued = sum(r[4] for r in rows)
    print(
        f"{'TOTAL':<{col1_w}}  {'':<{col2_w}}"
        f"  {total_done:>6}  {total_active:>6}  {total_queued:>6}"
    )
    print()

    if active_details:
        print(f"Currently running on {vendor}:")
        for title, job, runner_name in active_details:
            print(f"  -> {job}  (runner: {runner_name}, run: {title})")
    else:
        print(f"No {vendor} jobs currently running.")
    print()
    return True


def main():
    if len(sys.argv) < 2:
        print(f"Usage: python {os.path.basename(__file__)} <GITHUB_TOKEN> [vendor]")
        print(f"  vendor: {' | '.join(VALID_VENDORS)}  (omit to show all)")
        sys.exit(1)

    token = sys.argv[1]

    if len(sys.argv) >= 3:
        vendor = sys.argv[2].lower()
        if vendor not in VALID_VENDORS:
            print(f"Unknown vendor '{vendor}'. Choose from: {', '.join(VALID_VENDORS)}")
            sys.exit(1)
        vendors = [vendor]
    else:
        vendors = list(VALID_VENDORS)

    runs = get_runs(token)
    if not runs:
        print("No queued, in-progress, or recently completed runs found.")
        return

    runs.sort(key=lambda r: r["created_at"])

    # Cache jobs per run so they're only fetched once across vendors
    jobs_cache = {}
    runners_cache = {}

    for v in vendors:
        print_vendor_table(v, runs, jobs_cache, runners_cache, token)


if __name__ == "__main__":
    main()
