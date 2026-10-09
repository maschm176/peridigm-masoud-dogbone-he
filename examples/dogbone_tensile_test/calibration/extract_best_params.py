#!/usr/bin/env python3
"""Print the best-evaluation parameter values from a Dakota .out file.

Usage: extract_best_params.py <dakota.out> [descriptor ...]

With no descriptors, prints every parameter as `name=value`. With
descriptors given, prints only those, in the order requested -- handy for
pasting straight into the next stage's dakota.in continuous_state block.
"""
import re
import sys


def main():
    dakota_out, wanted = sys.argv[1], sys.argv[2:]

    with open(dakota_out) as f:
        lines = f.readlines()

    start = next(i for i, line in enumerate(lines) if "Best parameters" in line)
    params = {}
    for line in lines[start + 1:]:
        match = re.match(r"\s*([-\d.eE+]+)\s+(\S+)\s*$", line)
        if not match:
            break
        value, name = match.groups()
        params[name] = value

    names = wanted if wanted else params.keys()
    for name in names:
        print(f"{name}={params[name]}")


if __name__ == "__main__":
    main()
