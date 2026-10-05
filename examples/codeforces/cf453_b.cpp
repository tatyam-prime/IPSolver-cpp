// https://codeforces.com/problemset/problem/453/B
#include "ip_solver.hpp"
#include <array>
#include <iostream>
#include <map>

using namespace std;

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	int n;
	cin >> n;
	vector<int> a(n);
	for (int& x : a)
		cin >> x;
	ip::Options options;
	constexpr int primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59};
	struct Choice {
		int group, value, mask, gain;
	};
	vector<vector<int>> positions(31);
	vector<int> original(31);
	for (int i = 0; i < int(a.size()); ++i) {
		int group = a[i];
		positions[group].push_back(i);
		original[group] = a[i];
	}
	vector<Choice> choices;
	for (int g = 0; g < int(positions.size()); ++g)
		if (!positions[g].empty()) {
			map<int, Choice> best;
			for (int v = 2; v < 60; ++v) {
				int gain = original[g] - 1 - abs(original[g] - v), mask = 0;
				if (gain <= 0)
					continue; // Setting this position to 1 costs no more.
				for (int p = 0; p < 17; ++p)
					if (v % primes[p] == 0)
						mask |= 1 << p;
				Choice x{g, v, mask, gain};
				if (!best.count(mask) || best[mask].gain < gain)
					best[mask] = x;
			}
			for (auto entry : best)
				choices.push_back(entry.second);
		}
	int vars = int(choices.size());
	ip::Vec objective(vars);
	for (int j = 0; j < vars; ++j)
		objective[j] = choices[j].gain;
	ip::Solver s(objective);
	// Every choice uses a prime, so these nonnegative integer variables <=1.
	for (int p = 0; p < 17; ++p) {
		ip::Vec row(vars);
		bool used = false;
		for (int j = 0; j < vars; ++j)
			if (choices[j].mask >> p & 1)
				row[j] = 1, used = true;
		if (used) {
			s.add_le(row, 1);
		}
	}
	for (int g = 0; g < int(positions.size()); ++g)
		if (!positions[g].empty()) {
			ip::Vec row(vars);
			bool used = false;
			for (int j = 0; j < vars; ++j)
				if (choices[j].group == g)
					row[j] = 1, used = true;
			if (used) {
				s.add_le(row, int(positions[g].size()));
			}
		}
	if (vars) {
		int best_gain = -1;
		vector<int> order(vars);
		iota(order.begin(), order.end(), 0);
		for (int ratio = 0; ratio < 2; ++ratio) {
			sort(order.begin(), order.end(), [&](int i, int j) {
				int pi = ratio ? __builtin_popcount(unsigned(choices[i].mask)) : 1;
				int pj = ratio ? __builtin_popcount(unsigned(choices[j].mask)) : 1;
				return choices[i].gain * pj > choices[j].gain * pi;
			});
			ip::Vec x(vars);
			vector<int> count(positions.size());
			int used = 0, gain = 0;
			for (int j : order) {
				auto c = choices[j];
				if (!(used & c.mask) && count[c.group] < int(positions[c.group].size())) {
					x[j] = 1;
					used |= c.mask;
					++count[c.group];
					gain += c.gain;
				}
			}
			if (gain > best_gain)
				best_gain = gain, options.initial_solution = std::move(x);
		}
	}
	auto r = s.maximize(options);
	vector<int> b(a.size(), 1), next(positions.size());
	for (int j = 0; j < vars; ++j)
		if (r.x[j] > 0.5) {
			auto c = choices[j];
			b[positions[c.group][next[c.group]++]] = c.value;
		}
	for (int x : b)
		cout << x << ' ';
	cout << '\n';
}
