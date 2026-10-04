#!/usr/bin/env python3
"""Reproduce the ABC180E hint/GMI comparison without editing repository sources."""
import argparse
import csv
import io
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
	parser.add_argument("--random-cases", type=int, default=100)
	parser.add_argument("--max-cases", type=int, default=40)
	parser.add_argument("--output", type=Path,
						default=ROOT / "benchmarks/abc180_e_comparison.csv")
	args = parser.parse_args()
	compiler = shlex.split(args.cxx)
	version = subprocess.run(compiler + ["--version"], text=True, capture_output=True,
							 check=True).stdout.splitlines()[0]
	print(version)
	source = (ROOT / "examples/atcoder/abc180_e.cpp").read_text()
	test = (ROOT / "tests/test_abc180_e.cpp").read_text()
	cuts_line = "\toptions.cuts=0;\n"
	hint_lines = ("\toptions.initial_solution.assign(edge.size(),0);\n"
				  "\tfor (int i=0; i<n; ++i) "
				  "options.initial_solution[id[seed[i]][seed[(i+1)%n]]]=1;\n")
	include_line = '#include "../examples/atcoder/abc180_e.cpp"'
	assert source.count(cuts_line) == source.count(hint_lines) == test.count(include_line) == 1
	variants = {
		"baseline": source,
		"cuts8": source.replace(cuts_line, "\toptions.cuts=8;\n"),
		"nohint": source.replace(hint_lines, ""),
	}
	fields = ["mode", "cases", "sum_seconds", "max_seconds", "sum_nodes",
			  "sum_pivots", "sum_solves", "sum_subtour_cuts"]
	results = []
	with tempfile.TemporaryDirectory(prefix="ip-abc180-compare-") as temp:
		directory = Path(temp)
		for mode, code in variants.items():
			main_path, test_path = directory / f"{mode}.cpp", directory / f"{mode}_test.cpp"
			binary = directory / mode
			main_path.write_text(code)
			test_path.write_text(test.replace(include_line, f'#include "{main_path}"'))
			subprocess.run(compiler + ["-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic",
									   "-I" + str(ROOT / "include"), str(test_path),
									   "-o", str(binary)], check=True)
			run = subprocess.run([str(binary), str(args.random_cases), str(args.max_cases)],
								 text=True, capture_output=True, check=True, timeout=120)
			rows = list(csv.DictReader(io.StringIO(run.stdout)))
			# The test prints sample 3, six structures, and every N=17 random case.
			assert len(rows) == args.max_cases + 7
			record = {
				"mode": mode, "cases": len(rows),
				"sum_seconds": sum(float(row["seconds"]) for row in rows),
				"max_seconds": max(float(row["seconds"]) for row in rows),
			}
			for field in ["nodes", "pivots", "solves", "subtour_cuts"]:
				record["sum_" + field] = sum(int(row[field]) for row in rows)
			results.append(record)
			print(mode + ": " + run.stderr.strip())
	with args.output.open("w", newline="") as output:
		writer = csv.DictWriter(output, fieldnames=fields, lineterminator="\n")
		writer.writeheader()
		writer.writerows(results)
	print(f"Saved {args.output}")


if __name__ == "__main__":
	main()
