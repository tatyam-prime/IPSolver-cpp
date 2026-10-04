#!/usr/bin/env python3
"""Validate standalone output against the original per-position prime-mask DP."""
import argparse
import math
import os
from pathlib import Path
import random
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--random-cases", type=int, default=40)
    parser.add_argument("--max-cases", type=int, default=6)
    args = parser.parse_args()
    rng = random.Random(4532026104)
    cases = [[1] * 5, [1, 6, 4, 2, 8]]
    cases.extend([[x] for x in range(1, 31)])
    for case in range(args.random_cases):
        limit = 30 if case % 8 == 0 else 12
        cases.append([rng.randint(1, limit) for _ in range(rng.randint(1, 8))])
    for case in range(args.max_cases):
        cases.append([30] * 100 if case % 3 == 0 else
                     [1 + i % 30 for i in range(100)] if case % 3 == 1 else
                     [rng.randint(1, 30) for _ in range(100)])
    worst = 0.0
    with tempfile.TemporaryDirectory(prefix="ip-cf453b-") as directory:
        submission = Path(directory) / "submission"
        oracle = Path(directory) / "oracle"
        compiler = os.environ.get("CXX", "c++")
        for source, output in [(ROOT / "build/submissions/codeforces/cf453_b.cpp", submission),
                               (ROOT / "tests/test_cf453_b.cpp", oracle)]:
            subprocess.run([compiler, "-std=c++17", "-O2", "-Wall", "-Wextra",
                            "-Wpedantic", "-I" + str(ROOT / "include"), str(source),
                            "-o", str(output)], check=True)
        for a in cases:
            data = f"{len(a)}\n" + " ".join(map(str, a)) + "\n"
            expected = int(subprocess.run([str(oracle), "--oracle"], input=data,
                                          text=True, capture_output=True,
                                          check=True, timeout=30).stdout)
            start = time.perf_counter()
            result = subprocess.run([str(submission)], input=data, text=True,
                                    capture_output=True, timeout=4)
            worst = max(worst, time.perf_counter() - start)
            b = list(map(int, result.stdout.split()))
            valid = len(b) == len(a) and all(x >= 1 for x in b) and all(
                math.gcd(x, y) == 1 for i, x in enumerate(b) for y in b[:i])
            cost = sum(abs(x - y) for x, y in zip(a, b))
            if result.returncode or not valid or cost != expected:
                raise AssertionError(f"expected cost {expected}, got {cost}, "
                                     f"status {result.returncode}, output {b}\n{data}")
    print(f"CF453 B standalone: {len(cases)} checks passed; slowest process {worst*1000:.1f} ms")


if __name__ == "__main__":
    main()
