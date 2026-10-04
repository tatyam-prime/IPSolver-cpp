#include "ip_solver.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using Vec = std::vector<double>;
using Clock = std::chrono::steady_clock;
constexpr double INF = std::numeric_limits<double>::infinity();

// Small independent dynamic programs establish every reported optimum. Their
// running time and model construction are excluded from the solver timings.
struct Model {
	std::string name;
	Vec c, b, upper;
	std::vector<Vec> a;
	std::vector<bool> integer;
	double optimum = 0;
};

struct RNG {
	uint64_t s;
	int operator()(int n) {
		s ^= s << 13;
		s ^= s >> 7;
		s ^= s << 17;
		return int(s % uint64_t(n));
	}
};

Model empty_model(std::string name, int n, double upper = 1) {
	return {std::move(name), Vec(n), {}, Vec(n, upper), {},
			std::vector<bool>(n, true), 0};
}

Model knapsack(int n, uint64_t seed, int multiplicity = 1, bool filler = false) {
	Model m = empty_model((filler ? "mixed_filler_" : multiplicity == 1 ?
		"knapsack_" : "bounded_knapsack_") + std::to_string(n),
		n + int(filler), multiplicity);
	RNG rng{seed};
	Vec w(n + int(filler));
	int total = 0;
	for (int i = 0; i < n; ++i) {
		w[i] = 1 + rng(24);
		m.c[i] = 1 + rng(45);
		total += int(w[i]) * multiplicity;
	}
	int capacity = total * 2 / 5;
	std::vector<int> dp(capacity + 1, -1000000000);
	dp[0] = 0;
	for (int i = 0; i < n; ++i)
		for (int copies = 0; copies < multiplicity; ++copies)
			for (int j = capacity; j >= int(w[i]); --j)
				dp[j] = std::max(dp[j], dp[j - int(w[i])] + int(m.c[i]) -
					(filler ? int(w[i]) : 0));
	m.optimum = *std::max_element(dp.begin(), dp.end());
	m.b.push_back(capacity + (filler ? 0.5 : 0));
	if (filler) {
		w[n] = m.c[n] = 1;
		m.integer[n] = false;
		m.upper[n] = capacity + 0.5;
		m.optimum += capacity + 0.5;
	}
	m.a.push_back(std::move(w));
	return m;
}

Model two_knapsack(int n, uint64_t seed) {
	Model m = empty_model("two_knapsack_" + std::to_string(n), n);
	RNG rng{seed};
	Vec w(n), v(n);
	int sw = 0, sv = 0;
	for (int i = 0; i < n; ++i) {
		w[i] = 1 + rng(12);
		v[i] = 1 + rng(12);
		m.c[i] = 1 + rng(35);
		sw += int(w[i]);
		sv += int(v[i]);
	}
	int W = sw / 3, V = sv / 3;
	std::vector<int> dp((W + 1) * (V + 1));
	for (int i = 0; i < n; ++i)
		for (int j = W; j >= int(w[i]); --j)
			for (int k = V; k >= int(v[i]); --k) {
				auto &cell = dp[j * (V + 1) + k];
				cell = std::max(cell, dp[(j - int(w[i])) * (V + 1) +
					k - int(v[i])] + int(m.c[i]));
			}
	m.optimum = dp[W * (V + 1) + V];
	m.a = {std::move(w), std::move(v)};
	m.b = {double(W), double(V)};
	return m;
}

Model correlated_knapsack(int n, uint64_t seed) {
	Model m = empty_model("correlated_knapsack_" + std::to_string(n), n);
	RNG rng{seed};
	Vec w(n);
	int total = 0;
	for (int i = 0; i < n; ++i) {
		w[i] = 100 + rng(900);
		m.c[i] = w[i] + 10;
		total += int(w[i]);
	}
	int capacity = total * 2 / 5;
	std::vector<int> dp(capacity + 1);
	for (int i = 0; i < n; ++i)
		for (int j = capacity; j >= int(w[i]); --j)
			dp[j] = std::max(dp[j], dp[j - int(w[i])] + int(m.c[i]));
	m.optimum = dp.back();
	m.a = {std::move(w)};
	m.b = {double(capacity)};
	return m;
}

