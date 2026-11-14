#!/usr/bin/env python3
"""Generate JSON groups from equivalent REQUIRES expressions without running tests. Use lit to discover tests and preserve UNSUPPORTED and XFAIL as test metadata."""

import argparse
import json
import os
import sys
import tempfile
from pathlib import Path

import lit.discovery
import lit.formats
from lit import TestRunner
from lit.BooleanExpression import BooleanExpression
from lit.LitConfig import LitConfig
from lit.Test import TestSuite
from lit.TestingConfig import TestingConfig

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument(
    "--output",
    metavar="FILE",
    type=Path,
    help="write JSON to FILE (default: stdout)",
)
parser.add_argument(
    "source_root",
    nargs="?",
    type=Path,
    default=Path(__file__).resolve().parents[1] / "test",
    help="source test directory (default: this repository's test directory)",
)
args = parser.parse_args()
MAX_VARIABLES = 16
MAX_NORMALIZATION_WORK = 1_000_000
NO_REQUIREMENTS = "No requirements"


def parse_expression(text):
    """Parse a REQUIRES expression using lit's tokenizer, rejecting regexes."""
    if "{{" in text:
        raise ValueError("regex requirements are not supported: %r" % text)

    parser = BooleanExpression(text, set())

    def parse_not():
        if parser.accept("!"):
            return {"op": "not", "operand": parse_not()}
        if parser.accept("("):
            node = parse_or()
            parser.expect(")")
            return node
        if not BooleanExpression.isMatchExpression(parser.token):
            raise ValueError(
                "expected: '!', '(' or identifier\nhave: %s"
                % parser.quote(parser.token)
            )
        node = parser.token
        parser.token = next(parser.tokens)
        return node

    def parse_and():
        operands = [parse_not()]
        while parser.accept("&&"):
            operands.append(parse_not())
        return (
            operands[0] if len(operands) == 1 else {"op": "and", "operands": operands}
        )

    def parse_or():
        operands = [parse_and()]
        while parser.accept("||"):
            operands.append(parse_and())
        return operands[0] if len(operands) == 1 else {"op": "or", "operands": operands}

    try:
        parser.token = next(parser.tokens)
        node = parse_or()
        parser.expect(BooleanExpression.END)
        return node
    except ValueError as error:
        raise ValueError("%s\nin expression: %r" % (error, text)) from error


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
    """Evaluate every feature combination and store the results as bits. Each bit records whether the expression is true for that feature combination."""
    assignment_count = 1 << len(variables)
    all_assignments = (1 << assignment_count) - 1
    feature_masks = {}
    for feature_index, name in enumerate(variables):
        bits = 0
        for assignment in range(assignment_count):
            if (assignment >> feature_index) & 1:
                bits |= 1 << assignment
        feature_masks[name] = bits

    def evaluate(current_node):
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
    """Return the indices of feature combinations where the expression is true. Each index uses one bit per feature to represent its value."""
    return [
        assignment
        for assignment in range(1 << variable_count)
        if (table >> assignment) & 1
    ]


def merge_matching_terms(minterms, variable_count):
    """Merge terms that differ in one feature and return those that cannot merge further. Each (value, mask) pair marks ignored features with set mask bits."""
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
    return variable_count - bin(implicant[1]).count("1")


def select_group_terms(minterms, variable_count):
    """Choose terms covering every matching combination, starting with uniquely needed terms. Prefer more remaining combinations, fewer literals, then lower (value, mask), without guaranteeing the smallest result."""
    primes = sorted(merge_matching_terms(minterms, variable_count))
    work = len(primes) * len(minterms)
    if work > MAX_NORMALIZATION_WORK:
        raise ValueError("Boolean expression is too complex to normalize")
    remaining = set(minterms)
    chosen = []
    for minterm in minterms:
        candidates = [implicant for implicant in primes if covers(implicant, minterm)]
        if len(candidates) == 1 and candidates[0] not in chosen:
            chosen.append(candidates[0])
    for implicant in chosen:
        remaining -= {minterm for minterm in remaining if covers(implicant, minterm)}
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
    """Build a stable name from sorted features using AND and OR, or 'true' and '!true' for constant results. The name is not necessarily the shortest expression."""
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
    """Combine requirements with AND and return a grouping key, name, and relevant features, treating empty requirements as true. The key uses sorted relevant features and their truth table so equivalent expressions match."""
    nodes = [parse_expression(text) for text in requires]
    names = set()
    for node in nodes:
        collect_variables(node, names)
    names = sorted(names)
    if len(names) > MAX_VARIABLES:
        raise ValueError(
            "REQUIRES uses %d distinct features (max %d)" % (len(names), MAX_VARIABLES)
        )
    combined = {"op": "and", "operands": nodes}
    table = truth_table(combined, names)
    support = relevant_variables(table, names)
    # Irrelevant variables don't affect the result, so re-evaluate over the support
    # (truth_table treats variables outside its list as false).
    table = truth_table(combined, support)
    return (tuple(support), table), build_group_name(support, table), support


def discover_tests(source_root):
    """Discover tests using local lit settings without loading root or nested suite configurations."""
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
    lit_config.config_names = []
    lit_config.site_config_names = []
    config = TestingConfig.fromdefaults(lit_config)
    config.name = "OffloadTest"
    config.test_format = lit.formats.ShTest()
    config.suffixes = [".test", ".yaml"]
    config.excludes = [
        "Inputs",
        "CMakeLists.txt",
        "README.txt",
        "LICENSE.txt",
    ]
    config.test_source_root = config.test_exec_root = str(source_root)

    suite = TestSuite(config.name, str(source_root), str(source_root), config)
    tests = list(lit.discovery.getTestsInSuite(suite, (), lit_config, {}, {}))
    if lit_config.numErrors:
        raise ValueError("lit reported errors during test discovery")
    if not tests:
        raise ValueError("did not discover any tests in %s" % source_root)
    return sorted(tests, key=lambda test: test.path_in_suite)


def describe_test(test):
    """Read test directives without checking feature availability or removing tests marked config.unsupported. Require a RUN directive but do not execute it."""
    entry = {
        "name": test.getFullName(),
        "suite": test.suite.name,
        "path_in_suite": list(test.path_in_suite),
        "requires": list(test.requires),
        "xfail": list(test.xfails),
        "unsupported": list(test.unsupported),
    }
    try:
        parsed = TestRunner._parseKeywords(test.getSourcePath(), require_script=True)
        entry["requires"] += parsed["REQUIRES:"] or []
        entry["xfail"] += parsed["XFAIL:"] or []
        entry["unsupported"] += parsed["UNSUPPORTED:"] or []
    except (ValueError, OSError) as error:
        raise ValueError("%s: %s" % (entry["name"], error)) from error
    return entry


def build_requires_groups(entries):
    """Group equivalent requirements, add group names to entries, and collect paths relative to the suite. Requirements that are always true join 'No requirements', while those that are always false remain in '!true'."""
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


def generate_test_groups(source_root):
    entries = [describe_test(test) for test in discover_tests(source_root)]
    groups = build_requires_groups(entries)
    return {"requires_groups": groups, "tests": entries}


def write_json(result, destination):
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
            delete=False,
        ) as stream:
            temporary = Path(stream.name)
            json.dump(result, stream, indent=2)
            stream.write("\n")
        os.replace(temporary, destination)
    finally:
        if temporary is not None and temporary.exists():
            temporary.unlink()


try:
    result = generate_test_groups(args.source_root.resolve())
    write_json(result, args.output)
except (ValueError, OSError) as error:
    parser.exit(1, "error: %s\n" % error)
