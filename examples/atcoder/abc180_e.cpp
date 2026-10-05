// https://atcoder.jp/contests/abc180/tasks/abc180_e
#include "ip_solver.hpp"
#include <array>
#include <iostream>

using namespace std;

using City = array<int, 3>;

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	int n;
	cin >> n;
	vector<City> p(n);
	for (auto& city : p)
		for (int& x : city)
			cin >> x;
	ip::Options options;
	vector<vector<int>> d(n, vector<int>(n)), id(n, vector<int>(n, -1));
	for (int i = 0; i < n; ++i)
		for (int j = 0; j < n; ++j)
			d[i][j] =
				2 * abs(p[i][0] - p[j][0]) + 2 * abs(p[i][1] - p[j][1]) + abs(p[i][2] - p[j][2]);
	if (n == 2) {
		cout << d[0][1] << '\n';
		return 0;
	}
	vector<pair<int, int>> edge;
	ip::Vec objective;
	for (int i = 0; i < n; ++i)
		for (int j = i + 1; j < n; ++j) {
			id[i][j] = id[j][i] = int(edge.size());
			edge.push_back({i, j});
			objective.push_back(-d[i][j]);
		}
	ip::Solver solver(objective);
	for (int e = 0; e < int(edge.size()); ++e)
		solver.bounds(e, 0, 1);
	for (int i = 0; i < n; ++i) {
		ip::Vec row(edge.size());
		for (int j = 0; j < n; ++j)
			if (i != j)
				row[id[i][j]] = 1;
		solver.add_eq(row, 2);
	}
	// A feasible tour gives an upper bound; IP proves optimality.
	long long best = INT64_MAX;
	vector<int> seed;
	for (int start = 0; start < n; ++start) {
		vector<int> tour{start}, used(n);
		used[start] = 1;
		while (int(tour.size()) < n) {
			int next = -1;
			for (int j = 0; j < n; ++j)
				if (!used[j] && (next < 0 || d[tour.back()][j] < d[tour.back()][next]))
					next = j;
			tour.push_back(next);
			used[next] = 1;
		}
		bool changed = true;
		while (changed) {
			changed = false;
			for (int i = 0; i < n; ++i)
				for (int j = i + 2; j < n; ++j)
					if (i || j < n - 1) {
						int a = tour[i], b = tour[(i + 1) % n], c = tour[j], e = tour[(j + 1) % n];
						if (d[a][c] + d[b][e] < d[a][b] + d[c][e]) {
							reverse(tour.begin() + i + 1, tour.begin() + j + 1);
							changed = true;
						}
					}
		}
		long long cost = 0;
		for (int i = 0; i < n; ++i)
			cost += d[tour[i]][tour[(i + 1) % n]];
		if (cost < best) {
			best = cost;
			seed = std::move(tour);
		}
	}
	options.initial_solution.assign(edge.size(), 0);
	for (int i = 0; i < n; ++i)
		options.initial_solution[id[seed[i]][seed[(i + 1) % n]]] = 1;
	options.cuts = 0;
	vector<bool> added(1 << n);
	for (;;) {
		auto r = solver.maximize(options);
		vector<vector<int>> adj(n);
		for (int e = 0; e < int(edge.size()); ++e)
			if (r.x[e] > 0.5) {
				auto [i, j] = edge[e];
				adj[i].push_back(j);
				adj[j].push_back(i);
			}
		vector<bool> seen(n);
		vector<vector<int>> cycles;
		for (int i = 0; i < n; ++i)
			if (!seen[i]) {
				vector<int> cycle;
				int previous = -1, current = i;
				do {
					cycle.push_back(current);
					seen[current] = true;
					int next = adj[current][0] == previous ? adj[current][1] : adj[current][0];
					previous = current;
					current = next;
				} while (current != i);
				cycles.push_back(std::move(cycle));
			}
		if (cycles.size() == 1) {
			cout << llround(-r.objective) / 2 << '\n';
			return 0;
		}
		for (const auto& cycle : cycles) {
			int mask = 0;
			for (int i : cycle)
				mask |= 1 << i;
			if (mask & 1)
				mask ^= (1 << n) - 1; // S and its complement have the same cut.
			if (added[mask])
				continue;
			added[mask] = true;
			ip::Vec row(edge.size());
			for (int e = 0; e < int(edge.size()); ++e) {
				auto [i, j] = edge[e];
				row[e] = ((mask >> i) ^ (mask >> j)) & 1;
			}
			solver.add_ge(row, 2); // Every tour crosses each nontrivial cut twice.
		}
	}
}
