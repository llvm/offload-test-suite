#!/usr/bin/env python3
"""Generate a JSON inventory of offload tests grouped by equivalent REQUIRES expressions.

Uses lit to discover tests and read directives without running tests.
UNSUPPORTED and XFAIL are preserved separately as test metadata.
"""

import argparse
import json
import os
import sys
import tempfile
from pathlib import Path

MAX_VARIABLES = 16
MAX_NORMALIZATION_WORK = 1_000_000
NO_REQUIREMENTS = "No requirements"


def parse_expression(text):
    """Parse a REQUIRES expression using lit, rejecting regex requirements."""
    from lit.BooleanExpression import BooleanExpression

    if "{{" in text:
        raise ValueError("regex requirements are not supported: %r" % text)

    return BooleanExpression.normalize(text)


def collect_variables(node, variables):
    """Collect feature names, excluding lit's true constant."""
    if isinstance(node, str):
        if node != "true":
            variables.add(node)
    elif isinstance(node, dict):
        if node["op"] == "not":
            collect_variables(node["operand"], variables)
        else:
            for child in node["operands"]:
                collect_variables(child, variables)


def truth_table(node, variables):
    """Pack expression results for all feature assignments into an integer."""
    assignment_count = 1 << len(variables)
    all_assignments = (1 << assignment_count) - 1
    feature_masks = {}
    # Each assignment's bits indicate which features are present.
    for feature_index, name in enumerate(variables):
        bits = 0
        for assignment in range(assignment_count):
            if (assignment >> feature_index) & 1:
                bits |= 1 << assignment
        feature_masks[name] = bits

    def evaluate(current_node):
        if isinstance(current_node, bool):
            return all_assignments if current_node else 0
        if isinstance(current_node, str):
            if current_node == "true":
                return all_assignments
            return feature_masks.get(current_node, 0)
        if current_node["op"] == "not":
            return all_assignments ^ evaluate(current_node["operand"])
        is_and = current_node["op"] == "and"
        result = all_assignments if is_and else 0
        for operand in current_node["operands"]:
            value = evaluate(operand)
            result = result & value if is_and else result | value
        return result

    return evaluate(node)


def relevant_variables(table, variables):
    """Keep only features whose value can change the expression result."""
    assignment_count = 1 << len(variables)
    result = []
    for feature_index, name in enumerate(variables):
        depends = any(
            ((table >> assignment) & 1)
            != ((table >> (assignment ^ (1 << feature_index))) & 1)
            for assignment in range(assignment_count)
        )
        if depends:
            result.append(name)
    return result


def matching_combinations(table, variable_count):
    """List the feature assignments where the expression is true."""
    return [
        assignment
        for assignment in range(1 << variable_count)
        if (table >> assignment) & 1
    ]


def merge_matching_terms(minterms, variable_count):
    """Merge satisfying assignments into terms that cannot simplify further."""
    # An implicant is (value, mask): mask bits are "don't care".
    current = {(minterm, 0) for minterm in minterms}
    primes = set()
    work = 0
    while current:
        # Bound exponential minimization even below the feature-count limit.
        work += len(current) * variable_count
        if work > MAX_NORMALIZATION_WORK:
            raise ValueError("Boolean expression is too complex to normalize")
        merged = set()
        used = set()
        by_mask = {}
        for implicant in current:
            by_mask.setdefault(implicant[1], []).append(implicant)
        for mask, implicants in by_mask.items():
            values = {value for value, _ in implicants}
            for value in values:
                for feature_index in range(variable_count):
                    bit = 1 << feature_index
                    if mask & bit or value & bit:
                        continue
                    if (value | bit) in values:
                        merged.add((value, mask | bit))
                        used.add((value, mask))
                        used.add((value | bit, mask))
        primes |= current - used
        current = merged
    return primes


def covers(implicant, minterm):
    """Check whether an assignment matches a term's unmasked features."""
    value, mask = implicant
    return (minterm & ~mask) == value


def literal_count(implicant, variable_count):
    """Count the features a term requires to be present or absent."""
    return variable_count - bin(implicant[1]).count("1")


