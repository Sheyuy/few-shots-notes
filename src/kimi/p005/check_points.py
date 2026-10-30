#!/usr/bin/env python3
"""Fast deterministic checks for the archived point sets (no solver needed)."""
import argparse
from caplib import load, check_points, conflict_lines, valid_cap


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('files', nargs='+')
    ap.add_argument('--expect-size', type=int)
    ap.add_argument('--expect-bad', type=int)
    args = ap.parse_args()
    failed = False
    for path in args.files:
        S = load(path)
        bad = len(conflict_lines(S))
        ok = check_points(S)
        if args.expect_size is not None:
            ok &= len(S) == args.expect_size
        if args.expect_bad is not None:
            ok &= bad == args.expect_bad
        print(path, 'n', len(S), 'conflict_lines', bad,
              'unique_range_ok', check_points(S), 'PASS' if ok else 'FAIL')
        failed |= not ok
    raise SystemExit(1 if failed else 0)


if __name__ == '__main__':
    main()
