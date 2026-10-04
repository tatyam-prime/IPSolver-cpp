#!/usr/bin/env python3
"""Check standalone submissions against samples and independent integer oracles."""
import argparse
import os
from pathlib import Path
import random
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parent.parent


def clique_oracle(n, edges):
	adj = [0] * n
	for u, v in edges:
		adj[u] |= 1 << v
		adj[v] |= 1 << u
	best = 0
	for mask in range(1 << n):
		if mask.bit_count() <= best:
			continue
		rest = mask
		while rest:
			bit = rest & -rest
			rest ^= bit
			if rest & ~adj[bit.bit_length() - 1]:
				break
		else:
			best = mask.bit_count()
	return best


def recipes_oracle(q, a, b):
	ua = min(qi // ai for qi, ai in zip(q, a) if ai)
	ub = min(qi // bi for qi, bi in zip(q, b) if bi)
	if ub < ua:
		a, b, ua, ub = b, a, ub, ua
	best = 0
	for x in range(ua + 1):
		y = min((qi - ai * x) // bi for qi, ai, bi in zip(q, a, b) if bi)
		best = max(best, x + y)
	return best


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument("--random-cases", type=int, default=500)
	args = parser.parse_args()
	rng = random.Random(20261004)
	counts = {"abc002_d": 0, "abc338_c": 0}
	slowest = {name: 0 for name in counts}
	with tempfile.TemporaryDirectory(prefix="ip-atcoder-") as directory:
		binaries = {}
		for name in counts:
			executable = Path(directory) / name
			subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-O2",
							"-Wall", "-Wextra", "-Wpedantic",
							str(ROOT / "build/submissions/atcoder" / f"{name}.cpp"),
							"-o", str(executable)], check=True)
			binaries[name] = executable

		def check(name, data, expected):
			start = time.perf_counter()
			result = subprocess.run([str(binaries[name])], input=data, text=True,
									capture_output=True, timeout=2)
			elapsed = time.perf_counter() - start
			if result.returncode or result.stdout.strip() != str(expected):
				raise AssertionError(f"{name}: expected {expected}, returncode "
									 f"{result.returncode}, stdout={result.stdout!r}\n{data}")
			counts[name] += 1
			slowest[name] = max(slowest[name], elapsed)

		# Official samples: https://atcoder.jp/contests/abc002/tasks/abc002_4
		for data, answer in [
			("5 3\n1 2\n2 3\n1 3\n", 3),
			("5 3\n1 2\n2 3\n3 4\n", 2),
			("7 9\n1 2\n1 3\n2 3\n4 5\n4 6\n4 7\n5 6\n5 7\n6 7\n", 4),
			("12 0\n", 1),
		]:
			check("abc002_d", data, answer)
		# Official samples: https://atcoder.jp/contests/abc338/tasks/abc338_c
		for data, answer in [
			("2\n800 300\n100 100\n200 10\n", 5),
			("2\n800 300\n100 0\n0 10\n", 38),
			("2\n800 300\n801 300\n800 301\n", 0),
			("10\n" + "1000000 " * 10 + "\n0 1 2 3 4 5 6 7 8 9\n"
			 "9 8 7 6 5 4 3 2 1 0\n", 222222),
		]:
			check("abc338_c", data, answer)

		# Empty/complete graphs, cycles and complete multipartite graphs at N=12.
		graph_cases = [
			(1, []),
			(12, [(i, j) for i in range(12) for j in range(i + 1, 12)]),
			(12, [(i, (i + 1) % 12) for i in range(12)]),
		]
		for parts in (2, 3, 4, 6):
			graph_cases.append((12, [(i, j) for i in range(12) for j in range(i + 1, 12)
									 if i % parts != j % parts]))
		for _ in range(args.random_cases):
			n = rng.randint(1, 12)
			probability = rng.random()
			edges = [(i, j) for i in range(n) for j in range(i + 1, n)
					 if rng.random() < probability]
			graph_cases.append((n, edges))
		for n, edges in graph_cases:
			data = f"{n} {len(edges)}\n" + "".join(f"{i+1} {j+1}\n" for i, j in edges)
			check("abc002_d", data, clique_oracle(n, edges))

		recipe_cases = [
			([1000000], [1], [1]),
			([1000000, 1000000], [1, 0], [0, 1]),
			([1000000, 1000000], [999999, 1], [1, 999999]),
			([1000000, 999999], [97, 98], [98, 99]),
		]
		for case in range(args.random_cases):
			n = rng.randint(1, 10)
			# Small exhaustive cases and large capacities / nearly parallel rows.
			cap = 40 if case % 3 == 0 else 1000000
			limit = (8, 100, 1000000)[case % 3]
			q = [rng.randint(1, cap) for _ in range(n)]
			a = [rng.randint(0, limit) for _ in range(n)]
			b = [rng.randint(0, limit) for _ in range(n)]
			if case % 5 == 0:
				b = [max(0, min(limit, ai + rng.randint(-1, 1))) for ai in a]
			if not any(a):
				a[0] = 1
			if not any(b):
				b[-1] = 1
			recipe_cases.append((q, a, b))
		for q, a, b in recipe_cases:
			data = str(len(q)) + "\n" + "\n".join(" ".join(map(str, row)) for row in (q, a, b)) + "\n"
			check("abc338_c", data, recipes_oracle(q, a, b))

	for name in counts:
		print(f"{name}: {counts[name]} cases passed; slowest subprocess {slowest[name]*1000:.1f} ms")


if __name__ == "__main__":
	main()