Model independent_set(int n, uint64_t seed) {
	Model m = empty_model("independent_set_" + std::to_string(n), n);
	RNG rng{seed};
	std::vector<uint64_t> adjacency(n);
	for (int i = 0; i < n; ++i) {
		m.c[i] = 1 + rng(30);
		for (int j = 0; j < i; ++j)
			if (rng(100) < 22) {
				adjacency[i] |= uint64_t(1) << j;
				adjacency[j] |= uint64_t(1) << i;
				Vec row(n);
				row[i] = row[j] = 1;
				m.a.push_back(std::move(row));
				m.b.push_back(1);
			}
	}
	// Meet in the middle: best right-half independent set contained in each
	// mask, then enumerate left-half independent sets and their allowed masks.
	int left = n / 2, right = n - left;
	unsigned all_right = (1u << right) - 1;
	std::vector<int> best(1u << right);
	for (unsigned mask = 1; mask <= all_right; ++mask) {
		int v = __builtin_ctz(mask);
		unsigned rest = mask & (mask - 1);
		unsigned allowed = rest & ~unsigned(adjacency[left + v] >> left);
		best[mask] = std::max(best[rest], int(m.c[left + v]) + best[allowed]);
	}
	std::vector<int> weight(1u << left, -1);
	std::vector<unsigned> forbidden(1u << left);
	weight[0] = 0;
	m.optimum = best.back();
	for (unsigned mask = 1; mask < weight.size(); ++mask) {
		int v = __builtin_ctz(mask);
		unsigned rest = mask & (mask - 1);
		if (weight[rest] < 0 || (adjacency[v] & rest)) continue;
		weight[mask] = weight[rest] + int(m.c[v]);
		forbidden[mask] = forbidden[rest] | unsigned(adjacency[v] >> left);
		m.optimum = std::max(m.optimum,
			double(weight[mask] + best[all_right & ~forbidden[mask]]));
	}
	return m;
}

Model set_packing(int n, int resources, uint64_t seed) {
	Model m = empty_model("set_packing_" + std::to_string(n), n);
	RNG rng{seed};
	m.a.assign(resources, Vec(n));
	m.b.assign(resources, 1);
	std::vector<unsigned> masks(n);
	for (int i = 0; i < n; ++i) {
		m.c[i] = 1 + rng(35);
		int count = 2 + rng(3);
		while (__builtin_popcount(masks[i]) < count)
			masks[i] |= 1u << rng(resources);
		for (int j = 0; j < resources; ++j)
			m.a[j][i] = bool(masks[i] & (1u << j));
	}
	std::vector<int> dp(1u << resources, -1000000000);
	dp[0] = 0;
	for (int i = 0; i < n; ++i)
		for (unsigned mask = 0; mask < dp.size(); ++mask)
			if (!(mask & masks[i]) && dp[mask] >= 0)
				dp[mask | masks[i]] = std::max(dp[mask | masks[i]],
					dp[mask] + int(m.c[i]));
	m.optimum = *std::max_element(dp.begin(), dp.end());
	return m;
}

Model assignment(int n, uint64_t seed) {
	Model m = empty_model("assignment_" + std::to_string(n), n * n);
	RNG rng{seed};
	m.a.assign(2 * n, Vec(n * n));
	m.b.assign(2 * n, 1);
	for (int i = 0; i < n; ++i)
		for (int j = 0; j < n; ++j) {
			m.c[i * n + j] = 1 + rng(100);
			m.a[i][i * n + j] = m.a[n + j][i * n + j] = 1;
		}
	std::vector<int> dp(1u << n, -1000000000);
	dp[0] = 0;
	for (unsigned mask = 0; mask + 1 < dp.size(); ++mask) {
		int row = __builtin_popcount(mask);
		for (int j = 0; j < n; ++j)
			if (!(mask & (1u << j)))
				dp[mask | (1u << j)] = std::max(dp[mask | (1u << j)],
					dp[mask] + int(m.c[row * n + j]));
	}
	m.optimum = dp.back();
	return m;
}

