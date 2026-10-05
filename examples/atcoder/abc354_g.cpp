// https://atcoder.jp/contests/abc354/tasks/abc354_g
#include "ip_solver.hpp"
#include <iostream>
#include <map>
#include <string>

using namespace std;

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	int n;
	cin >> n;
	vector<string> input(n);
	vector<long long> weight(n);
	for (auto& s : input)
		cin >> s;
	for (auto& w : weight)
		cin >> w;
	// Equal strings conflict: only the most valuable copy is needed.
	map<string, long long> unique;
	for (int i = 0; i < int(input.size()); ++i)
		unique[input[i]] = max(unique[input[i]], weight[i]);
	vector<string> s;
	ip::Vec c;
	for (const auto& item : unique) {
		s.push_back(item.first);
		c.push_back(double(item.second));
		c.push_back(-double(item.second));
	}
	n = int(s.size());
	vector<vector<int>> contains(n, vector<int>(n));
	for (int i = 0; i < n; ++i)
		for (int j = 0; j < n; ++j)
			contains[i][j] = i != j && s[j].find(s[i]) != string::npos;
	ip::Solver solver(c);
	for (int i = 0; i < n; ++i) {
		solver.bounds(2 * i, 0, 1); // p_i; 0 <= q_i <= p_i makes q_i binary too
		ip::Vec a(2 * n);
		a[2 * i + 1] = 1;
		a[2 * i] = -1;
		solver.add_le(a, 0);
	}
	for (int i = 0; i < n; ++i)
		for (int j = 0; j < n; ++j)
			if (contains[i][j]) {
				bool redundant = false;
				for (int k = 0; k < n; ++k)
					if (contains[i][k] && contains[k][j]) {
						redundant = true;
						break;
					}
				if (redundant)
					continue;
				ip::Vec a(2 * n);
				a[2 * i] = 1;
				a[2 * j + 1] = -1;
				solver.add_le(a, 0); // p_i <= q_j for each cover relation i < j
			}
	// Select i iff p_i-q_i=1. Difference constraints give an integral LP.
	ip::Options options;
	options.cuts = 0;
	auto result = solver.maximize(options);
	cout << llround(result.objective) << '\n';
}
