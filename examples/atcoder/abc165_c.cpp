// https://atcoder.jp/contests/abc165/tasks/abc165_c
#include "ip_solver.hpp"
#include <iostream>

using namespace std;

struct Requirement {
	int a, b, c, d;
};

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	int n, m, k;
	cin >> n >> m >> k;
	vector<Requirement> q(k);
	for (auto& r : q) {
		cin >> r.a >> r.b >> r.c >> r.d;
		--r.a;
		--r.b;
	}
	ip::Options options;
	options.cuts = 0;
	int gaps = n - 1, vars = gaps + int(q.size()), cap = m - 1;
	ip::Vec objective(vars);
	for (int i = 0; i < int(q.size()); ++i)
		objective[gaps + i] = q[i].d;
	ip::Solver s(objective);
	for (int j = 0; j < gaps; ++j) {
		s.bounds(j, 0, cap);
		s.continuous(j);
	}
	for (int i = gaps; i < vars; ++i)
		s.bounds(i, 0, 1);
	ip::Vec total(vars);
	fill(total.begin(), total.begin() + gaps, 1);
	s.add_le(total, cap);
	for (int i = 0; i < int(q.size()); ++i) {
		auto r = q[i];
		if (r.c < cap) {
			ip::Vec row(vars);
			for (int j = r.a; j < r.b; ++j)
				row[j] = 1;
			row[gaps + i] = cap - r.c;
			s.add_le(row, cap);
		}
		if (r.c > 0) {
			ip::Vec row(vars);
			for (int j = r.a; j < r.b; ++j)
				row[j] = -1;
			row[gaps + i] = r.c;
			s.add_le(row, 0);
		}
	}
	for (int a = 0; a < n; ++a)
		for (int b = a + 1; b < n; ++b) {
			ip::Vec row(vars);
			int count = 0;
			for (int i = 0; i < int(q.size()); ++i)
				if (q[i].a == a && q[i].b == b)
					row[gaps + i] = 1, ++count;
			if (count > 1) {
				s.add_le(row, 1);
			}
		}
	{
		auto score = [&](const vector<int>& a) {
			int value = 0;
			for (auto r : q)
				if (a[r.b] - a[r.a] == r.c)
					value += r.d;
			return value;
		};
		vector<int> best(n, 1);
		int best_score = score(best);
		uint32_t state = 1652026104;
		// Coordinate improvement from deterministic monotone starting points.
		for (int start = 0; start < 24; ++start) {
			vector<int> a(n, 1);
			if (start) {
				for (int& x : a) {
					state ^= state << 13;
					state ^= state >> 17;
					state ^= state << 5;
					x = 1 + int(state % unsigned(m));
				}
				sort(a.begin(), a.end());
			}
			int value = score(a);
			for (;;) {
				int gain = 0, position = -1, chosen = 0;
				for (int j = 0; j < n; ++j) {
					int old = a[j], lo = j ? a[j - 1] : 1, hi = j + 1 < n ? a[j + 1] : m;
					for (int x = lo; x <= hi; ++x) {
						a[j] = x;
						int next = score(a);
						if (next - value > gain)
							gain = next - value, position = j, chosen = x;
					}
					a[j] = old;
				}
				if (!gain)
					break;
				a[position] = chosen;
				value += gain;
			}
			if (value > best_score)
				best_score = value, best = std::move(a);
		}
		options.initial_solution.assign(vars, 0);
		for (int j = 0; j < gaps; ++j)
			options.initial_solution[j] = best[j + 1] - best[j];
		for (int i = 0; i < int(q.size()); ++i)
			options.initial_solution[gaps + i] = best[q[i].b] - best[q[i].a] == q[i].c;
	}
	auto r = s.maximize(options);
	cout << llround(r.objective) << '\n';
}