ip::Solver make_solver(const Model &m, const Vec &lo, const Vec &hi,
					   bool relaxation = false) {
	ip::Solver solver(m.c);
	for (int i = 0; i < int(m.c.size()); ++i) {
		solver.bounds(i, lo[i], hi[i]);
		if (relaxation || !m.integer[i]) solver.continuous(i);
	}
	for (int i = 0; i < int(m.a.size()); ++i)
		solver.add_le(m.a[i], m.b[i]);
	return solver;
}

const char *status_name(ip::Status status) {
	switch (status) {
		case ip::Status::Optimal: return "optimal";
		case ip::Status::Infeasible: return "infeasible";
		case ip::Status::UnboundedRelaxation: return "unbounded_relaxation";
		case ip::Status::Limit: return "limit";
		case ip::Status::NumericalError: return "numerical_error";
	}
	return "unknown";
}

// This intentionally simple reference branch-and-bound rebuilds and solves the
// entire LP at each node. It also uses different branching heuristics, so its
// timing difference must not be attributed solely to warm starts.
ip::Result cold_solve(const Model &m, ip::Options options) {
	struct Node { Vec lo, hi; double bound; };
	std::vector<Node> stack{{Vec(m.c.size()), m.upper, INF}};
	ip::Result result;
	result.status = ip::Status::Optimal;
	result.objective = -INF;
	auto start = Clock::now();
	while (!stack.empty()) {
		double elapsed = std::chrono::duration<double>(Clock::now() - start).count();
		if (result.nodes >= options.node_limit || result.pivots >= options.pivot_limit ||
			elapsed >= options.time_limit) {
			result.status = ip::Status::Limit;
			break;
		}
		Node node = std::move(stack.back());
		stack.pop_back();
		if (!result.x.empty() && node.bound <= result.objective + options.eps) continue;
		auto solver = make_solver(m, node.lo, node.hi, true);
		auto lp_options = options;
		lp_options.time_limit -= elapsed;
		lp_options.node_limit = 1;
		lp_options.pivot_limit -= result.pivots;
		auto lp = solver.maximize(lp_options);
		++result.nodes;
		result.pivots += lp.pivots;
		if (lp.status == ip::Status::Infeasible) continue;
		if (lp.status != ip::Status::Optimal) {
			stack.push_back(std::move(node));
			result.status = lp.status;
			break;
		}
		if (!result.x.empty() && lp.objective <= result.objective + options.eps) continue;
		int branch = -1;
		double fraction = options.integer_eps;
		for (int i = 0; i < int(lp.x.size()); ++i) {
			double f = std::abs(lp.x[i] - std::round(lp.x[i]));
			if (m.integer[i] && f > fraction) branch = i, fraction = f;
		}
		if (branch == -1) {
			result.objective = lp.objective;
			result.x = std::move(lp.x);
			continue;
		}
		Node down = node, up = std::move(node);
		down.hi[branch] = std::floor(lp.x[branch]);
		up.lo[branch] = std::ceil(lp.x[branch]);
		down.bound = up.bound = lp.objective;
		if (down.lo[branch] <= down.hi[branch]) stack.push_back(std::move(down));
		if (up.lo[branch] <= up.hi[branch]) stack.push_back(std::move(up));
	}
	if (stack.empty()) {
		result.status = result.x.empty() ? ip::Status::Infeasible : ip::Status::Optimal;
		result.bound = result.objective;
	} else {
		result.bound = result.objective;
		for (const auto &node : stack) result.bound = std::max(result.bound, node.bound);
	}
	return result;
}

