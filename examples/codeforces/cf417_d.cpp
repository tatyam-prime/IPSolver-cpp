// https://codeforces.com/problemset/problem/417/D
#include "ip_solver.hpp"
#include <iostream>

using namespace std;

using ll = long long;
struct Friend {
	ll cost, monitors;
	int mask;
};

ll cunning_gena(int m, ll b, vector<Friend> a) {
	// A cheaper friend solving a superset with fewer monitors dominates this one.
	vector<Friend> useful;
	for (int i = 0; i < int(a.size()); ++i) {
		bool dominated = false;
		for (int j = 0; j < int(a.size()); ++j)
			if (i != j && (a[i].mask | a[j].mask) == a[j].mask && a[j].cost <= a[i].cost &&
				a[j].monitors <= a[i].monitors &&
				(a[j].cost < a[i].cost || a[j].monitors < a[i].monitors || a[j].mask != a[i].mask ||
				 j < i))
				dominated = true;
		if (!dominated)
			useful.push_back(a[i]);
	}
	a = std::move(useful);
	sort(a.begin(), a.end(),
		 [](const Friend& x, const Friend& y) { return x.monitors < y.monitors; });
	int all = (1 << m) - 1, covered = 0, prefix = 0;
	while (prefix < int(a.size()) && covered != all)
		covered |= a[prefix++].mask;
	if (covered != all)
		return -1;
	ll baseline = a[prefix - 1].monitors;

	// Obtain an exact integer upper bound using at most m friends at baseline.
	ll upper = 0;
	covered = 0;
	while (covered != all) {
		int best = -1, gain = 0;
		for (int i = 0; i < int(a.size()) && a[i].monitors <= baseline; ++i) {
			int g = __builtin_popcount(unsigned(a[i].mask & ~covered));
			if (g && (best < 0 || a[i].cost * gain < a[best].cost * g))
				best = i, gain = g;
		}
		upper += a[best].cost;
		covered |= a[best].mask;
	}
	// The answer can exceed 2^53. Keep baseline*b outside the double model.
	a.erase(remove_if(a.begin(), a.end(),
					  [&](const Friend& x) { return (x.monitors - baseline) * b >= upper; }),
			a.end());
	vector<ll> levels;
	for (const auto& x : a)
		if (x.monitors > baseline && (levels.empty() || levels.back() != x.monitors))
			levels.push_back(x.monitors);
	int n = int(a.size()), k = int(levels.size()), vars = n + k;
	int minimum = 3, most = 0;
	for (int i = 0; i < n; ++i) {
		most = max(most, __builtin_popcount(unsigned(a[i].mask)));
		if (a[i].mask == all)
			minimum = 1;
		for (int j = 0; j < i; ++j)
			if ((a[i].mask | a[j].mask) == all)
				minimum = min(minimum, 2);
	}
	minimum = max(minimum, (m + most - 1) / most);
	ip::Options options;
	options.cuts = 0; // Repeated GMI cuts are unstable on highly degenerate covers.
	ll cheapest = a[0].cost, dearest = 0;
	for (const auto& x : a)
		cheapest = min(cheapest, x.cost), dearest = max(dearest, x.cost);
	ll residual = n * (dearest - cheapest) + (a.back().monitors - baseline) * b;
	int cardinality = -1;
	if (cheapest > residual) {
		// One extra friend costs more than every possible remaining cost change.
		// Solve cardinality first, then subtract its constant large base cost.
		ip::Solver count(ip::Vec(n, -1));
		for (int i = 0; i < n; ++i)
			count.bounds(i, 0, 1);
		for (int p = 0; p < m; ++p) {
			ip::Vec row(n);
			for (int i = 0; i < n; ++i)
				if (a[i].mask >> p & 1)
					row[i] = 1;
			count.add_ge(row, 1);
		}
		count.add_ge(ip::Vec(n, 1), minimum);
		auto r = count.maximize(options);
		cardinality = int(llround(-r.objective));
	}
	ip::Vec objective(vars);
	for (int i = 0; i < n; ++i)
		objective[i] = -(a[i].cost - (cardinality < 0 ? 0 : cheapest));
	for (int j = 0; j < k; ++j)
		objective[n + j] = -(levels[j] - (j ? levels[j - 1] : baseline)) * b;
	ip::Solver s(objective);
	for (int i = 0; i < vars; ++i)
		s.bounds(i, 0, 1);
	for (int p = 0; p < m; ++p) {
		ip::Vec row(vars);
		for (int i = 0; i < n; ++i)
			if (a[i].mask >> p & 1)
				row[i] = 1;
		s.add_ge(row, 1);
	}
	ip::Vec count_row(vars);
	fill(count_row.begin(), count_row.begin() + n, 1);
	if (cardinality >= 0)
		s.add_eq(count_row, cardinality);
	else
		s.add_ge(count_row, minimum);
	for (int i = 0; i < n; ++i)
		if (a[i].monitors > baseline) {
			int j = int(lower_bound(levels.begin(), levels.end(), a[i].monitors) - levels.begin());
			ip::Vec row(vars);
			row[i] = 1;
			row[n + j] = -1;
			s.add_le(row, 0); // Choosing friend i activates its monitor threshold.
		}
	for (int j = 1; j < k; ++j) {
		ip::Vec row(vars);
		row[n + j] = 1;
		row[n + j - 1] = -1;
		s.add_le(row, 0); // Higher threshold includes all preceding increments.
	}
	auto r = s.maximize(options);
	// Recompute from chosen friends, so no large integer passes through double.
	ll cost = 0, monitors = 0;
	for (int i = 0; i < n; ++i)
		if (r.x[i] > 0.5) {
			cost += a[i].cost;
			monitors = max(monitors, a[i].monitors);
		}
	return cost + monitors * b;
}

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);
	int n, m;
	ll b;
	cin >> n >> m >> b;
	vector<Friend> a(n);
	for (auto& x : a) {
		int count;
		cin >> x.cost >> x.monitors >> count;
		x.mask = 0;
		while (count--) {
			int p;
			cin >> p;
			x.mask |= 1 << (p - 1);
		}
	}
	cout << cunning_gena(m, b, std::move(a)) << '\n';
	return 0;
}
