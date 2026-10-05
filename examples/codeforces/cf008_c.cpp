// https://codeforces.com/problemset/problem/8/C
#include "ip_solver.hpp"
#include <iostream>

using namespace std;

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);
	int sx, sy, n;
	cin >> sx >> sy >> n;
	vector<int> x(n), y(n), d(n), used(n);
	long long answer = 0;
	for (int i = 0; i < n; ++i) {
		cin >> x[i] >> y[i];
		x[i] -= sx;
		y[i] -= sy;
		d[i] = x[i] * x[i] + y[i] * y[i];
		answer += 2 * d[i]; // Initially collect every object in its own trip.
	}
	vector<pair<int, int>> edge;
	ip::Vec gain;
	for (int i = 0; i < n; ++i)
		for (int j = i + 1; j < n; ++j) {
			int saving = 2 * (x[i] * x[j] + y[i] * y[j]);
			if (saving > 0) {
				edge.push_back({i, j});
				gain.push_back(saving);
			}
		}
	ip::Solver solver(gain);
	for (int i = 0; i < n; ++i) {
		ip::Vec row(edge.size());
		for (int e = 0; e < int(edge.size()); ++e)
			if (edge[e].first == i || edge[e].second == i)
				row[e] = 1;
		solver.add_le(row, 1); // Nonnegative integers + degree <= 1 imply binary.
	}
	auto result = solver.maximize();
	vector<int> route{0};
	for (int e = 0; e < int(edge.size()); ++e)
		if (result.x[e] > 0.5) {
			auto [i, j] = edge[e];
			used[i] = used[j] = 1;
			answer -= llround(gain[e]);
			route.insert(route.end(), {i + 1, j + 1, 0});
		}
	for (int i = 0; i < n; ++i)
		if (!used[i])
			route.insert(route.end(), {i + 1, 0});
	cout << answer << '\n';
	for (int v : route)
		cout << v << ' ';
	cout << '\n';
}
