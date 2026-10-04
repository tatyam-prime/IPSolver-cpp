#!/usr/bin/env python3
"""Check the submission against exhaustive skill levels and an integer max-flow."""
import argparse
from collections import deque
from itertools import product
import os
from pathlib import Path
import random
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parent.parent


def exhaustive(c, a, levels):
	return max(sum(ai for ai, requirement in zip(a, levels)
				   if all(x >= need for x, need in zip(choice, requirement)))
			   - sum(ci * (x - 1) for ci, x in zip(c, choice))
			   for choice in product(range(1, 6), repeat=len(c)))


def flow_oracle(c, a, levels):
	# Use all four unit upgrades rather than the submission's compressed levels.
	n, m = len(c), len(a)
	source, sink = 4 * n + m, 4 * n + m + 1
	graph = [[] for _ in range(sink + 1)]

	def add(u, v, cap):
		forward = [v, cap, len(graph[v])]
		reverse = [u, 0, len(graph[u])]
		graph[u].append(forward)
		graph[v].append(reverse)

	infinity = sum(a) + 1
	for j, cost in enumerate(c):
		for k in range(4):
			add(4 * j + k, sink, cost)
			if k:
				add(4 * j + k, 4 * j + k - 1, infinity)
	for i, reward in enumerate(a):
		vertex = 4 * n + i
		add(source, vertex, reward)
		for j, need in enumerate(levels[i]):
			if need > 1:
				add(vertex, 4 * j + need - 2, infinity)
	flow = 0
	while True:
		distance = [-1] * len(graph)
		distance[source] = 0
		queue = deque([source])
		while queue:
			u = queue.popleft()
			for v, cap, _ in graph[u]:
				if cap and distance[v] < 0:
					distance[v] = distance[u] + 1
					queue.append(v)
		if distance[sink] < 0:
			return sum(a) - flow
		position = [0] * len(graph)

		def send(u, amount):
			if u == sink:
				return amount
			while position[u] < len(graph[u]):
				edge = graph[u][position[u]]
				v, cap, reverse = edge
				if cap and distance[v] == distance[u] + 1:
					pushed = send(v, min(amount, cap))
					if pushed:
						edge[1] -= pushed
						graph[v][reverse][1] += pushed
						return pushed
				position[u] += 1
			return 0

		while True:
			pushed = send(source, infinity)
			if not pushed:
				break
			flow += pushed


