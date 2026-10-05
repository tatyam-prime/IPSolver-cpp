#!/usr/bin/env python3
"""Generate standalone contest sources while preserving comments and whitespace."""
import pathlib

root = pathlib.Path(__file__).resolve().parent.parent
header = (root / "include/ip_solver.hpp").read_text().replace("#pragma once\n", "", 1)
for source in sorted((root / "examples").glob("*/*.cpp")):
	destination = root / "build/submissions" / source.relative_to(root / "examples")
	destination.parent.mkdir(parents=True, exist_ok=True)
	destination.write_text(source.read_text().replace('#include "ip_solver.hpp"\n', header, 1))
	print(f"{destination.relative_to(root)}: {destination.stat().st_size} bytes")
