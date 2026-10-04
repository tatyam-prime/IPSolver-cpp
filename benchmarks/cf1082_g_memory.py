#!/usr/bin/env python3
"""Measure one already-built executable; compiler processes are excluded."""
import argparse
import json
from pathlib import Path
import resource
import subprocess
import sys
import time

parser = argparse.ArgumentParser()
parser.add_argument("binary", type=Path)
parser.add_argument("input", type=Path)
args = parser.parse_args()
start = time.perf_counter()
result = subprocess.run([str(args.binary.resolve())], input=args.input.read_text(),
                        text=True, capture_output=True, check=True)
elapsed = 1000 * (time.perf_counter() - start)
rss = resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss
print(json.dumps({"input": str(args.input), "answer": int(result.stdout), "ms": elapsed,
                  "max_rss": rss, "rss_unit": "bytes" if sys.platform == "darwin" else "native"},
                 indent=2))