bool validate(const Model &m, const ip::Result &r) {
	constexpr double eps = 1e-5;
	if (r.status == ip::Status::Optimal &&
		(!r.has_solution() || std::abs(r.objective - m.optimum) > eps)) return false;
	if (r.status == ip::Status::Infeasible ||
		r.status == ip::Status::UnboundedRelaxation ||
		r.status == ip::Status::NumericalError) return false;
	if (r.bound + eps < m.optimum) return false;
	if (!r.has_solution()) return true;
	if (r.x.size() != m.c.size() || r.objective > m.optimum + eps) return false;
	double objective = 0;
	for (int i = 0; i < int(r.x.size()); ++i) {
		if (!std::isfinite(r.x[i]) || r.x[i] < -eps ||
			r.x[i] > m.upper[i] + eps ||
			(m.integer[i] && std::abs(r.x[i] - std::round(r.x[i])) > eps)) return false;
		objective += r.x[i] * m.c[i];
	}
	if (std::abs(objective - r.objective) > eps) return false;
	for (int i = 0; i < int(m.a.size()); ++i) {
		double lhs = 0;
		for (int j = 0; j < int(r.x.size()); ++j) lhs += m.a[i][j] * r.x[j];
		if (lhs > m.b[i] + eps) return false;
	}
	return true;
}

int main(int argc, char **argv) {
	bool cold = false;
	std::string filter;
	ip::Options options;
	options.time_limit = 2;
	options.node_limit = 50000;
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--cold") cold = true;
		else if (arg == "--seconds" && i + 1 < argc) options.time_limit = std::stod(argv[++i]);
		else if (arg == "--nodes" && i + 1 < argc) options.node_limit = std::stoull(argv[++i]);
		else if (arg == "--cuts" && i + 1 < argc) options.cuts = std::stoi(argv[++i]);
		else if (arg == "--strong" && i + 1 < argc) options.strong_branching = std::stoi(argv[++i]);
		else if (arg == "--case" && i + 1 < argc) filter = argv[++i];
		else {
			std::cerr << "Usage: bench [--cold] [--seconds N] [--nodes N] [--cuts N] [--strong N] [--case TEXT]\n";
			return 2;
		}
	}
	std::vector<Model> models;
	for (int n : {30, 60, 100}) models.push_back(knapsack(n, 1701 + n));
	for (int n : {18, 30}) models.push_back(knapsack(n, 2801 + n, 5));
	for (int n : {28, 48}) models.push_back(two_knapsack(n, 3901 + n));
	for (int n : {50, 80}) models.push_back(correlated_knapsack(n, 7201 + n));
	for (int n : {24, 36}) models.push_back(set_packing(n, 18, 4101 + n));
	for (int n : {28, 36}) models.push_back(independent_set(n, 8301 + n));
	for (int n : {6, 9, 12}) models.push_back(assignment(n, 5201 + n));
	for (int n : {50, 80}) models.push_back(knapsack(n, 6301 + n, 1, true));

	std::cout << "case,method,cuts,strong,variables,constraints,known_optimum,status,objective,bound,nodes,pivots,ms,verified\n";
	std::cout << std::setprecision(12);
	bool success = true;
	for (const auto &m : models) {
		if (m.name.find(filter) == std::string::npos) continue;
		auto solver = make_solver(m, Vec(m.c.size()), m.upper);
		for (int method = 0; method < 1 + int(cold); ++method) {
			auto start = Clock::now();
			auto r = method ? cold_solve(m, options) : solver.maximize(options);
			double ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
			bool verified = validate(m, r);
			success &= verified;
			std::cout << m.name << ',' << (method ? "cold" : "solver") << ','
				<< (method ? 0 : options.cuts) << ','
				<< (method ? 0 : options.strong_branching) << ','
				<< m.c.size() << ',' << m.a.size() << ',' << m.optimum << ','
				<< status_name(r.status) << ',' << r.objective << ',' << r.bound << ','
				<< r.nodes << ',' << r.pivots << ',' << ms << ',' << verified << '\n';
		}
	}
	return success ? 0 : 1;
}
