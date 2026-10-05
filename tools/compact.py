#!/usr/bin/env python3
"""Generate a contest paste header without changing any C++ tokens."""
import pathlib
import re

root = pathlib.Path(__file__).resolve().parent.parent
source = (root / "include/ip_solver.hpp").read_text()
parts = re.split(r'("(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/)', source)
source = "".join(" " if part.startswith(("//", "/*")) else part for part in parts)
lines, body = [], []
for line in source.splitlines():
	line = line.strip()
	if line.startswith("#"):
		lines.append(line)
	elif line:
		body.append(line)
# Leave whitespace inside string literals untouched; punctuation cannot combine
# into another token when only whitespace around these characters is removed.
body = " ".join(body)
parts = re.split(r'("(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')', body)
for i in range(0, len(parts), 2):
	parts[i] = re.sub(r"\s*([{}();,])\s*", r"\1", parts[i])
body = "".join(parts)
compact = "\n".join(lines) + "\n" + body + "\n"
for destination in (root / "include/ip_solver.min.hpp", root / "single_include/ip_solver.hpp"):
	destination.parent.mkdir(exist_ok=True)
	destination.write_text(compact)
	print(f"{destination.relative_to(root)}: {destination.stat().st_size} bytes")
