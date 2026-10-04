#include "ip_solver.hpp"
#include <iostream>

// Input: n capacity, then n lines of weight profit (0/1 knapsack).
int main() {
	int n;
	double capacity;
	if (!(std::cin >> n >> capacity) || n < 0) return 0;
	ip::Vec weight(n), profit(n);
	for (int i=0; i<n; ++i) std::cin >> weight[i] >> profit[i];
	ip::Solver s(profit);
	s.add_le(weight, capacity);
	for (int i=0; i<n; ++i) s.bounds(i, 0, 1);
	ip::Options o;
	o.time_limit=1;
	auto r=s.maximize(o);
	if (r.has_solution()) std::cout << r.objective << '\n';
	if (r.status!=ip::Status::Optimal)
		std::cerr << "Stopped before proving optimality; upper bound = " << r.bound << '\n';
}
