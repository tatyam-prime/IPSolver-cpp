// https://qoj.ac/problem/20736
#include "ip_solver.hpp"
#include <array>
#include <iostream>
#include <numeric>

using namespace std;

long long maximize_lcm(const vector<int>& a) {
	array<int, 8> count{};
	for (int x : a)
		++count[x];
	vector<int> values;
	for (int x = 1; x <= 7; ++x)
		if (count[x])
			values.push_back(x);
	int k = int(values.size()), n = int(a.size());
	if (k == 1)
		return 1LL * (n - 1) * values[0];

	// x[i,j] counts undirected adjacencies; x[i,i] is a loop.
	vector<pair<int, int>> edges;
	array<array<int, 8>, 8> id{};
	ip::Vec objective;
	for (int i = 0; i < k; ++i)
		for (int j = i; j < k; ++j) {
			if (i == j && count[values[i]] == 1)
				continue;
			id[values[i]][values[j]] = id[values[j]][values[i]] = int(edges.size());
			edges.push_back({i, j});
			objective.push_back(lcm(values[i], values[j]));
		}
	int m = int(edges.size());
	ip::Solver solver(objective);
	for (int e = 0; e < m; ++e) {
		auto [i, j] = edges[e];
		int upper = i == j ? count[values[i]] - 1 : 2 * min(count[values[i]], count[values[j]]);
		solver.bounds(e, 0, upper);
	}
	solver.add_eq(ip::Vec(m, 1), n - 1);
	for (int i = 0; i < k; ++i) {
		ip::Vec degree(m);
		for (int e = 0; e < m; ++e) {
			auto [u, v] = edges[e];
			degree[e] = int(u == i) + int(v == i);
		}
		// Every occurrence contributes two incidences except the two endpoints.
		solver.add_ge(degree, 2 * count[values[i]] - 2);
		solver.add_le(degree, 2 * count[values[i]]);
	}
	// The total degree deficit is two, so a connected solution has an Euler trail.
	// Its visits to type i are exactly count[i], including both endpoints.

	ip::Options options;
	options.initial_solution.assign(m, 0);
	for (int i = 1; i < n; ++i)
		++options.initial_solution[id[a[i - 1]][a[i]]];
	vector<bool> added(1 << k);
	for (;;) {
		auto result = solver.maximize(options);

		array<unsigned, 7> neighbors{};
		for (int e = 0; e < m; ++e)
			if (result.x[e] > 0.5) {
				auto [i, j] = edges[e];
				neighbors[i] |= 1u << j;
				neighbors[j] |= 1u << i;
			}
		unsigned seen = 0;
		vector<unsigned> components;
		for (int i = 0; i < k; ++i)
			if (!(seen >> i & 1)) {
				unsigned component = 1u << i, previous = 0;
				while (component != previous) {
					previous = component;
					for (int j = 0; j < k; ++j)
						if (component >> j & 1)
							component |= neighbors[j];
				}
				seen |= component;
				components.push_back(component);
			}

		if (components.size() == 1) {
			long long answer = 0;
			for (int e = 0; e < m; ++e)
				answer += llround(result.x[e]) * llround(objective[e]);
			return answer;
		}
		for (unsigned mask : components) {
			// A cut and its complement are identical: keep the side containing type 0.
			if (!(mask & 1))
				mask ^= (1u << k) - 1;
			if (added[mask])
				continue;
			added[mask] = true;
			ip::Vec cut(m);
			for (int e = 0; e < m; ++e) {
				auto [i, j] = edges[e];
				cut[e] = ((mask >> i) & 1) != ((mask >> j) & 1);
			}
			solver.add_ge(cut, 1);
		}
	}
}

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);
	int t;
	cin >> t;
	while (t--) {
		int n;
		cin >> n;
		vector<int> a(n);
		for (int& x : a)
			cin >> x;
		cout << maximize_lcm(a) << '\n';
	}
}
