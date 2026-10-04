#!/usr/bin/env python3
"""Check ABC180E stdin/stdout using an independent directed Held-Karp oracle."""
import argparse
import os
from pathlib import Path
import random
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def encode(points):
    return f"{len(points)}\n" + "".join(f"{x} {y} {z}\n" for x, y, z in points)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--standalone", action="store_true")
    parser.add_argument("--random-cases", type=int, default=50)
    parser.add_argument("--max-cases", type=int, default=10)
    args = parser.parse_args()
    cases = [encode([(0, 0, 0), (1, 2, 3)]),
             encode([(0, 0, 0), (1, 1, 1), (-1, -1, -1)])]
    rng = random.Random(1802026104)
    for tc in range(args.random_cases + args.max_cases):
        n = rng.randint(2, 10) if tc < args.random_cases else 17
        occupied, points = set(), []
        while len(points) < n:
            p = tuple(rng.randint(-1000000, 1000000) for _ in range(3))
            if p not in occupied:
                occupied.add(p)
                points.append(p)
        cases.append(encode(points))
    with tempfile.TemporaryDirectory(prefix="ip-abc180-") as temp:
        binary, oracle = Path(temp) / "submission", Path(temp) / "oracle"
        cxx = os.environ.get("CXX", "c++")
        flags = [cxx, "-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic"]
        source = ROOT / ("build/submissions/atcoder" if args.standalone else "examples/atcoder") / "abc180_e.cpp"
        subprocess.run(flags + ["-I" + str(ROOT / "include"), str(source),
                                "-o", str(binary)], check=True)
        subprocess.run(flags + ["-I" + str(ROOT / "include"),
                                str(ROOT / "tests/test_abc180_e.cpp"),
                                "-o", str(oracle)], check=True)
        oracle_run = subprocess.run([str(oracle), "--oracle"],
                                    input=str(len(cases)) + "\n" + "".join(cases),
                                    text=True, capture_output=True, check=True, timeout=60)
        expected = list(map(int, oracle_run.stdout.split()))
        assert len(expected) == len(cases)
        assert expected[:2] == [9, 10]
        for data, answer in zip(cases, expected):
            run = subprocess.run([str(binary)], input=data, text=True,
                                 capture_output=True, timeout=2)
            assert run.returncode == 0 and run.stdout.strip() == str(answer), (
                data, answer, run.returncode, run.stdout, run.stderr)
        print(f"ABC180E submissions: {len(cases)} cases passed "
              f"({args.max_cases} at N=17)")


if __name__ == "__main__":
    main()
