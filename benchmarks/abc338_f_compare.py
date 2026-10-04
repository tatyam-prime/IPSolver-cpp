#!/usr/bin/env python3
"""Compare ABC338 F solver settings in temporary source copies only."""
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
	parser.add_argument("--random-cases", type=int, default=2000)
	parser.add_argument("--max-cases", type=int, default=60)
	parser.add_argument("--output", type=Path,
						default=ROOT / "benchmarks/abc338_f_comparison.csv")
	args = parser.parse_args()
	compiler = shlex.split(args.cxx)
	print(subprocess.run(compiler + ["--version"], text=True, capture_output=True,
						 check=True).stdout.splitlines()[0])
	source = (ROOT / "examples/atcoder/abc338_f.cpp").read_text()
	test = (ROOT / "tests/test_abc338_f.cpp").read_text()
	cuts_line = "\toptions.cuts=0;\n"
	hint_lines = ("\toptions.initial_solution.assign(arc.size(),0);\n"
				  "\toptions.initial_solution[id[n][seed.front()]]=1;\n"
				  "\tfor (int i=1;i<n;++i) options.initial_solution[id[seed[i-1]][seed[i]]]=1;\n"
				  "\toptions.initial_solution[id[seed.back()][n]]=1;\n")
	shift_line = "\tWeight offset=0; for (Weight x:row_min) offset+=x;\n"
	include_line = '#include "../examples/atcoder/abc338_f.cpp"'
	assert all(source.count(block) == 1 for block in [cuts_line, hint_lines, shift_line])
	assert test.count(include_line) == 1
	variants = {
		"baseline": source,
		"cuts8": source.replace(cuts_line, "\toptions.cuts=8;\n"),
		"nohint": source.replace(hint_lines, ""),
		"noshift": source.replace(shift_line,
								  "\tstd::fill(row_min.begin(),row_min.end(),0);\n" + shift_line),
	}
	fields = ["mode", "cases", "sum_seconds", "max_seconds", "sum_nodes",
			  "sum_pivots", "sum_solves", "sum_subtour_cuts"]
	results = []
	with tempfile.TemporaryDirectory(prefix="ip-abc338-compare-") as temp:
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
								 text=True, capture_output=True, check=True, timeout=180)
			rows = [row for row in csv.DictReader(io.StringIO(run.stdout)) if row["n"] == "20"]
			assert len(rows) == args.max_cases + 11
			record = {"mode": mode, "cases": len(rows),
					  "sum_seconds": sum(float(row["seconds"]) for row in rows),
					  "max_seconds": max(float(row["seconds"]) for row in rows)}
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
