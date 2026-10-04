#!/usr/bin/env python3
"""Check the actual submission executable with an independent integer max flow."""
import argparse
import os
from pathlib import Path
import random
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parent.parent


def encode(cost, edges):
    return (f"{len(cost)} {len(edges)}\n" + " ".join(map(str, cost)) + "\n"
            + "".join(f"{u + 1} {v + 1} {w}\n" for u, v, w in edges))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path,
                        default=ROOT / "examples/codeforces/cf1082_g.cpp")
    parser.add_argument("--small", type=int, default=200)
    parser.add_argument("--stress", type=int, default=20)
    parser.add_argument("--timeout", type=float, default=3)
    args = parser.parse_args()
    rng = random.Random(108220261005)
    cases = [
        encode([1, 5, 2, 2], [(0, 2, 4), (0, 3, 4), (2, 3, 5), (2, 1, 2), (3, 1, 2)]),
        encode([9, 7, 8], [(0, 1, 1), (1, 2, 2), (0, 2, 3)]),
        encode([10**9], []),
        encode([10**9] * 1000, []),
    ]
    for shape in ("path", "cycle", "star"):
        edges = [(0 if shape == "star" else v - 1, v, 10**9) for v in range(1, 1000)]
        if shape == "cycle":
            edges.append((999, 0, 10**9))
        cases.append(encode([10**9] * 1000, edges))
    joined = [(0, 1, 10**9)] + [(hub, v, 10**9) for v in range(2, 500) for hub in (0, 1)]
    cases.append(encode([10**9] * 1000, joined))
    dense = [(u, v, 10**9) for u in range(45) for v in range(u + 1, 45)]
    cases.append(encode([1] * 1000, dense))
    for delta in (-1, 0, 1):
        edges = [(v, (v + 1) % 666, 666666666) for v in range(666)]
        edges += [(v, v + 333, 666666666) for v in range(333)]
        edges.append((0, 2, 1))
        cases.append(encode([999999999 + delta] * 666 + [10**9] * 334, edges))
    # All 1000 positive edge rewards contribute, so the answer is close to 1e12.
    profitable = [(v, (v + 1) % 1000, 10**9) for v in range(1000)]
    cases.append(encode([1] * 1000, profitable))
    for t in range(args.small + args.stress):
        n = rng.randint(1, 12) if t < args.small else 1000
        limit = 10 if t % 3 else 10**9
        cost = [rng.randint(1, limit) for _ in range(n)]
        if t < args.small:
            density = rng.random()
            edges = [(u, v, rng.randint(1, limit)) for u in range(n) for v in range(u + 1, n)
                     if rng.random() < density]
        else:
            used = set()
            while len(used) < 1000:
                u, v = sorted(rng.sample(range(n), 2))
                used.add((u, v))
            edges = [(u, v, rng.randint(1, limit)) for u, v in sorted(used)]
        cases.append(encode(cost, edges))
    compiler = os.environ.get("CXX", "c++")
    with tempfile.TemporaryDirectory(prefix="ip-cf1082-g-") as directory:
        submission = Path(directory) / "submission"
        oracle = Path(directory) / "oracle"
        for source, binary in [(args.source, submission),
                               (ROOT / "tests/test_cf1082_g.cpp", oracle)]:
            subprocess.run([compiler, "-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic",
                            "-I", str(ROOT / "include"), str(source), "-o", str(binary)], check=True)
        expected = subprocess.run([str(oracle), "--oracle"], input="".join(cases), text=True,
                                  capture_output=True, check=True).stdout.split()
        assert len(expected) == len(cases)
        assert list(map(int, expected[:2])) == [8, 0]
        assert int(expected[7]) == 497000000000
        assert int(expected[12]) == 999999999000
        slowest = 0
        for case, value in zip(cases, expected, strict=True):
            start = time.perf_counter()
            result = subprocess.run([str(submission)], input=case, text=True, capture_output=True,
                                    timeout=args.timeout)
            slowest = max(slowest, time.perf_counter() - start)
            assert result.returncode == 0 and result.stdout.strip() == value, (
                case, value, result.returncode, result.stdout, result.stderr)
    print(f"CF1082 G submission: {len(cases)} cases passed; "
          f"slowest subprocess {slowest * 1000:.1f} ms; compiler {compiler}")


if __name__ == "__main__":
    main()
