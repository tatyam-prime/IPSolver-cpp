#!/usr/bin/env python3
"""Compile the generated single-file submissions and check their actual stdin/stdout."""
import argparse
import itertools
import math
import pathlib
import random
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]


def pack_dp(a):
	dp = [(len(a) + 1, 0)] * (1 << len(a))
	dp[0] = (1, 0)
	for mask in range(1, len(dp)):
		for i, size in enumerate(a):
			if mask >> i & 1:
				bins, load = dp[mask ^ (1 << i)]
				candidate = (bins + 1, size) if load + size > 12 else (bins, load + size)
				dp[mask] = min(dp[mask], candidate)
	return dp[-1][0]


def cover_brute(n, edges):
	return min(mask.bit_count() for mask in range(1 << n)
			   if all(mask >> u & 1 or mask >> v & 1 for u, v in edges))


def lcm_dp(a):
	n = len(a)
	dp = [[-1] * n for _ in range(1 << n)]
	for i in range(n):
		dp[1 << i][i] = 0
	for mask in range(1, 1 << n):
		for i in range(n):
			if dp[mask][i] < 0:
				continue
			for j in range(n):
				if not (mask >> j & 1):
					next_mask = mask | (1 << j)
					dp[next_mask][j] = max(dp[next_mask][j], dp[mask][i] + math.lcm(a[i], a[j]))
	return max(dp[-1])


def clues(x):
	r, c = len(x), len(x[0])
	return [[sum(x[u][v] for u in range(max(0, i - 1), min(r, i + 2))
				 for v in range(max(0, j - 1), min(c, j + 2)))
			 for j in range(c)] for i in range(r)]


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument('--random-cases', type=int, default=100)
	args = parser.parse_args()
	rng = random.Random(20261004)
	packing = [([8, 2, 2, 3, 9], 2), ([11, 1, 1], 2)]
	for _ in range(args.random_cases):
		a = [rng.randint(1, 12) for _ in range(rng.randint(1, 12))]
		packing.append((a, pack_dp(a)))
	packing += [([12] * 1000, 1000), ([7] * 1000, 1000),
				([1] * 1000, 84), ([i % 12 + 1 for i in range(1000)], 541)]
	covers = [(3, [(0, 1), (0, 2)], 1),
			  (6, [(0, 1), (0, 2), (0, 3), (1, 4), (1, 5)], 2)]
	for _ in range(args.random_cases):
		n = rng.randint(2, 12)
		density = rng.random()
		e = [(i, j) for i, j in itertools.combinations(range(n), 2)
			 if rng.random() < density]
		if not e:
			e = [(0, 1)]
		covers.append((n, e, cover_brute(n, e)))
	covers += [(500, [(i, j) for i in range(30) for j in range(i + 1, 500)], 30),
			   (500, list(itertools.combinations(range(30), 2)), 29),
			   (500, [(i, j) for i in range(30) for j in range(i + 1, 30)
					  if i // 3 == j // 3], 20)]
	mines = [([[2, 2, 1], [3, 4, 3], [2, 3, 2]], 1),
			 ([[1, 2, 1, 1], [2, 3, 3, 2], [2, 2, 2, 1]], 1)]
	for _ in range(args.random_cases):
		r, c = rng.choice([3, 5, 7, 11, 17]), rng.randint(3, 17)
		x = [[rng.randrange(2) for _ in range(c)] for _ in range(r)]
		mines.append((clues(x), sum(x[r // 2])))
	for r in [47, 49]:
		for family in range(4):
			x = [[rng.randrange(2) if family == 3 else
				  (i + j) % 2 if family == 2 else family
				  for j in range(r)] for i in range(r)]
			mines.append((clues(x), sum(x[r // 2])))
	packing_input = str(len(packing)) + '\n' + ''.join(
		f'{len(a)}\n' + ' '.join(map(str, a)) + '\n' for a, _ in packing)
	cover_input = ''.join(f'{n} {len(e)}\n' + ''.join(f'{u+1} {v+1}\n' for u, v in e)
						  for n, e, _ in covers)
	mine_input = str(len(mines)) + '\n' + ''.join(
		f'{len(a)} {len(a[0])}\n' + ''.join(' '.join(map(str, row)) + '\n' for row in a)
		for a, _ in mines)
	checks = [('qoj10266', packing_input, [str(v) for _, v in packing]),
			  ('qoj3699', cover_input, [str(v) for _, _, v in covers]),
			  ('qoj5785', mine_input, [f'Case #{i}: {v}' for i, (_, v) in enumerate(mines, 1)])]
	lcms = [([x], 0) for x in range(1, 8)]
	lcms += [([2, 2, 3, 3, 5, 5, 7, 7], 153), ([1, 1, 2, 2, 3, 3], 23)]
	all_types = list(range(1, 8))
	lcms.append((all_types, lcm_dp(all_types)))
	for _ in range(args.random_cases):
		a = [rng.randint(1, 7) for _ in range(rng.randint(1, 11))]
		lcms.append((a, lcm_dp(a)))
	lcms += [([x] * 100000, 99999 * x) for x in range(1, 8)]
	lcms += [([6] * 50000 + [7] * 50000, 99999 * 42),
			 ([1] * 99999 + [7], 99997 + 14)]
	# Keep each process within t <= 10 and sum(n) <= 100000.
	batch, elements = [], 0
	for case in lcms + [None]:
		if batch and (case is None or len(batch) == 10 or elements + len(case[0]) > 100000):
			data = str(len(batch)) + '\n' + ''.join(
				f'{len(a)}\n' + ' '.join(map(str, a)) + '\n' for a, _ in batch)
			checks.append(('qoj20736', data, [str(value) for _, value in batch]))
			batch, elements = [], 0
		if case is not None:
			batch.append(case)
			elements += len(case[0])
	with tempfile.TemporaryDirectory(prefix='ip-qoj-submissions-') as d:
		compiled = set()
		for name, data, expected in checks:
			exe = str(pathlib.Path(d) / name)
			if name not in compiled:
				subprocess.run(['clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Wpedantic',
								str(ROOT / 'build/submissions/qoj' / (name + '.cpp')), '-o', exe], check=True)
				compiled.add(name)
			result = subprocess.run([exe], input=data, text=True, capture_output=True,
									timeout=20, check=True)
			if result.stdout.splitlines() != expected:
				raise AssertionError(f'{name}: output mismatch')
			print(f'{name}: {len(expected)} standalone cases passed')


if __name__ == '__main__':
	main()
