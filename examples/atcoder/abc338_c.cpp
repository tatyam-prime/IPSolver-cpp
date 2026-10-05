// https://atcoder.jp/contests/abc338/tasks/abc338_c
#include "ip_solver.hpp"
#include <iostream>

using namespace std;

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);
	int n;
	cin >> n;
	vector<int> q(n), a(n), b(n);
	for (int& x : q)
		cin >> x;
	for (int& x : a)
		cin >> x;
	for (int& x : b)
		cin >> x;
	ip::Solver s({1, 1}); // maximize servings of A + servings of B
	int ua = 1000000, ub = 1000000;
	for (int i = 0; i < n; ++i) {
		s.add_le({double(a[i]), double(b[i])}, q[i]);
		if (a[i])
			ua = min(ua, q[i] / a[i]);
		if (b[i])
			ub = min(ub, q[i] / b[i]);
	}
	s.bounds(0, 0, ua);
	s.bounds(1, 0, ub);
	auto r = s.maximize();
	cout << llround(r.objective) << '\n';
}
