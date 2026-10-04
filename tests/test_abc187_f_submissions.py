#!/usr/bin/env python3
"""Validate the actual submission's I/O using exact complement coloring."""
import argparse
import os
from pathlib import Path
import random
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parent.parent


def encode(n, edges):
	return f"{n} {len(edges)}\n" + "".join(f"{u + 1} {v + 1}\n" for u, v in edges)


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument("--source", type=Path,
						default=ROOT / "examples/atcoder/abc187_f.cpp")
	parser.add_argument("--small", type=int, default=300)
	parser.add_argument("--stress", type=int, default=100)
	args = parser.parse_args()
	rng = random.Random(18720261004)
	cases = [
		encode(3, [(0, 1), (0, 2)]),
		encode(4, [(i, j) for i in range(4) for j in range(i + 1, 4)]),
		encode(10, [(8, 9), (1, 9), (7, 8), (2, 3), (4, 7), (0, 7),
					(4, 5), (1, 4), (2, 5), (5, 8), (0, 8)]),
		encode(18, []),
		encode(1, []),
		encode(18, [(i, j) for i in range(18) for j in range(i + 1, 18)]),
		encode(18, [(i, j) for i in range(18) for j in range(i + 1, 18)
					if i // 3 != j // 3]),
	]
	for t in range(args.small + args.stress):
		n = rng.randint(1, 12) if t < args.small else 18
		density = rng.random()
		cases.append(encode(n, [(i, j) for i in range(n) for j in range(i + 1, n)
								if rng.random() < density]))
	compiler = os.environ.get("CXX", "c++")
	with tempfile.TemporaryDirectory(prefix="ip-abc187-f-") as directory:
		source_binary = Path(directory) / "submission"
		oracle_binary = Path(directory) / "oracle"
		for source, binary in [(args.source, source_binary),
							   (ROOT / "tests/test_abc187_f.cpp", oracle_binary)]:
			subprocess.run([compiler, "-std=c++17", "-O2", "-Wall", "-Wextra",
							"-Wpedantic", "-I", str(ROOT / "include"),
							str(source), "-o", str(binary)], check=True)
		oracle = subprocess.run([str(oracle_binary), "--oracle"], input="".join(cases),
								text=True, capture_output=True, check=True).stdout.split()
		assert list(map(int, oracle[:4])) == [2, 1, 5, 18]
		slowest = 0
		for case, expected in zip(cases, oracle, strict=True):
			start = time.perf_counter()
			result = subprocess.run([str(source_binary)], input=case, text=True,
									capture_output=True, timeout=3)
			slowest = max(slowest, time.perf_counter() - start)
			assert result.returncode == 0 and result.stdout.strip() == expected, (
				case, expected, result.returncode, result.stdout, result.stderr)
	print(f"ABC187 F submission: {len(cases)} cases passed; "
		  f"slowest subprocess {slowest * 1000:.1f} ms")


if __name__ == "__main__":
	main()