def select_group_terms(minterms, variable_count):
    """Choose a repeatable set of terms covering every satisfying assignment."""
    primes = sorted(merge_matching_terms(minterms, variable_count))
    work = len(primes) * len(minterms)
    if work > MAX_NORMALIZATION_WORK:
        raise ValueError("Boolean expression is too complex to normalize")
    remaining = set(minterms)
    chosen = []
    # Essential prime implicants first.
    for minterm in minterms:
        candidates = [implicant for implicant in primes if covers(implicant, minterm)]
        if len(candidates) == 1 and candidates[0] not in chosen:
            chosen.append(candidates[0])
    for implicant in chosen:
        remaining -= {minterm for minterm in remaining if covers(implicant, minterm)}
    # Then greedy: most newly covered minterms, fewest literals, deterministic.
    while remaining:
        work += len(primes) * len(remaining)
        if work > MAX_NORMALIZATION_WORK:
            raise ValueError("Boolean expression is too complex to normalize")
        best = max(
            (implicant for implicant in primes if implicant not in chosen),
            key=lambda implicant: (
                sum(1 for minterm in remaining if covers(implicant, minterm)),
                -literal_count(implicant, variable_count),
                tuple(-component for component in implicant),
            ),
        )
        chosen.append(best)
        remaining -= {minterm for minterm in remaining if covers(best, minterm)}
    return chosen


def build_group_name(variables, table):
    """Render a repeatable OR of AND terms using lit syntax."""
    variable_count = len(variables)
    if variable_count == 0:
        return "true" if table & 1 else "!true"
    minterms = matching_combinations(table, variable_count)
    terms = []
    for value, mask in select_group_terms(minterms, variable_count):
        literals = []
        for feature_index, name in enumerate(variables):
            if mask & (1 << feature_index):
                continue
            literals.append(name if value & (1 << feature_index) else "!" + name)
        terms.append(literals)
    terms.sort(
        key=lambda term: (
            len(term),
            [literal.lstrip("!") + literal for literal in term],
        )
    )
    return " || ".join(" && ".join(term) for term in terms)


def compute_requires_group(requires):
    """Returns (key, display_name, features) for a list of REQUIRES strings."""
    from lit.BooleanExpression import BooleanExpression

    nodes = []
    for text in requires:
        for part in text.split(","):
            part = part.strip()
            nodes.append(parse_expression(part))
    names = set()
    for node in nodes:
        collect_variables(node, names)
    names = sorted(names)
    if len(names) > MAX_VARIABLES:
        raise ValueError(
            "REQUIRES uses %d distinct features (max %d)" % (len(names), MAX_VARIABLES)
        )
    combined = BooleanExpression.combine("and", nodes)
    table = truth_table(combined, names)
    support = relevant_variables(table, names)
    # Irrelevant variables don't affect the result, so re-evaluate over the support
    # (truth_table treats variables outside its list as false).
    table = truth_table(combined, support)
    return (tuple(support), table), build_group_name(support, table), support


def discover_tests(source_root):
    """Discover suite tests without loading the runtime tool and GPU setup."""
    import lit.discovery
    import lit.formats
    from lit.LitConfig import LitConfig
    from lit.Test import TestSuite
    from lit.TestingConfig import TestingConfig

    if not source_root.is_dir():
        raise ValueError("test source directory does not exist: %s" % source_root)
    lit_config = LitConfig(
        progname="generate_test_groups",
        path=[],
        diagnostic_level="warning",
        useValgrind=False,
        valgrindLeakCheck=False,
        valgrindArgs=[],
        noExecute=True,
        debug=False,
        isWindows=sys.platform == "win32",
        order="lexical",
        params={},
    )
    config = TestingConfig.fromdefaults(lit_config)
    config.name = "OffloadTest"
    config.test_format = lit.formats.ShTest()
    # Match test/lit.cfg.py's discovery rules, without its tool/GPU setup.
    config.suffixes = [".test", ".yaml"]
    config.excludes = [
        "Inputs",
        "CMakeLists.txt",
        "README.txt",
        "LICENSE.txt",
    ]
    config.test_source_root = config.test_exec_root = str(source_root)

    # Nested suites would load runtime configuration, including hardware probes.
    for directory, children, files in os.walk(source_root):
        children[:] = [
            name
            for name in children
            if name not in config.excludes and name not in ("Output", ".svn", ".git")
        ]
        if Path(directory) != source_root and any(
            name in files
            for name in lit_config.config_names + lit_config.site_config_names
        ):
            raise ValueError("nested test suites are not supported: %s" % directory)

    suite = TestSuite(
        config.name, str(source_root), str(source_root), config, lit_config
    )
    tests = list(lit.discovery.getTestsInSuite(suite, (), lit_config, {}, {}))
    if lit_config.numErrors:
        raise ValueError("lit reported errors during test discovery")
    if not tests:
        raise ValueError("did not discover any tests in %s" % source_root)
    return sorted(tests, key=lambda test: test.path_in_suite)


