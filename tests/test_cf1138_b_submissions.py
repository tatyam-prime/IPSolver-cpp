#!/usr/bin/env python3
"""Validate Circus indices against exhaustive choice and a count oracle."""
import argparse
import itertools
import os
from pathlib import Path
import random
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def possible(c, a):
    n = len(c)
    if n <= 14:
        return any(sum(int(c[i]) + int(a[i]) for i in chosen) == a.count('1')
                   for chosen in itertools.combinations(range(n), n // 2))
    count = {pair: sum(x == pair[0] and y == pair[1] for x, y in zip(c, a))
             for pair in ['00', '01', '10', '11']}
    for both in range(count['11'] + 1):
        one = a.count('1') - 2 * both
        neither = n // 2 - both - one
        if 0 <= one <= count['01'] + count['10'] and 0 <= neither <= count['00']:
            return True
    return False


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--standalone', action='store_true')
    parser.add_argument('--small', type=int, default=200)
    parser.add_argument('--stress', type=int, default=30)
    args = parser.parse_args()
    cases = [('0011', '0101'), ('000000', '111111'),
             ('0011', '1100'), ('00100101', '01111100')]
    cases += [(c * 5000, a * 5000) for c, a in ['00', '01', '10', '11']]
    cases.append(('1' * 2501 + '0' * 2499,) * 2)
    rng = random.Random(113820261004)
    for i in range(args.small + args.stress):
        n = 2 * rng.randint(1, 7) if i < args.small else 5000
        cases.append(tuple(''.join(rng.choice('01') for _ in range(n)) for _ in range(2)))
    with tempfile.TemporaryDirectory(prefix='ip-circus-') as directory:
        binary = Path(directory) / 'submission'
        source = ROOT / ('build/submissions/codeforces' if args.standalone else 'examples/codeforces') / 'cf1138_b.cpp'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-Wall',
                        '-Wextra', '-Wpedantic', '-I' + str(ROOT / 'include'),
                        str(source), '-o', str(binary)], check=True)
        for c, a in cases:
            expected = possible(c, a)
            run = subprocess.run([str(binary)], input=f'{len(c)}\n{c}\n{a}\n',
                                 text=True, capture_output=True, timeout=1)
            assert run.returncode == 0, (c, a, run.stderr)
            indices = list(map(int, run.stdout.split()))
            if indices == [-1]:
                assert not expected, (c, a, 'wrong infeasible')
                continue
            assert expected and len(indices) == len(c) // 2
            assert len(set(indices)) == len(indices) and all(1 <= i <= len(c) for i in indices)
            first = set(i - 1 for i in indices)
            assert sum(c[i] == '1' for i in first) == sum(a[i] == '1' for i in range(len(a)) if i not in first)
    print(f'CF1138B submissions: {len(cases)} cases passed')


if __name__ == '__main__':
    main()