def input_text(c, a, levels):
	return (f"{len(c)} {len(a)}\n"
			+ " ".join(map(str, c)) + "\n"
			+ " ".join(map(str, a)) + "\n"
			+ "".join(" ".join(map(str, row)) + "\n" for row in levels))


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument("--small-cases", type=int, default=400)
	parser.add_argument("--stress-cases", type=int, default=100)
	parser.add_argument("--source", type=Path,
						default=ROOT / "examples/atcoder/abc326_g.cpp")
	parser.add_argument("--timeout", type=float, default=2)
	args = parser.parse_args()
	rng = random.Random(32620261004)
	counts = {"samples": 0, "small": 0, "stress": 0}
	slowest = {group: 0 for group in counts}
	with tempfile.TemporaryDirectory(prefix="ip-abc326-g-") as directory:
		binary = Path(directory) / "submission"
		subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-O2",
						"-Wall", "-Wextra", "-Wpedantic", "-I", str(ROOT / "include"),
						str(args.source), "-o", str(binary)], check=True)

		def check(group, c, a, levels, expected=None):
			oracle = flow_oracle(c, a, levels)
			if expected is not None:
				assert oracle == expected
			if group == "small":
				assert oracle == exhaustive(c, a, levels)
			data = input_text(c, a, levels)
			start = time.perf_counter()
			result = subprocess.run([str(binary)], input=data, text=True,
									capture_output=True, timeout=args.timeout)
			elapsed = time.perf_counter() - start
			if result.returncode or result.stdout.strip() != str(oracle):
				failure = ROOT / "tests/abc326_g_failure.txt"
				failure.write_text(data)
				raise AssertionError(f"expected {oracle}, returncode {result.returncode}, "
									 f"stdout {result.stdout!r}; input saved to {failure}")
			counts[group] += 1
			slowest[group] = max(slowest[group], elapsed)

		# Official samples: https://atcoder.jp/contests/abc326/tasks/abc326_g
		check("samples", [10, 20], [100, 50], [[3, 1], [1, 4]], 80)
		check("samples", [10, 20], [100, 50], [[3, 2], [1, 4]], 70)
		check("samples",
			  [10922, 23173, 32300, 22555, 29525, 16786, 3135, 17046, 11245, 20310],
			  [177874, 168698, 202247, 31339, 10336, 14825, 56835, 6497, 12440, 110702],
			  [[2, 1, 4, 1, 3, 4, 4, 5, 1, 4], [2, 3, 4, 4, 5, 3, 5, 5, 2, 3],
			   [2, 3, 5, 1, 4, 2, 2, 2, 2, 5], [3, 5, 5, 3, 5, 2, 2, 1, 5, 4],
			   [3, 1, 1, 4, 4, 1, 1, 5, 3, 1], [1, 2, 3, 2, 4, 2, 4, 3, 3, 1],
			   [4, 4, 4, 2, 5, 1, 4, 2, 2, 2], [5, 3, 1, 2, 3, 4, 2, 5, 2, 2],
			   [5, 4, 3, 4, 3, 1, 5, 1, 5, 4], [2, 3, 2, 5, 2, 3, 1, 2, 2, 4]],
			  66900)

		# Exercise missing intermediate thresholds, tied optima, and maximum coefficients.
		for cost, reward in [(1, 1), (1, 4), (1, 5), (1000000, 1000000)]:
			for level in range(1, 6):
				check("small", [cost], [reward], [[level]])
		for _ in range(args.small_cases):
			n, m = rng.randint(1, 5), rng.randint(1, 10)
			bound = rng.choice([1, 10, 1000000])
			c = [rng.randint(1, bound) for _ in range(n)]
			a = [rng.randint(1, bound) for _ in range(m)]
			levels = [[rng.randint(1, 5) for _ in range(n)] for _ in range(m)]
			check("small", c, a, levels)

		n = m = 50
		# A fixed near-tie input that took 7,576 root pivots with the default pricing.
		tokens = iter(map(int, (ROOT / "tests/data/abc326_g_degenerate.in").read_text().split()))
		fixture_n, fixture_m = next(tokens), next(tokens)
		fixture_c = [next(tokens) for _ in range(fixture_n)]
		fixture_a = [next(tokens) for _ in range(fixture_m)]
		fixture_levels = [[next(tokens) for _ in range(fixture_n)] for _ in range(fixture_m)]
		check("stress", fixture_c, fixture_a, fixture_levels, 0)
		# Extremal coefficient patterns and degenerate / dense implications.
		for level in range(1, 6):
			for cost, reward in [(1, 1000000), (1000000, 1), (1000000, 1000000)]:
				check("stress", [cost] * n, [reward] * m, [[level] * n for _ in range(m)])
		for case in range(args.stress_cases):
			family = case % 8
			c = [rng.randint(1, 1000000) for _ in range(n)]
			a = [rng.randint(1, 1000000) for _ in range(m)]
			if family == 0:
				levels = [[rng.randint(1, 5) for _ in range(n)] for _ in range(m)]
			elif family == 1:
				levels = [[rng.randint(2, 5) for _ in range(n)] for _ in range(m)]
			elif family == 2:
				# Sparse dependencies make the optimum use a nontrivial proper subset.
				levels = [[rng.randint(2, 5) if rng.random() < 0.04 else 1
						   for _ in range(n)] for _ in range(m)]
			elif family == 3:
				levels = [[rng.randint(2, 5) if rng.random() < 0.15 else 1
						   for _ in range(n)] for _ in range(m)]
			elif family == 4:
				c = [100000] * n
				a = [100000] * m
				levels = [[1] * n for _ in range(m)]
				for i in range(m):
					for offset in range(3):
						levels[i][(i + offset) % n] = rng.randint(2, 5)
			elif family == 5:
				levels = [[1 + (i + j) % 5 for j in range(n)] for i in range(m)]
			elif family == 6:
				levels = [[rng.randint(1, 5) if j % 10 == i % 10 else 1
						   for j in range(n)] for i in range(m)]
				c = [rng.randint(1, 100000) for _ in range(n)]
			else:
				# Almost tied all-upgrade profit increases degenerate simplex pivots.
				c = [rng.choice([249999, 250000, 250001]) for _ in range(n)]
				a = [rng.choice([999999, 1000000]) for _ in range(m)]
				density = rng.uniform(0.2, 1)
				levels = [[rng.randint(2, 5) if rng.random() < density else 1
						   for _ in range(n)] for _ in range(m)]
			check("stress", c, a, levels)

	for group in counts:
		print(f"abc326_g {group}: {counts[group]} cases passed; "
			  f"slowest subprocess {slowest[group] * 1000:.1f} ms")


if __name__ == "__main__":
	main()
