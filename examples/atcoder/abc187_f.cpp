// https://atcoder.jp/contests/abc187/tasks/abc187_f
#include "ip_solver.hpp"
#include <cstdint>
#include <functional>
#include <iostream>

using namespace std;

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	int n, m;
	cin >> n >> m;
	vector<pair<int, int>> edges(m);
	for (auto& [u, v] : edges) {
		cin >> u >> v;
		--u;
		--v;
	}
	ip::Options options;
	vector<uint32_t> adj(n);
	for (auto [u, v] : edges) {
		adj[u] |= 1u << v;
		adj[v] |= 1u << u;
	}
	vector<uint32_t> patterns;
	// Bron-Kerbosch: enumerate inclusion-maximal cliques, including singletons.
	function<void(uint32_t, uint32_t, uint32_t)> dfs = [&](uint32_t r, uint32_t p, uint32_t x) {
		if (!(p | x)) {
			patterns.push_back(r);
			return;
		}
		int pivot = -1, score = -1;
		for (uint32_t left = p | x; left; left &= left - 1) {
			int u = __builtin_ctz(left), value = __builtin_popcount(p & adj[u]);
			if (value > score) {
				score = value;
				pivot = u;
			}
		}
		for (uint32_t left = p & ~adj[pivot]; left; left &= left - 1) {
			uint32_t bit = left & -left;
			int v = __builtin_ctz(bit);
			dfs(r | bit, p & adj[v], x & adj[v]);
			p ^= bit;
			x |= bit;
		}
	};
	uint32_t all = (1u << n) - 1;
	dfs(0, all, 0);
	int k = int(patterns.size()), upper = 0;
	ip::Vec initial(k);
	for (uint32_t remaining = all; remaining;) {
		int best = 0;
		for (int j = 1; j < k; ++j)
			if (__builtin_popcount(patterns[j] & remaining) >
				__builtin_popcount(patterns[best] & remaining))
				best = j;
		++initial[best];
		++upper;
		remaining &= ~patterns[best];
	}
	ip::Solver solver(ip::Vec(k, -1));
	for (int v = 0; v < n; ++v) {
		ip::Vec row(k);
		for (int j = 0; j < k; ++j)
			row[j] = (patterns[j] >> v) & 1u;
		solver.add_ge(std::move(row), 1);
	}
	// Bounds all pattern counts with one row, rather than k upper-bound rows.
	solver.add_le(ip::Vec(k, 1), upper);
	options.initial_solution = std::move(initial);
	auto result = solver.maximize(options);
	cout << llround(-result.objective) << '\n';
}
