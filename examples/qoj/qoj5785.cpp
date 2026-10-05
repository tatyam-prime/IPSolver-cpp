// https://qoj.ac/problem/5785
#include "ip_solver.hpp"
#include <iostream>

using namespace std;

// A_n is tridiagonal; if n%3==2 its null vector is (1,-1,0,...).
vector<int> mine_line(const vector<int>& b) {
	int n = int(b.size());
	vector<int> p(n), h(n);
	h[0] = 1;
	for (int i = 0; i + 1 < n; ++i) {
		p[i + 1] = b[i] - p[i] - (i ? p[i - 1] : 0);
		h[i + 1] = -h[i] - (i ? h[i - 1] : 0);
	}
	int residual = b.back() - p.back() - (n > 1 ? p[n - 2] : 0);
	int coefficient = h.back() + (n > 1 ? h[n - 2] : 0);
	if (coefficient)
		for (int i = 0; i < n; ++i)
			p[i] += h[i] * (residual / coefficient);
	return p;
}
int mine_layer(const vector<vector<int>>& clue) {
	int r = int(clue.size()), c = int(clue[0].size());
	bool vr = r % 3 == 2, hc = c % 3 == 2;
	vector<vector<int>> q(r, vector<int>(c)), y;
	for (const auto& row : clue)
		y.push_back(mine_line(row));
	for (int j = 0; j < c; ++j) {
		vector<int> b(r);
		for (int i = 0; i < r; ++i)
			b[i] = y[i][j];
		auto v = mine_line(b);
		for (int i = 0; i < r; ++i)
			q[i][j] = v[i];
	}
	auto h = [](int i) { return i % 3 == 0 ? 1 : i % 3 == 1 ? -1 : 0; };
	int n = (hc ? r : 0) + (vr ? c - int(hc) : 0), offset = 0;
	ip::Vec objective(n);
	auto terms = [&](int i, int j) {
		vector<pair<int, int>> a;
		if (hc && h(j))
			a.push_back({i, h(j)});
		if (vr && h(i) && (!hc || j))
			a.push_back({(hc ? r : 0) + j - int(hc), h(i)});
		return a;
	};
	for (int j = 0; j < c; ++j) {
		offset += q[r / 2][j];
		for (auto [k, a] : terms(r / 2, j))
			objective[k] += a;
	}
	ip::Solver s(objective);
	for (int k = 0; k < n; ++k)
		s.bounds(k, hc && k >= r ? -1 : 0, hc && k >= r ? 2 : 1);
	for (int i = 0; i < r; ++i)
		for (int j = 0; j < c; ++j) {
			auto a = terms(i, j);
			if (!a.empty()) {
				ip::Vec row(n);
				for (auto [k, v] : a)
					row[k] = v;
				s.add_le(row, 1 - q[i][j]);
				s.add_ge(row, -q[i][j]);
			}
		}
	// The remaining constraints are a signed bipartite incidence matrix (TU).
	ip::Options options;
	options.cuts = 0;
	auto result = s.maximize(options);
	return offset + int(llround(result.objective));
}

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);
	int t;
	cin >> t;
	for (int k = 1; k <= t; ++k) {
		int r, c;
		cin >> r >> c;
		vector<vector<int>> clue(r, vector<int>(c));
		for (auto& row : clue)
			for (int& x : row)
				cin >> x;
		cout << "Case #" << k << ": " << mine_layer(clue) << '\n';
	}
}
