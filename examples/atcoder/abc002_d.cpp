// https://atcoder.jp/contests/abc002/tasks/abc002_4
#include "ip_solver.hpp"
#include <iostream>

using namespace std;

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);
	int n, m;
	cin >> n >> m;
	vector<vector<int>> edge(n, vector<int>(n));
	while (m--) {
		int u, v;
		cin >> u >> v;
		--u;
		--v;
		edge[u][v] = edge[v][u] = 1;
	}
	ip::Solver s(ip::Vec(n, 1)); // maximize the number of selected vertices
	for (int i = 0; i < n; ++i)
		s.bounds(i, 0, 1);
	for (int i = 0; i < n; ++i)
		for (int j = i + 1; j < n; ++j)
			if (!edge[i][j]) {
				ip::Vec a(n);
				a[i] = a[j] = 1;
				s.add_le(a, 1); // nonadjacent vertices cannot both be selected
			}
	auto r = s.maximize();
	cout << llround(r.objective) << '\n';
}
