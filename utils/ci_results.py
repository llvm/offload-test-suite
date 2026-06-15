#!/usr/bin/env python3
"""Utilities to inspect and root cause recent CI results.
"""

import argparse
import bisect
import json
import pathlib
import re
import shutil
import subprocess
import sys
import textwrap


class CIResultsError(Exception):
    pass


def main(argv):
    parser = get_argument_parser(argv[0])
    args = parser.parse_args(argv[1:])

    if not shutil.which('gh'):
        print('error: gh utility not found, cannot continue')
        return 1
    try:
        args.func(args)
    except CIResultsError as e:
        print(f'error: {e}', file=sys.stderr)
        return 1
    except subprocess.CalledProcessError as e:
        print(f'error: {e}', file=sys.stderr)
        return 1

    return 0


class ArgumentParserWithSubcommandUsage(argparse.ArgumentParser):
    """Version of argparse.ArgumentParser that prints usage of each subcommand.
    """

    def format_usage(self):
        lines = []
        has_subparsers = False
        for action in self._actions:
            if isinstance(action, argparse._SubParsersAction):
                if not has_subparsers:
                    lines.append('usage:')
                    has_subparsers = True
                for choice, subparser in action.choices.items():
                    usage = subparser.format_usage().strip()[len('usage: '):]
                    lines.append(f'       {usage}')
        if not has_subparsers:
            lines.append(super().format_usage().strip())
        lines.append('')
        return '\n'.join(lines)


