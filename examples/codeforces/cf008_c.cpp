// https://codeforces.com/problemset/problem/8/C
#include "ip_solver.hpp"
#include <iostream>

int main() {
	std::ios::sync_with_stdio(false);
	std::cin.tie(nullptr);
	int sx, sy, n;
	std::cin >> sx >> sy >> n;
	std::vector<int> x(n), y(n), d(n), used(n);
	long long answer = 0;
	for (int i=0; i<n; ++i) {
		std::cin >> x[i] >> y[i];
		x[i] -= sx; y[i] -= sy;
		d[i] = x[i]*x[i] + y[i]*y[i];
		answer += 2*d[i]; // Initially collect every object in its own trip.
	}
	std::vector<std::pair<int,int>> edge;
	ip::Vec gain;
	for (int i=0; i<n; ++i) for (int j=i+1; j<n; ++j) {
		int saving = 2*(x[i]*x[j] + y[i]*y[j]);
		if (saving>0) { edge.push_back({i,j}); gain.push_back(saving); }
	}
	ip::Solver solver(gain);
	for (int i=0; i<n; ++i) {
		ip::Vec row(edge.size());
		for (int e=0; e<int(edge.size()); ++e)
			if (edge[e].first==i || edge[e].second==i) row[e]=1;
		solver.add_le(row, 1); // Nonnegative integers + degree <= 1 imply binary.
	}
#ifdef IPSOLVER_STATS
	auto started = std::chrono::steady_clock::now();
#endif
	auto result = solver.maximize();
	if (result.status != ip::Status::Optimal) return 1;
	std::vector<int> route{0};
	for (int e=0; e<int(edge.size()); ++e) if (result.x[e]>0.5) {
		auto [i,j] = edge[e];
		used[i] = used[j] = 1;
		answer -= std::llround(gain[e]);
		route.insert(route.end(), {i+1,j+1,0});
	}
	for (int i=0; i<n; ++i) if (!used[i]) route.insert(route.end(), {i+1,0});
	std::cout << answer << '\n';
	for (int v:route) std::cout << v << ' ';
	std::cout << '\n';
#ifdef IPSOLVER_STATS
	std::cerr << result.nodes << ' ' << result.pivots << ' '
			  << std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count() << '\n';
#endif
}
