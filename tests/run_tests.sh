#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="$(mktemp -d "${TMPDIR:-/tmp}/ip-solver-tests.XXXXXX")"
trap 'rm -rf "$build_dir"' EXIT

cxx="${CXX:-c++}"
flags=(-std=c++17 -Wall -Wextra -Wpedantic -I "$root_dir/include")
if [[ "${SANITIZE:-0}" == 1 ]]; then
  flags+=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
else
  flags+=(-O2)
fi

"$cxx" "${flags[@]}" "$root_dir/tests/test_ip.cpp" -o "$build_dir/test_ip"
"$build_dir/test_ip" "${1:-1000}"
"$cxx" "${flags[@]}" "$root_dir/tests/test_cut_recovery.cpp" -o "$build_dir/test_cut_recovery"
"$build_dir/test_cut_recovery" "$root_dir/tests/data/cf417_d_gmi_limit.in"
"$cxx" "${flags[@]}" "$root_dir/tests/test_numerical.cpp" -o "$build_dir/test_numerical"
"$build_dir/test_numerical" "$root_dir/tests/data"