def get_argument_parser(prog_name):
    parser = ArgumentParserWithSubcommandUsage(
        prog_name, description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    subparsers = parser.add_subparsers(required=True)

    parser_status = subparsers.add_parser(
        'current-status',
        help=summarize_docstring(analyze_current_status),
        description=dedent_docstring(analyze_current_status),
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser_status.add_argument(
        'workflows', nargs='*',
        help='Limit to particular workflow .yaml files')
    parser_status.add_argument(
        '--regressions', action='store_true',
        help='Only show workflows that have regressed')
    parser_status.set_defaults(
        func=lambda args: analyze_current_status(
            args.workflows, regressions_only=args.regressions))

    parser_range = subparsers.add_parser(
        'failure-range',
        help=summarize_docstring(find_failure_range),
        description=dedent_docstring(find_failure_range),
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser_range.add_argument(
        'workflow',
        help='Workflow .yaml file to inspect')
    parser_range.add_argument(
        'test_path',
        help='Test case to narrow down range for')
    parser_range.add_argument(
        '--run-limit', type=int, default=100,
        help='Number of runs to consider in search (default: %(default)s)')
    parser_range.add_argument(
        '--old-result', default='XFAIL|PASS',
        help='"old" state results, separated by `|` (default: %(default)s)')
    parser_range.add_argument(
        '--new-result', default='FAIL|XPASS',
        help='"new" state results, separated by `|` (default: %(default)s)')
    parser_range.add_argument(
        '--old-runid', type=int,
        help='Run ID to start searching from')
    parser_range.add_argument(
        '--new-runid', type=int,
        help='Run ID to search up to')
    parser_range.set_defaults(
        func=lambda args: find_failure_range(
            args.workflow, args.test_path, run_limit=args.run_limit,
            old_result=args.old_result, new_result=args.new_result,
            old_runid=args.old_runid, new_runid=args.new_runid))

    return parser


def summarize_docstring(function):
    return function.__doc__.split('\n')[0]


def dedent_docstring(function):
    lines = function.__doc__.split('\n')
    return '\n'.join([lines[0], textwrap.dedent('\n'.join(lines[1:]))])


def analyze_current_status(workflows, regressions_only=False):
    """Show the current CI status for one or more workflows.

    For each given workflow, show the status of the most recent run and
    indicate any failing or unexpectedly passing tests. If no workflows are
    provided, heuristically attempt to run all workflows.

    If `regressions_only` is set, runs that have never succeeded will be
    omitted.
    """
    if not workflows:
        for path in pathlib.Path(".github/workflows").glob("*.yaml"):
            if any((path.name.startswith("build-"),
                    path.name.startswith("pr-"),
                    path.name.startswith("validate-"))):
                continue
            workflows.append(path.name)
        workflows.sort(key=workflow_status_key)
    if not workflows:
        raise CIResultsError(
            f'No workflows found in .github/workflows/. '
            f'Please run from the top level directory of the repository.')

    printer = ResultPrinter()

    for workflow in workflows:
        project = get_project_for_workflow(workflow)

        last_completed = get_last_run(workflow, status='completed')
        last_success = get_last_run(workflow, status='success')

        if regressions_only and not last_success:
            if not last_success:
                continue
            if last_completed['databaseId'] == last_success['databaseId']:
                continue

        if last_success:
            proc = get_log_proc(last_success['databaseId'])
            success_hash = read_until_githash(proc, project)
            suite_success_hash = read_until_githash(proc,
                                                    'llvm/offload-test-suite')
            proc.terminate()
        else:
            success_hash = None
            suite_success_hash = None

        printer.print_header(workflow, last_completed['conclusion'])
        printer.print_metadata(f"Name: {last_completed['name']}")
        printer.print_metadata(f"Timestamp: {last_completed['createdAt']}")

        if (last_success and
            last_completed['databaseId'] == last_success['databaseId']):
            # The most recent run passed, we have nothing more to do.
            printer.print_commit(project, success_hash)
            printer.print_commit('llvm/offload-test-suite', suite_success_hash)
            printer.printline()
            continue

        proc = get_log_proc(last_completed['databaseId'])
        failed_hash = read_until_githash(proc, project)
        suite_failed_hash = read_until_githash(proc, 'llvm/offload-test-suite')

        printer.print_commit(project, failed_hash)
        if success_hash and failed_hash:
            printer.print_commit_range(project, success_hash, failed_hash)
        printer.print_commit('llvm/offload-test-suite', suite_failed_hash)
        if success_hash and failed_hash:
            printer.print_commit_range('llvm/offload-test-suite',
                                       suite_success_hash, suite_failed_hash)
        printer.printline()

        test_re = re.compile(r'\b(?P<result>XPASS|FAIL): .* :: (?P<test>.*) \(')
        for line in proc.stdout:
            found = test_re.search(line)
            if found:
                printer.print_result(found.group('test'), found.group('result'))
        proc.wait()
        printer.printline()


def get_project_for_workflow(workflow):
    if '-clang-' in workflow:
        return 'llvm/llvm-project'
    if '-dxc-' in workflow:
        return 'Microsoft/DirectXShaderCompiler'
    raise CIResultsError(f'Workflow {workflow} is neither clang nor dxc')


def workflow_status_key(workflow):
    parts = workflow[:-len('.yaml')].split('-')
    if not parts:
        return None
    parts.reverse()

    host = parts.pop()
    if host == 'macos':
        target = 'metal'
    else:
        target = parts.pop()
    compiler = parts.pop()
    driver = parts.pop()

    # We label warp warp-d3d12 and the variant warp-preview-d3d12.
    if driver == 'warp' and parts[-1] == 'd3d12':
        parts.pop()

    variant = True if parts else False

    # Match the order that we print the results in the offload test suite
    # README for easier correlation. Unfortunately I don't see an obvious way
    # to automate this.
    tier_list = [
        ('d3d12', 'intel'),
        ('d3d12', 'nvidia'),
        ('warp', 'amd'),
        ('warp', 'qc'),
        ('vk', 'intel'),
        ('mtl', 'metal'),
        ('d3d12', 'amd'),
        ('d3d12', 'qc'),
        ('vk', 'amd'),
        ('vk', 'nvidia'),
        ('vk', 'qc'),
    ]
    try:
        tier = tier_list.index((driver, target))
    except ValueError:
        tier = len(tier_list)

    compiler_key = 0 if compiler == 'dxc' else 1

    return (variant, tier, driver, target, host, compiler_key)


def find_failure_range(workflow, test_path, *,
                       run_limit, old_result, new_result,
                       old_runid=None, new_runid=None):
    """Find the git range where a test started to fail.

    Given a workflow and test path, attempt to find the CI run where it first
    started failing and report the git ranges from the previous success. These
    ranges can then be used to further investigate or bisect locally.

    If old_runid is not provided, the search will trot backwards from new_runid
    or the latest run until it finds a success to start bisecting from. This
    will abort if we exceed run_limit.

    old_result and new_result may be set to regexes to search for test status
    changes other than failure and unexpected passes.
    """

    # Sanitize result arguments
    old_result = re.compile(
        '|'.join(re.escape(x) for x in old_result.split('|')))
    new_result = re.compile(
        '|'.join(re.escape(x) for x in new_result.split('|')))

    # Lookup the runs we're going to work with.
    # TODO: Use paging instead of a run_limit?
    runs = get_recent_runs(workflow, run_limit=run_limit)
    if not runs:
        raise CIResultsError(f'could not find any runs for {workflow}')

    runids = [run['databaseId'] for run in runs]
    start_index = runid_index(runids, new_runid) if new_runid else 0
    end_index = runid_index(runids, old_runid) if old_runid else 0

    project = get_project_for_workflow(workflow)

    # Sanity check that the starting index has the right state
    new_hash, result = get_test_result(runids[start_index], project, test_path)
    print(f'{start_index} - '
          f'Run {runids[start_index]} ({new_hash}): {result}',
          file=sys.stderr)
    if not new_result.match(result):
        raise CIResultsError(
            f'Current result is {result}, not {new_result.pattern}')

    if end_index == 0:
        # We don't have a range yet. "Gallop" until we find an end index.
        offset = 1
        while start_index + offset < len(runids):
            end_index = start_index + offset
            old_hash, result = get_test_result(
                runids[end_index], project, test_path)
            print(f'{end_index} - '
                  f'Run {runids[end_index]} ({old_hash}): {result}',
                  file=sys.stderr)
            if old_result.match(result):
                break
            if not new_result.match(result):
                raise CIResultsError(
                    f'Unhandled result in run {runids[end_index]}: {result}')
            start_index = end_index
            new_hash = old_hash
            offset *= 2
        else:
            raise CIResultsError(
                f'Did not find run with result matching {old_result.pattern}. '
                f'Try higher --run-limit and --new-runid={runids[start_index]}')
    else:
        # Sanity check that the end index has the right state
        old_hash, result = get_test_result(
            runids[end_index], project, test_path)
        print(f'{end_index} - '
              f'Run {runids[end_index]} ({old_hash}): {result}',
              file=sys.stderr)
        if not old_result.match(result):
            raise CIResultsError(
                f'Old result is {result}, not {old_result.pattern}')

    # Finally, we can bisect the logs.
    while end_index > start_index + 1:
        current_index = int((start_index + end_index) / 2)
        git_hash, result = get_test_result(
            runids[current_index], project, test_path)

        print(f'{current_index} - '
              f'Run {runids[current_index]} ({git_hash}): {result}',
              file=sys.stderr)
        if old_result.match(result):
            old_hash = git_hash
            end_index = current_index
        elif new_result.match(result):
            new_hash = git_hash
            start_index = current_index
        else:
            raise CIResultsError(
                f'Unhandled result in run {runids[end_index]}: {result}')

    if (end_index != start_index + 1):
        raise CIResultsError(
            f'Bisection ended early? Range is {start_index} to {end_index}')

    print()
    print(f'{project} range: {old_hash.project_hash}..{new_hash.project_hash}')
    print(f'test suite range: {old_hash.offload_hash}..{new_hash.offload_hash}')


def runid_index(runids, runid):
    try:
        return runids.index(runid)
    except ValueError:
        raise CIResultsError(
            f'Could not find runid ({runid}). Try raising `run_limit`.')


def get_last_run(workflow, status='completed'):
    output = subprocess.run(
        ['gh', 'run', '-R', 'llvm/offload-test-suite', 'list',
         '--workflow', workflow, '--status', status,
         '--json', 'name,databaseId,createdAt,conclusion',
         '--jq', 'max_by(.createdAt)'],
        check=True, stdout=subprocess.PIPE).stdout.strip()
    if not output:
        return None
    return json.loads(output)


def get_recent_runs(workflow, *, run_limit):
    output = subprocess.run(
        ['gh', 'run', '-R', 'llvm/offload-test-suite', 'list',
         '-L', str(run_limit), '--workflow', workflow, '--status', 'completed',
         '--json', 'name,databaseId,createdAt,conclusion'],
        check=True, stdout=subprocess.PIPE).stdout
    if not output:
        return None
    return json.loads(output)


def get_log_proc(databaseId):
    return subprocess.Popen(
        ['gh', 'run', '-R', 'llvm/offload-test-suite',
         'view', '--log', str(databaseId)],
        text=True, bufsize=1, stdout=subprocess.PIPE)


def read_until_githash(proc, repo):
    checkout_action_re = re.compile(r'Run actions/checkout')
    repo_re = re.compile(f'repository: {repo}')
    git_log_hash_re = re.compile('log -1 --format=%H')
    hash_re = re.compile('[0-9a-f]{40}')

    state = None
    for line in proc.stdout:
        if checkout_action_re.search(line):
            state = 'checkout'
        elif state == 'checkout' and repo_re.search(line):
            state = 'repo'
        elif state == 'repo':
            if git_log_hash_re.search(line):
                state = 'hash'
        elif state == 'hash':
            found = hash_re.search(line)
            if not found:
                raise CIResultsError('Hash not printed from git log command?')
            return found.group(0)
    return None


class Hashes(object):
    def __init__(self, project, project_hash, offload_hash):
        self.project = project
        # Note: Without looking at the repos we can't actually calculate what's
        # sufficiently long for a short hash, so we just use something large.
        self.project_hash = project_hash[:12]
        self.offload_hash = offload_hash[:12]

    def __repr__(self):
        return (f'Hashes(project={self.project}, '
                f'project_hash={self.project_hash}, '
                f'offload_hash={self.offload_hash})')

    def __str__(self):
        return ', '.join([f"{self.project}: {self.project_hash or '-'}",
                          f"test-suite: {self.offload_hash or '-'}"])


def get_test_result(databaseId, project, test_path):
    proc = get_log_proc(databaseId)
    project_hash = read_until_githash(proc, project)
    offload_hash = read_until_githash(proc, 'llvm/offload-test-suite')
    if not project_hash or not offload_hash:
        raise CIResultsError(f'Failed to find repo hashes for run {databaseId}')

    test_re = re.compile(
        r'\b(?P<result>PASS|XPASS|FAIL|XFAIL|UNSUPPORTED): .* :: ' +
        re.escape(test_path) + r' \(')
    result = None
    for line in proc.stdout:
        found = test_re.search(line)
        if found:
            result = found.group('result')
            break
    proc.terminate()

    return Hashes(project, project_hash, offload_hash), result


class ResultPrinter:
    def __init__(self):
        self.use_color = sys.stdout.isatty()
        self.bold = '\033[1m' if self.use_color else ''
        self.reset = '\033[0m' if self.use_color else ''
        self.red = '\033[0;31m' if self.use_color else ''
        self.green = '\033[0;32m' if self.use_color else ''
        self.yellow = '\033[0;33m' if self.use_color else ''

    def print_header(self, job_name, status):
        status_color = self.yellow
        if status == 'success':
            status_color = self.green
        elif status == 'failure':
            status_color = self.red

        print(f'{self.bold}## {job_name}{self.reset} '
              f'({status_color}{status}{self.reset})')

    def print_metadata(self, info):
        print(f' - {info}')

    def print_commit(self, project, commit):
        print(f" - {project}: {commit or 'unknown'}")

    def print_commit_range(self, project, success_hash, failed_hash):
        print(f' - {project} range: {success_hash[:12]}..{failed_hash[:12]}')

    def print_result(self, test, result):
        color = self.reset
        if result == 'FAIL':
            color = self.red
        elif result == 'XPASS':
            color = self.yellow
        print(f"{color}{result}{self.reset}: {test}")

    def printline(self):
        print()


if __name__ == '__main__':
    sys.exit(main(sys.argv))
