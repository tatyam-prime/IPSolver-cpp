#!/usr/bin/env python3
"""Check ABC354 G and CF417 D contest input/output against subset enumeration."""
import argparse
import itertools
import os
from pathlib import Path
import random
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def strings_oracle(strings, weights):
	best = 0
	for mask in range(1 << len(strings)):
		chosen = [i for i in range(len(strings)) if mask >> i & 1]
		if all(strings[i] not in strings[j] and strings[j] not in strings[i]
			   for i, j in itertools.combinations(chosen, 2)):
			best = max(best, sum(weights[i] for i in chosen))
	return best


def friends_oracle(m, b, friends):
	best = None
	for mask in range(1 << len(friends)):
		covered = cost = monitors = 0
		for i, (price, level, problems) in enumerate(friends):
			if mask >> i & 1:
				cost += price
				monitors = max(monitors, level)
				covered |= problems
		if covered == (1 << m) - 1:
			value = cost + b * monitors
			best = value if best is None else min(best, value)
	return -1 if best is None else best


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument('--standalone', action='store_true')
	parser.add_argument('--random-cases', type=int, default=100)
	args = parser.parse_args()
	rng = random.Random(354417)
	strings_cases = [(['a'], [1000000000]),
					 (['a', 'a', 'aa', 'b'], [1, 8, 6, 4]),
					 (['a', 'aa', 'aaa', 'aaaa'], [1, 2, 3, 4]),
					 (['ab', 'bc', 'abc', 'b', 'c'], [7, 5, 11, 3, 2])]
	friends_cases = [(1, 1, [(1, 1, 0)]),
					 (2, 0, [(5, 100, 1), (7, 1, 2), (20, 1, 3)]),
					 (2, 1000000000, [(1000000000, 1000000000, 3)]),
					 (3, 1, [(1000000000, 1, 3), (999999999, 2, 6),
							 (1000000000, 3, 5)])]
	for _ in range(args.random_cases):
		n = rng.randint(1, 10)
		strings = [''.join(rng.choice('abc') for _ in range(rng.randint(1, 6)))
				   for _ in range(n)]
		strings_cases.append((strings, [rng.randint(1, 1000000000) for _ in range(n)]))
		m = rng.randint(1, 6)
		b = rng.choice([0, 1, rng.randint(1, 1000000000)])
		friends = [(rng.randint(1, 1000000000), rng.randint(1, 1000000000),
					rng.randrange(1 << m)) for _ in range(rng.randint(1, 10))]
		friends_cases.append((m, b, friends))

	with tempfile.TemporaryDirectory(prefix='ip-selection-') as directory:
		binaries = {}
		for site, name in [('atcoder', 'abc354_g'), ('codeforces', 'cf417_d')]:
			base = ROOT / ('build/submissions' if args.standalone else 'examples')
			binary = Path(directory) / name
			subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2',
							'-Wall', '-Wextra', '-Wpedantic', '-I' + str(ROOT / 'include'),
							str(base / site / (name + '.cpp')), '-o', str(binary)], check=True)
			binaries[name] = binary

		def check(name, data, expected):
			result = subprocess.run([str(binaries[name])], input=data, text=True,
									capture_output=True, timeout=3)
			assert result.returncode == 0 and result.stdout.strip() == str(expected), (
				name, data, expected, result.returncode, result.stdout, result.stderr)

		for strings, weights in strings_cases:
			data = str(len(strings)) + '\n' + '\n'.join(strings) + '\n'
			data += ' '.join(map(str, weights)) + '\n'
			check('abc354_g', data, strings_oracle(strings, weights))
		for m, b, friends in friends_cases:
			data = f'{len(friends)} {m} {b}\n'
			for cost, level, mask in friends:
				problems = [str(i + 1) for i in range(m) if mask >> i & 1]
				data += f'{cost} {level} {len(problems)}\n' + ' '.join(problems) + '\n'
			check('cf417_d', data, friends_oracle(m, b, friends))
	print(f'ABC354 G: {len(strings_cases)} stdin/stdout cases passed')
	print(f'CF417 D: {len(friends_cases)} stdin/stdout cases passed')


if __name__ == '__main__':
	main()
