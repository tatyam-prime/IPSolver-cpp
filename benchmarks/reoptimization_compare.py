#!/usr/bin/env python3
"""Compare root reuse with a pre-change header using the same oracle tests."""
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
	parser.add_argument("--baseline-ref", default="00a5097566935825871c595f38651760ca94bc1b")
	parser.add_argument("--repeats", type=int, default=3)
	parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
	parser.add_argument("--output", type=Path, default=ROOT / "benchmarks/reoptimization.csv")
	args = parser.parse_args()
	compiler = shlex.split(args.cxx)
	print(subprocess.check_output(compiler + ["--version"], text=True).splitlines()[0], flush=True)
	records = []
	with tempfile.TemporaryDirectory(prefix="ip-reoptimization-") as temp:
		directory = Path(temp)
		(directory / "ip_solver.hpp").write_bytes(subprocess.check_output(
			["git", "show", args.baseline_ref + ":include/ip_solver.hpp"], cwd=ROOT))
		for problem, small, large, size in [("abc180_e", 100, 40, "17"), ("abc338_f", 2000, 60, "20")]:
			binaries = {}
			for mode, include in [("cold", directory), ("reuse", ROOT / "include")]:
				binary = directory / (problem + "_" + mode)
				subprocess.run(compiler + ["-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic",
					"-I" + str(include), str(ROOT / "tests" / ("test_" + problem + ".cpp")),
					"-o", str(binary)], check=True)
				binaries[mode] = binary
			for repeat in range(args.repeats):
				for mode in (["cold", "reuse"] if repeat % 2 == 0 else ["reuse", "cold"]):
					run = subprocess.run([str(binaries[mode]), str(small), str(large)],
						text=True, capture_output=True, check=True, timeout=180)
					rows = [row for row in csv.DictReader(io.StringIO(run.stdout)) if row["n"] == size]
					assert len(rows) == large + (7 if problem == "abc180_e" else 11)
					record = dict(problem=problem, mode=mode, repeat=repeat + 1, cases=len(rows),
						sum_seconds=sum(float(row["seconds"]) for row in rows),
						max_seconds=max(float(row["seconds"]) for row in rows))
					for field in ["nodes", "pivots", "solves", "subtour_cuts"]:
						record["sum_" + field] = sum(int(row[field]) for row in rows)
					records.append(record)
					print(problem, mode, repeat + 1, record, run.stderr.strip(), flush=True)
	with args.output.open("w", newline="") as output:
		writer = csv.DictWriter(output, fieldnames=records[0].keys(), lineterminator="\n")
		writer.writeheader()
		writer.writerows(records)


if __name__ == "__main__":
	main()