def describe_test(test, test_runner):
    """Read each test's directives without evaluating its applicability."""
    entry = {
        "name": test.getFullName(),
        "suite": test.suite.name,
        "path_in_suite": list(test.path_in_suite),
        "requires": list(test.requires),
        "xfail": list(test.xfails),
        "unsupported": list(test.unsupported),
    }
    try:
        parsed = test_runner._parseKeywords(test.getSourcePath(), require_script=True)
        entry["requires"] += parsed["REQUIRES:"] or []
        entry["xfail"] += parsed["XFAIL:"] or []
        entry["unsupported"] += parsed["UNSUPPORTED:"] or []
    except (ValueError, OSError, UnicodeDecodeError) as error:
        raise ValueError("%s: %s" % (entry["name"], error)) from error
    # Local config.unsupported depends on runtime features, not this inventory.
    return entry


def build_requires_groups(entries):
    """Group tests by equivalent REQUIRES expressions using relative paths."""
    groups = {}
    for entry in entries:
        try:
            key, name, features = compute_requires_group(entry["requires"])
        except ValueError as error:
            raise ValueError("%s: %s" % (entry["name"], error)) from error
        if name == "true":
            name = NO_REQUIREMENTS
        entry["requires_group"] = name
        group = groups.setdefault(
            key, {"group": name, "features": list(features), "tests": []}
        )
        group["tests"].append("/".join(entry["path_in_suite"]))
    result = []
    for group in sorted(
        groups.values(), key=lambda group: (len(group["features"]), group["group"])
    ):
        group["tests"].sort()
        group["test_count"] = len(group["tests"])
        result.append(group)
    return result


def generate_inventory(source_root):
    """Assemble the complete test inventory and its REQUIRES groups."""
    from lit import TestRunner

    entries = [describe_test(test, TestRunner) for test in discover_tests(source_root)]
    groups = build_requires_groups(entries)
    return {"requires_groups": groups, "tests": entries}


def write_inventory(result, destination):
    """Write JSON to stdout or atomically replace the destination file."""
    if destination is None:
        json.dump(result, sys.stdout, indent=2)
        sys.stdout.write("\n")
        return
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w",
            encoding="utf-8",
            dir=destination.parent,
            prefix=destination.name + ".",
            suffix=".tmp",
            delete=False,
        ) as stream:
            temporary = Path(stream.name)
            json.dump(result, stream, indent=2)
            stream.write("\n")
        os.replace(temporary, destination)
    finally:
        if temporary is not None and temporary.exists():
            temporary.unlink()


def main():
    """Parse command-line options and report inventory errors."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--dump-json", metavar="FILE", type=Path, help="default: stdout"
    )
    parser.add_argument("--lit-path", metavar="DIR", help="directory containing lit")
    parser.add_argument(
        "source_root",
        nargs="?",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "test",
        help="source test directory (default: this repository's test directory)",
    )
    args = parser.parse_args()
    if args.lit_path:
        sys.path.insert(0, args.lit_path)

    try:
        result = generate_inventory(args.source_root.resolve())
        write_inventory(result, args.dump_json)
    except (ImportError, ValueError, OSError, UnicodeDecodeError) as error:
        parser.exit(1, "error: %s\n" % error)


if __name__ == "__main__":
    main()
