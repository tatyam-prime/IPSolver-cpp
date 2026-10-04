#!/usr/bin/env python3
"""Validate CF8C optimal cost and reconstructed route against subset DP."""
import argparse
import os
from pathlib import Path
import random
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parent.parent


def encode(start, points):
	return f"{start[0]} {start[1]}\n{len(points)}\n" + "".join(
		f"{x} {y}\n" for x, y in points)


def validate(data, output, expected):
	values = list(map(int, data.split()))
	start, n = tuple(values[:2]), values[2]
	points = [start] + list(zip(values[3::2], values[4::2]))
	lines = output.splitlines()
	assert len(lines) == 2, output
	answer = int(lines[0])
	route = list(map(int, lines[1].split()))
	assert route[0] == route[-1] == 0, output
	assert all(0 <= v <= n for v in route), output
	assert sorted(v for v in route if v) == list(range(1, n + 1)), output
	cost, carried = 0, 0
	for previous, current in zip(route, route[1:]):
		cost += sum((a - b) ** 2 for a, b in zip(points[previous], points[current]))
		if current == 0:
			assert 1 <= carried <= 2, output
			carried = 0
		else:
			carried += 1
			assert carried <= 2, output
	assert answer == cost == expected, (data, output, expected, cost)


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument("--random-cases", type=int, default=500)
	parser.add_argument("--max-cases", type=int, default=100)
	parser.add_argument("--standalone", action="store_true")
	args = parser.parse_args()
	rng = random.Random(20261004)
	cases = [
		# Official samples: https://codeforces.com/problemset/problem/8/C
		("sample", encode((0, 0), [(1, 1), (-1, 1)]), 8),
		("sample", encode((1, 1), [(4, 3), (3, 4), (0, 0)]), 32),
	]
	# Zero positive pair gains, odd cycles, ties, extreme coordinates, and n=1.
	for start, points in [
		((0, 0), [(100, 0)]),
		((0, 0), [(100, 0), (-100, 0)]),
		((0, 0), [(1, 0), (-1, 0), (0, 1), (0, -1)]),
		((0, 0), [(10, 0), (9, 1), (9, -1)]),
		((-100, -100), [(100, 100), (100, -100), (-100, 100)]),
		((0, 0), [(i, 0) for i in range(1, 25)]),
		((0, 0), [(i, i) for i in range(-12, 13) if i]),
		((0, 0), [(100, i) for i in range(-12, 12)]),
		((100, 100), [(i, -100) for i in range(-12, 12)]),
		((0, 0), [(x, y) for x, y in
					[(100, i) for i in range(-4, 4)] +
					[(-50+i, 86) for i in range(-4, 4)] +
					[(-50+i, -86) for i in range(-4, 4)]]),
		((0, 0), [(x, y) for x, y in
					[(100, i) for i in range(-3, 4)] +
					[(-50+i, 86) for i in range(-3, 4)] +
					[(-50+i, -86) for i in range(-3, 4)]]),
	]:
		cases.append(("edge", encode(start, points), None))
	# Several separated odd matching components defeat the uncut relaxation.
	for lengths in [(7, 7, 9), (7, 9, 7), (9, 7, 7), (5, 9, 9), (11, 11, 1)]:
		points = []
		for (x, y), count in zip([(100, 0), (-50, 86), (-50, -86)], lengths):
			points.extend((x, y + i) if y == 0 else (x + i, y)
						  for i in range(-(count // 2), count // 2 + 1))
		cases.append(("odd clusters", encode((0, 0), points), None))
	for _ in range(args.random_cases):
		n = rng.randint(1, 16)
		start = (rng.randint(-100, 100), rng.randint(-100, 100))
		occupied, points = {start}, []
		while len(points) < n:
			p = (rng.randint(-100, 100), rng.randint(-100, 100))
			if p not in occupied:
				occupied.add(p)
				points.append(p)
		cases.append(("random", encode(start, points), None))
	for t in range(args.max_cases):
		start = (rng.randint(-100, 100), rng.randint(-100, 100))
		occupied, points = {start}, []
		while len(points) < 24:
			# Mix global points and dense clusters (including high degeneracy).
			p = (rng.randint(-100, 100), rng.randint(-100, 100))
			if t % 4 == 1:
				p = (rng.randint(70, 100), rng.randint(70, 100))
			elif t % 4 == 2:
				p = (rng.randint(-10, 10), rng.randint(-10, 10))
			if p not in occupied:
				occupied.add(p)
				points.append(p)
		cases.append(("maximum", encode(start, points), None))

	with tempfile.TemporaryDirectory(prefix="ip-cf008-") as temp:
		cxx = os.environ.get("CXX", "c++")
		flags = [cxx, "-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic"]
		submission, oracle = Path(temp) / "submission", Path(temp) / "oracle"
		source = ROOT / ("build/submissions/codeforces" if args.standalone else "examples/codeforces") / "cf008_c.cpp"
		subprocess.run(flags + ["-DIPSOLVER_STATS", "-I" + str(ROOT / "include"),
								str(source), "-o", str(submission)], check=True)
		subprocess.run(flags + [str(ROOT / "tests/test_cf008_c.cpp"),
								"-o", str(oracle)], check=True)
		oracle_data = f"{len(cases)}\n" + "".join(data for _, data, _ in cases)
		oracle_output = subprocess.run([str(oracle)], input=oracle_data, text=True,
									   capture_output=True, check=True, timeout=120)
		optimum = list(map(int, oracle_output.stdout.split()))
		assert len(optimum) == len(cases)
		worst_wall, worst_solver, max_nodes, max_pivots = 0, (0, 0, 0, ""), 0, 0
		for (kind, data, known), expected in zip(cases, optimum):
			if known is not None:
				assert known == expected
			before = time.perf_counter()
			run = subprocess.run([str(submission)], input=data, text=True,
								 capture_output=True, timeout=4)
			elapsed = time.perf_counter() - before
			assert run.returncode == 0, (run.returncode, run.stdout, run.stderr, data)
			validate(data, run.stdout, expected)
			stats = run.stderr.split()
			nodes, pivots, solver_time = int(stats[0]), int(stats[1]), float(stats[2])
			worst_wall = max(worst_wall, elapsed)
			max_nodes, max_pivots = max(max_nodes, nodes), max(max_pivots, pivots)
			if solver_time > worst_solver[0]:
				worst_solver = (solver_time, nodes, pivots, kind)
		print(f"CF8C: {len(cases)} cases passed (including {args.max_cases} maximum-size); "
			  f"worst process wall time {worst_wall:.4f}s; "
			  f"worst solver/output time {worst_solver[0]:.4f}s, "
			  f"{worst_solver[1]} nodes, {worst_solver[2]} pivots ({worst_solver[3]}); "
			  f"max nodes/pivots {max_nodes}/{max_pivots}")


if __name__ == "__main__":
	main()
