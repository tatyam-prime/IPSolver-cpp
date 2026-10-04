#!/usr/bin/env python3
"""Compile the standalone ABC165 C submission and check an independent oracle."""
import argparse
import itertools
import os
from pathlib import Path
import random
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parent.parent


def oracle(n, m, requirements):
    return max(sum(d for a, b, c, d in requirements if seq[b] - seq[a] == c)
               for seq in itertools.combinations_with_replacement(range(1, m + 1), n))


def encode(n, m, requirements):
    return f"{n} {m} {len(requirements)}\n" + "".join(
        f"{a+1} {b+1} {c} {d}\n" for a, b, c, d in requirements)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--random-cases", type=int, default=100)
    parser.add_argument("--max-cases", type=int, default=4)
    args = parser.parse_args()
    source = ROOT / "build/submissions/atcoder/abc165_c.cpp"
    rng = random.Random(1652026104)
    checks, worst = 0, 0.0
    with tempfile.TemporaryDirectory(prefix="ip-abc165c-") as directory:
        executable = Path(directory) / "submission"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-O2",
                        "-Wall", "-Wextra", "-Wpedantic", str(source), "-o",
                        str(executable)], check=True)

        def check(data, expected):
            nonlocal checks, worst
            start = time.perf_counter()
            r = subprocess.run([str(executable)], input=data, text=True,
                               capture_output=True, timeout=3)
            worst = max(worst, time.perf_counter() - start)
            if r.returncode or r.stdout.strip() != str(expected):
                raise AssertionError(f"expected {expected}, returncode {r.returncode}, "
                                     f"stdout={r.stdout!r}, stderr={r.stderr!r}\n{data}")
            checks += 1

        samples = [
            ("3 4 3\n1 3 3 100\n1 2 2 10\n2 3 2 10\n", 110),
            ("4 6 10\n2 4 1 86568\n1 4 0 90629\n2 3 0 90310\n3 4 1 29211\n"
             "3 4 3 78537\n3 4 2 8580\n1 2 1 96263\n1 4 2 2156\n"
             "1 2 0 94325\n1 4 3 94328\n", 357500),
            ("10 10 1\n1 10 9 1\n", 1),
        ]
        for data, expected in samples:
            check(data, expected)
        for name in ("pair_numeric", "clique_numeric"):
            data = (ROOT / "tests/data" / f"abc165_c_{name}.in").read_text()
            tokens = list(map(int, data.split()))
            n, m, q = tokens[:3]
            requirements = [(a-1, b-1, c, d) for a, b, c, d in
                            zip(tokens[3::4], tokens[4::4], tokens[5::4], tokens[6::4])]
            assert len(requirements) == q
            check(data, oracle(n, m, requirements))
        for case in range(args.random_cases + args.max_cases):
            large = case >= args.random_cases
            n = 10 if large else rng.randint(2, 7)
            m = 10 if large else rng.randint(1, 6)
            triples = [(a, b, c) for a in range(n) for b in range(a + 1, n)
                       for c in range(m)]
            q = 50 if large else rng.randint(1, min(50, len(triples)))
            requirements = [(*triple, 100000 if case % 3 == 0 else rng.randint(1, 100000))
                            for triple in rng.sample(triples, q)]
            check(encode(n, m, requirements), oracle(n, m, requirements))
    print(f"ABC165 C standalone: {checks} checks passed; slowest process {worst*1000:.1f} ms")


if __name__ == "__main__":
    main()
