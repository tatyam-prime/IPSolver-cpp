#!/usr/bin/env python3
"""Check ABC338 F stdin/stdout against the independent C++ Held-Karp oracle."""
import argparse
import os
from pathlib import Path
import random
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def encode(n, edges):
	return f"{n} {len(edges)}\n" + "".join(
		f"{i + 1} {j + 1} {w}\n" for i, j, w in edges)


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument("--standalone", action="store_true")
	parser.add_argument("--random-cases", type=int, default=50)
	parser.add_argument("--max-cases", type=int, default=10)
	args = parser.parse_args()
	cases = [encode(3, [(0, 1, 5), (1, 0, -3), (1, 2, -4), (2, 0, 100)]),
			 encode(3, [(0, 1, 0), (1, 0, 0)]),
			 encode(5, [(0, 1, -246288), (3, 4, -222742), (2, 0, 246288),
						(2, 3, 947824), (4, 1, -178721), (3, 2, -947824),
						(4, 3, 756570), (1, 4, 707902), (4, 0, 36781)])]
	rng = random.Random(3382026104)
	for tc in range(args.random_cases + args.max_cases):
		n = rng.randint(2, 9) if tc < args.random_cases else 20
		potential = [rng.randint(-300000, 300000) for _ in range(n)]
		density = [100, 50, 20, 8][tc % 4]
		# Nonnegative reduced costs guarantee that no cycle has negative cost.
		edges = [(i, j, (0 if tc % 5 == 0 else rng.randint(0, 400000))
				  + potential[j] - potential[i])
				 for i in range(n) for j in range(n)
				 if i != j and rng.randrange(100) < density]
		if not edges:
			edges = [(0, 1, potential[1] - potential[0])]
		cases.append(encode(n, edges))
	with tempfile.TemporaryDirectory(prefix="ip-abc338-") as temp:
		binary, oracle = Path(temp) / "submission", Path(temp) / "oracle"
		compiler = shlex.split(os.environ.get("CXX", "c++"))
		flags = compiler + ["-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic"]
		source = ROOT / ("build/submissions/atcoder" if args.standalone else "examples/atcoder") / "abc338_f.cpp"
		subprocess.run(flags + ["-I" + str(ROOT / "include"), str(source),
								"-o", str(binary)], check=True)
		subprocess.run(flags + ["-I" + str(ROOT / "include"),
								str(ROOT / "tests/test_abc338_f.cpp"),
								"-o", str(oracle)], check=True)
		reference = subprocess.run([str(oracle), "--oracle"],
								   input=str(len(cases)) + "\n" + "".join(cases),
								   text=True, capture_output=True, check=True, timeout=120)
		expected = reference.stdout.split()
		assert len(expected) == len(cases)
		assert expected[:3] == ["-2", "No", "-449429"]
		for data, answer in zip(cases, expected):
			run = subprocess.run([str(binary)], input=data, text=True,
								 capture_output=True, timeout=6)
			assert run.returncode == 0 and run.stdout.strip() == answer, (
				data, answer, run.returncode, run.stdout, run.stderr)
		print(f"ABC338F submissions: {len(cases)} cases passed "
			  f"({args.max_cases} at N=20)")


if __name__ == "__main__":
	main()
