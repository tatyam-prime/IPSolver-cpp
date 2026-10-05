// https://codeforces.com/problemset/problem/1138/B
#include "ip_solver.hpp"
#include <iostream>
#include <string>

using namespace std;

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	int n;
	string c, a;
	cin >> n >> c >> a;
	vector<int> groups[3];
	int total = 0;
	for (int i = 0; i < n; ++i) {
		groups[c[i] + a[i] - 2 * '0'].push_back(i);
		total += a[i] - '0';
	}
	ip::Solver s(ip::Vec(3));
	for (int k = 0; k < 3; ++k)
		s.bounds(k, 0, groups[k].size());
	s.add_eq({1, 1, 1}, n / 2);
	s.add_eq({0, 1, 2}, total);

	auto result = s.maximize();
	if (result.status == ip::Status::Infeasible) {
		cout << -1 << '\n';
		return 0;
	}
	for (int k = 0; k < 3; ++k) {
		int count = int(llround(result.x[k]));
		for (int j = 0; j < count; ++j)
			cout << groups[k][j] + 1 << ' ';
	}
	cout << '\n';
}
