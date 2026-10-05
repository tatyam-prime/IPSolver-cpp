// https://atcoder.jp/contests/abc338/tasks/abc338_f
#include "ip_solver.hpp"
#include <iostream>

using namespace std;

using Weight = long long;
struct DirectedEdge {
	int from, to;
	Weight cost;
};

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	int n, m;
	cin >> n >> m;
	vector<DirectedEdge> edges(m);
	for (auto& e : edges) {
		cin >> e.from >> e.to >> e.cost;
		--e.from;
		--e.to;
	}
	ip::Options options;
	constexpr Weight infinity = 1LL << 60;
	vector<vector<Weight>> d(n, vector<Weight>(n, infinity));
	for (int i = 0; i < n; ++i)
		d[i][i] = 0;
	for (auto e : edges) {
		d[e.from][e.to] = e.cost;
	}
	for (int k = 0; k < n; ++k)
		for (int i = 0; i < n; ++i)
			for (int j = 0; j < n; ++j)
				if (d[i][k] != infinity && d[k][j] != infinity && d[i][j] > d[i][k] + d[k][j]) {
					d[i][j] = d[i][k] + d[k][j];
				}
	for (int i = 0; i < n; ++i)
		for (int j = 0; j < i; ++j)
			if (d[i][j] == infinity && d[j][i] == infinity) {
				cout << "No\n";
				return 0;
			}

	auto path_cost = [&](const vector<int>& path) {
		Weight cost = 0;
		for (int i = 1; i < int(path.size()); ++i) {
			if (d[path[i - 1]][path[i]] == infinity)
				return infinity;
			cost += d[path[i - 1]][path[i]];
		}
		return cost;
	};
	Weight best = infinity;
	vector<int> seed;
	for (int start = 0; start < n; ++start) {
		bool possible = true;
		for (int j = 0; j < n; ++j)
			possible &= d[start][j] != infinity;
		if (!possible)
			continue;
		vector<int> path{start}, used(n);
		used[start] = 1;
		while (int(path.size()) < n) {
			int next = -1;
			for (int j = 0; j < n; ++j)
				if (!used[j]) {
					bool reachable = true;
					for (int k = 0; k < n; ++k)
						if (!used[k])
							reachable &= d[j][k] != infinity;
					if (reachable && (next < 0 || d[path.back()][j] < d[path.back()][next]))
						next = j;
				}
			path.push_back(next);
			used[next] = 1;
		}
		Weight cost = path_cost(path);
		if (cost < best) {
			best = cost;
			seed = std::move(path);
		}
	}
	// Directed reversals: recompute every affected arc, including internal arcs.
	for (;;) {
		Weight improved = best;
		vector<int> next = seed;
		for (int i = 0; i < n; ++i)
			for (int j = i + 1; j < n; ++j) {
				auto path = seed;
				reverse(path.begin() + i, path.begin() + j + 1);
				Weight cost = path_cost(path);
				if (cost < improved) {
					improved = cost;
					next = std::move(path);
				}
			}
		if (improved == best)
			break;
		best = improved;
		seed = std::move(next);
	}

	// Dummy vertex n turns an arbitrary-endpoint path into a cycle.
	int vertices = n + 1;
	vector<pair<int, int>> arc;
	vector<Weight> cost;
	vector<vector<int>> id(vertices, vector<int>(vertices, -1));
	for (int i = 0; i < vertices; ++i)
		for (int j = 0; j < vertices; ++j)
			if (i != j && (i == n || j == n || d[i][j] != infinity)) {
				id[i][j] = int(arc.size());
				arc.push_back({i, j});
				cost.push_back(i == n || j == n ? 0 : d[i][j]);
			}
	vector<Weight> row_min(vertices, infinity);
	for (int a = 0; a < int(arc.size()); ++a)
		row_min[arc[a].first] = min(row_min[arc[a].first], cost[a]);
	Weight offset = 0;
	for (Weight x : row_min)
		offset += x;
	ip::Vec objective(arc.size());
	for (int a = 0; a < int(arc.size()); ++a)
		objective[a] = -(cost[a] - row_min[arc[a].first]);
	ip::Solver solver(objective);
	for (int a = 0; a < int(arc.size()); ++a)
		solver.bounds(a, 0, 1);
	for (int i = 0; i < vertices; ++i) {
		ip::Vec out(arc.size()), in(arc.size());
		for (int a = 0; a < int(arc.size()); ++a) {
			out[a] = arc[a].first == i;
			in[a] = arc[a].second == i;
		}
		solver.add_eq(out, 1);
		if (i < n)
			solver.add_eq(in, 1); // Last incoming degree follows from the others.
	}
	options.initial_solution.assign(arc.size(), 0);
	options.initial_solution[id[n][seed.front()]] = 1;
	for (int i = 1; i < n; ++i)
		options.initial_solution[id[seed[i - 1]][seed[i]]] = 1;
	options.initial_solution[id[seed.back()][n]] = 1;
	options.cuts = 0;
	vector<bool> added(1 << n);
	for (;;) {
		auto r = solver.maximize(options);
		vector<int> successor(vertices, -1);
		for (int a = 0; a < int(arc.size()); ++a)
			if (r.x[a] > 0.5) {
				auto [i, j] = arc[a];
				successor[i] = j;
			}
		vector<bool> seen(vertices);
		vector<unsigned> components;
		for (int i = 0; i < vertices; ++i)
			if (!seen[i]) {
				unsigned mask = 0;
				int current = i;
				do {
					seen[current] = true;
					mask |= 1u << current;
					current = successor[current];
				} while (current != i);
				components.push_back(mask);
			}
		if (components.size() == 1) {
			cout << llround(-r.objective) + offset << '\n';
			return 0;
		}
		for (unsigned mask : components) {
			if (mask >> n & 1)
				mask ^= (1u << vertices) - 1;
			if (added[mask])
				continue;
			added[mask] = true;
			ip::Vec row(arc.size());
			for (int a = 0; a < int(arc.size()); ++a) {
				auto [i, j] = arc[a];
				row[a] = (mask >> i & 1) && !(mask >> j & 1);
			}
			solver.add_ge(row, 1);
		}
	}
}
