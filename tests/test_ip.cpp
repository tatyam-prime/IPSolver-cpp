#include "ip_solver.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr double tol = 2e-6;

enum class Relation { le, ge, eq };

struct Row {
	std::vector<double> a;
	Relation relation;
	double b;
};

struct Problem {
	std::vector<double> c, lo, hi;
	std::vector<bool> integer;
	std::vector<Row> rows;
};

struct OracleResult {
	bool feasible = false;
	double objective = -std::numeric_limits<double>::infinity();
	std::vector<double> x;
};

std::string describe(const Problem& p) {
	std::ostringstream s;
	s.precision(17);
	s << "maximize";
	for (double c : p.c) s << ' ' << c;
	s << "\nbounds";
	for (std::size_t j = 0; j < p.c.size(); ++j)
		s << " [" << p.lo[j] << ',' << p.hi[j] << ']'
		  << (p.integer[j] ? " integer" : " continuous");
	for (const Row& r : p.rows) {
		s << '\n';
		for (double a : r.a) s << a << ' ';
		s << (r.relation == Relation::le ? "<= " :
			  r.relation == Relation::ge ? ">= " : "= ") << r.b;
	}
	return s.str();
}

void require(bool value, const std::string& message) {
	if (!value) throw std::runtime_error(message);
}

bool near(double a, double b) {
	return std::isfinite(a) && std::isfinite(b) &&
		   std::abs(a - b) <= tol * (1 + std::max(std::abs(a), std::abs(b)));
}

double dot(const std::vector<double>& a, const std::vector<double>& x) {
	double value = 0;
	for (std::size_t j = 0; j < a.size(); ++j) value += a[j] * x[j];
	return value;
}

bool feasible(const Problem& p, const std::vector<double>& x) {
	if (x.size() != p.c.size()) return false;
	for (std::size_t j = 0; j < x.size(); ++j) {
		if (!std::isfinite(x[j]) || x[j] < p.lo[j] - tol ||
			x[j] > p.hi[j] + tol ||
			(p.integer[j] && std::abs(x[j] - std::round(x[j])) > tol)) return false;
	}
	for (const Row& r : p.rows) {
		double v = dot(r.a, x);
		if ((r.relation != Relation::ge && v > r.b + tol) ||
			(r.relation != Relation::le && v < r.b - tol)) return false;
	}
	return true;
}

// Independent exhaustive oracle. At most one variable may be continuous;
// after fixing all integer variables, every constraint gives an interval for it.
OracleResult enumerate(const Problem& p) {
	OracleResult best;
	int continuous = -1;
	for (std::size_t j = 0; j < p.c.size(); ++j) {
		require(std::isfinite(p.lo[j]) && std::isfinite(p.hi[j]),
				"oracle requires finite bounds");
		if (!p.integer[j]) {
			require(continuous == -1, "oracle supports one continuous variable");
			continuous = static_cast<int>(j);
		}
	}
	std::vector<double> x(p.c.size());
	auto visit = [&](auto&& self, std::size_t j) -> void {
		if (j != x.size()) {
			if (static_cast<int>(j) == continuous) {
				x[j] = 0;
				self(self, j + 1);
			} else {
				for (int v = static_cast<int>(std::ceil(p.lo[j]));
					 v <= static_cast<int>(std::floor(p.hi[j])); ++v) {
					x[j] = v;
					self(self, j + 1);
				}
			}
			return;
		}
		if (continuous != -1) {
			long double lo = p.lo[continuous], hi = p.hi[continuous];
			auto impose_le = [&](long double coefficient, long double rhs) {
				if (coefficient > 0) hi = std::min(hi, rhs / coefficient);
				else if (coefficient < 0) lo = std::max(lo, rhs / coefficient);
				else if (rhs < -1e-12L) lo = hi + 1;
			};
			for (const Row& r : p.rows) {
				long double rhs = r.b;
				for (std::size_t k = 0; k < x.size(); ++k)
					if (static_cast<int>(k) != continuous)
						rhs -= static_cast<long double>(r.a[k]) * x[k];
				if (r.relation != Relation::ge) impose_le(r.a[continuous], rhs);
				if (r.relation != Relation::le) impose_le(-r.a[continuous], -rhs);
			}
			if (lo > hi + 1e-12L) return;
			x[continuous] = static_cast<double>(p.c[continuous] >= 0 ? hi : lo);
		}
		if (!feasible(p, x)) return;
		double objective = dot(p.c, x);
		if (!best.feasible || objective > best.objective) {
			best.feasible = true;
			best.objective = objective;
			best.x = x;
		}
	};
	visit(visit, 0);
	return best;
}

// A separate LP oracle for bounded models: enumerate intersections of n active
// hyperplanes and solve them with long-double Gaussian elimination.
OracleResult vertices(const Problem& p) {
	const int n = static_cast<int>(p.c.size());
	std::vector<Row> planes = p.rows;
	for (int j = 0; j < n; ++j) {
		require(!p.integer[j], "vertex oracle requires continuous variables");
		std::vector<double> unit(n);
		unit[j] = 1;
		planes.push_back({unit, Relation::eq, p.lo[j]});
		planes.push_back({unit, Relation::eq, p.hi[j]});
	}
	OracleResult best;
	std::vector<int> selected;
	auto visit = [&](auto&& self, int start) -> void {
		if (static_cast<int>(selected.size()) < n) {
			for (int i = start; i < static_cast<int>(planes.size()); ++i) {
				selected.push_back(i);
				self(self, i + 1);
				selected.pop_back();
			}
			return;
		}
		std::vector<std::vector<long double>> matrix(n, std::vector<long double>(n + 1));
		for (int i = 0; i < n; ++i) {
			const Row& row = planes[selected[i]];
			for (int j = 0; j < n; ++j) matrix[i][j] = row.a[j];
			matrix[i][n] = row.b;
		}
		for (int j = 0; j < n; ++j) {
			int pivot = j;
			for (int i = j + 1; i < n; ++i)
				if (std::abs(matrix[i][j]) > std::abs(matrix[pivot][j])) pivot = i;
			if (std::abs(matrix[pivot][j]) < 1e-14L) return;
			std::swap(matrix[j], matrix[pivot]);
			long double divisor = matrix[j][j];
			for (int k = j; k <= n; ++k) matrix[j][k] /= divisor;
			for (int i = 0; i < n; ++i) if (i != j) {
				long double multiplier = matrix[i][j];
				for (int k = j; k <= n; ++k) matrix[i][k] -= multiplier * matrix[j][k];
			}
		}
		std::vector<double> x(n);
		for (int j = 0; j < n; ++j) x[j] = static_cast<double>(matrix[j][n]);
		if (!feasible(p, x)) return;
		double objective = dot(p.c, x);
		if (!best.feasible || objective > best.objective) {
			best.feasible = true;
			best.objective = objective;
			best.x = std::move(x);
		}
	};
	visit(visit, 0);
	return best;
}

Problem make_problem(std::vector<double> c, std::vector<double> hi) {
	Problem p;
	p.c = std::move(c);
	p.lo.assign(p.c.size(), 0);
	p.hi = std::move(hi);
	p.integer.assign(p.c.size(), true);
	return p;
}

ip::Solver build(const Problem& p) {
	ip::Solver solver(p.c);
	for (std::size_t j = 0; j < p.c.size(); ++j) {
		solver.bounds(static_cast<int>(j), p.lo[j], p.hi[j]);
		if (!p.integer[j]) solver.continuous(static_cast<int>(j));
	}
	for (const Row& r : p.rows) {
		if (r.relation == Relation::le) solver.add_le(r.a, r.b);
		else if (r.relation == Relation::ge) solver.add_ge(r.a, r.b);
		else solver.add_eq(r.a, r.b);
	}
	return solver;
}

std::size_t checks = 0;

void check(const Problem& p, const std::string& name,
		   const ip::Options& options = ip::Options(), bool minimize = false) {
	int continuous = static_cast<int>(std::count(p.integer.begin(), p.integer.end(), false));
	Problem target = p;
	if (minimize) for (double& c : target.c) c = -c;
	OracleResult expected = continuous <= 1 ? enumerate(target) : vertices(target);
	if (minimize && expected.feasible) expected.objective = -expected.objective;
	const auto solver = build(p);
	ip::Result actual = minimize ? solver.minimize(options) : solver.maximize(options);
	std::ostringstream context;
	context.precision(17);
	context << "\nactual status=" << static_cast<int>(actual.status)
			<< " objective=" << actual.objective << " bound=" << actual.bound
			<< " nodes=" << actual.nodes << " pivots=" << actual.pivots << " x=";
	for (double x : actual.x) context << x << ' ';
	context << "\nexpected objective=" << expected.objective << " x=";
	for (double x : expected.x) context << x << ' ';
	auto assert_that = [&](bool value, const std::string& message) {
		require(value, name + ": " + message + "\n" + describe(p) + context.str());
	};
	if (!expected.feasible) {
		assert_that(actual.status == ip::Status::Infeasible, "expected infeasible");
		assert_that(!actual.has_solution(), "infeasible result has a solution");
		assert_that(actual.objective == (minimize ? ip::INF : -ip::INF) &&
					actual.bound == actual.objective, "incorrect infeasible objective/bound");
	} else {
		assert_that(actual.status == ip::Status::Optimal, "expected optimal");
		assert_that(actual.has_solution(), "optimal result has no solution");
		assert_that(feasible(p, actual.x), "returned vector violates the model");
		assert_that(near(actual.objective, dot(p.c, actual.x)),
					"objective disagrees with the returned vector");
		bool exact_objective = true;
		double magnitude = 0;
		for (std::size_t j = 0; j < p.c.size(); ++j) {
			exact_objective = exact_objective &&
				p.c[j] == std::round(p.c[j]) && (p.integer[j] || p.c[j] == 0);
			magnitude += std::abs(p.c[j]) * std::max(std::abs(p.lo[j]), std::abs(p.hi[j]));
		}
		exact_objective = exact_objective && magnitude < 4503599627370496.0;
		assert_that(exact_objective ? actual.objective == expected.objective :
									near(actual.objective, expected.objective),
					"incorrect optimum: got " + std::to_string(actual.objective) +
					", expected " + std::to_string(expected.objective));
		assert_that(near(actual.bound, expected.objective), "incorrect optimal bound");
	}
	++checks;
}

void deterministic_tests() {
	check(make_problem({3, 2}, {4, 5}), "only variable bounds");
	check(make_problem({-3, -2}, {4, 5}), "negative objective");
	check(make_problem({0, 0}, {4, 5}), "zero objective");

	Problem p = make_problem({3, 2}, {5, 5});
	p.rows = {{{2, 1}, Relation::le, 4}, {{1, 2}, Relation::le, 4}};
	check(p, "fractional LP vertex");
	p.rows.push_back({{2, 1}, Relation::le, 4});
	p.rows.push_back({{0, 0}, Relation::eq, 0});
	p.rows.push_back({{1, 0}, Relation::le, 100});
	check(p, "duplicate and redundant constraints");

	p = make_problem({-1}, {8});
	p.rows = {{{-1}, Relation::le, -3}};
	check(p, "negative RHS phase one");
	p.rows = {{{1}, Relation::ge, 3}};
	check(p, "greater than constraint");
	p.rows = {{{1}, Relation::le, -1}};
	check(p, "inconsistent with nonnegative default");
	p.rows = {{{2}, Relation::eq, 1}};
	check(p, "LP feasible but integer infeasible");
	p.rows = {{{0}, Relation::le, -1}};
	check(p, "infeasible zero row");
	p.rows = {{{1}, Relation::ge, 5}, {{1}, Relation::le, 4}};
	check(p, "infeasible LP relaxation");

	p = make_problem({-4, 2}, {3, 4});
	p.lo = {-3, -2};
	p.rows = {{{1, -1}, Relation::eq, -3}};
	check(p, "negative bounds and equality");
	p.lo = {-3, 4};
	p.hi = {-3, 4};
	p.rows.clear();
	check(p, "fixed variables");

	p = make_problem({1}, {0.8});
	p.lo = {0.2};
	check(p, "fractional bounds contain no integer");
	p.lo = {-1.8};
	p.hi = {-0.2};
	check(p, "negative fractional bounds contain an integer");
	p.lo = {-2.4};
	p.hi = {3.6};
	check(p, "integer tightening of fractional bounds");
	p.lo = {2};
	p.hi = {1};
	check(p, "contradictory variable bounds");

	p = make_problem({1, 1}, {3, 3});
	p.integer[1] = false;
	p.rows = {{{2, 2}, Relation::le, 3}};
	check(p, "mixed integer fractional optimum");
	p = make_problem({1, -1}, {3, 3});
	p.integer[1] = false;
	p.lo[1] = -3;
	p.rows = {{{2, -1}, Relation::le, 2.5}, {{1, 1}, Relation::ge, -1.5}};
	check(p, "mixed integer negative continuous variable");
	p.rows = {{{1, 2}, Relation::eq, 0.5}};
	check(p, "mixed integer equality");
	p = make_problem({1, 0}, {1, 1.5});
	p.integer[1] = false;
	p.rows = {{{1, 1}, Relation::eq, 1.5}};
	check(p, "mixed integer equality requires continuous repair");

	p = make_problem({2.25, -1.5}, {4, 4});
	p.lo = {-2, -2};
	p.rows = {{{-2.5, 3.25}, Relation::le, -0.75},
			  {{1.5, 0.5}, Relation::ge, 1.25}};
	check(p, "fractional coefficients");

	// Nonempty integer coordinates in a zero-dimensional problem are impossible.
	check(make_problem({}, {}), "zero variables feasible");
	p = make_problem({}, {});
	p.rows.push_back({{}, Relation::le, -1});
	check(p, "zero variables infeasible");

	ip::Solver default_bounds({-1, -2});
	ip::Result r = default_bounds.maximize();
	require(r.status == ip::Status::Optimal && near(r.objective, 0) &&
			r.x.size() == 2 && near(r.x[0], 0) && near(r.x[1], 0),
			"variables must default to nonnegative integers");
	++checks;

	ip::Solver continuous({1, 1});
	continuous.continuous(0);
	continuous.continuous(1);
	continuous.add_le({2, 1}, 4);
	continuous.add_le({1, 2}, 4);
	r = continuous.maximize();
	require(r.status == ip::Status::Optimal && near(r.objective, 8.0 / 3) &&
			r.x.size() == 2 && near(r.x[0], 4.0 / 3) && near(r.x[1], 4.0 / 3),
			"all-continuous LP optimum");
	++checks;

	ip::Solver cycling({10, -57, -9, -24});
	for (int j = 0; j < 4; ++j) cycling.continuous(j);
	cycling.add_le({0.5, -5.5, -2.5, 9}, 0);
	cycling.add_le({0.5, -1.5, -0.5, 1}, 0);
	cycling.add_le({1, 0, 0, 0}, 1);
	r = cycling.maximize();
	require(r.status == ip::Status::Optimal && near(r.objective, 1),
			"degenerate cycling LP");
	++checks;

	for (bool use_continuous : {false, true}) {
		ip::Solver ill_scaled({0, 1});
		if (use_continuous) {
			ill_scaled.continuous(0);
			ill_scaled.continuous(1);
		}
		ill_scaled.add_le({1e9, 1}, 1);
		r = ill_scaled.maximize();
		require(r.status == ip::Status::Optimal && near(r.objective, 1) &&
				r.x.size() == 2 && 1e9 * r.x[0] + r.x[1] <= 1 + tol,
				"row scaling must preserve a small essential coefficient");
		++checks;
	}
	ip::Solver large_bound({1, 0});
	large_bound.bounds(0, 0, 1e9);
	large_bound.continuous(1);
	large_bound.bounds(1, 0, 1);
	large_bound.add_le({1, 1}, 1e9 - 0.25);
	r = large_bound.maximize();
	require(r.status == ip::Status::Optimal && r.objective == 999999999 &&
			r.x.size() == 2 && r.x[0] == 999999999 &&
			r.x[0] + r.x[1] <= 1e9 - 0.25 + tol,
			"large RHS must not allow infeasible integer rounding");
	++checks;

	p = make_problem({4, 1000000000011.0, 1000000000011.0,
					  2000000000000.0, 2000000000012.0, 1}, {1, 1, 1, 1, 1, 1});
	p.rows = {{{1, 1, 13, 5, 4, 5}, Relation::le, 9},
			  {{2, 7, 9, 8, 5, 3}, Relation::le, 11},
			  {{12, 8, 4, 11, 8, 6}, Relation::le, 16},
			  {{5, 5, 11, 7, 6, 6}, Relation::le, 13}};
	ip::Options large_options;
	large_options.cuts = 0;
	large_options.strong_branching = 0;
	check(p, "large objective bound must preserve a one-unit gap", large_options);
	r = build(p).maximize(large_options);
	require(r.status == ip::Status::Optimal && r.objective == 2000000000013.0 &&
			r.x == std::vector<double>({0, 0, 0, 0, 1, 1}),
			"large objective exact regression");
	++checks;
	p = make_problem({2000000000011.0, 11, 10, 1000000000003.0, 4, 0},
					 {1, 1, 1, 1, 1, 1});
	p.rows = {{{5, 3, 5, 14, 4, 2}, Relation::le, 11},
			  {{12, 1, 7, 14, 9, 2}, Relation::le, 15},
			  {{13, 9, 3, 1, 10, 2}, Relation::le, 12},
			  {{6, 10, 3, 10, 1, 2}, Relation::le, 10}};
	check(p, "large reduced costs with small optimal objective", large_options);
	r = build(p).maximize(large_options);
	require(r.status == ip::Status::Optimal && r.objective == 11,
			"large coefficients small objective exact regression");
	++checks;
	p = make_problem({2000000000010.0, -1000000000015.0, 13,
					  2000000000009.0, 2000000000008.0}, {1, 1, 2, 1, 2});
	p.lo = {-1, -1, -2, -2, -2};
	p.rows = {{{2, -1, 0, 2, 2}, Relation::eq, 0},
			  {{-1, 12, 7, 2, 6}, Relation::le, 16},
			  {{-4, 8, 11, 5, -3}, Relation::le, 10},
			  {{-2, 9, -3, 8, 4}, Relation::le, 1},
			  {{4, -1, -7, 4, -6}, Relation::le, 8},
			  {{-5, 2, -1, -2, 5}, Relation::le, 0}};
	check(p, "GMI with large objective cancellation");

	ip::Solver unbounded({1});
	r = unbounded.maximize();
	require(r.status == ip::Status::UnboundedRelaxation && !r.has_solution(),
			"nonnegative unbounded relaxation");
	++checks;
	ip::Solver ambiguous({1, 0});
	ambiguous.add_eq({0, 2}, 1);
	r = ambiguous.maximize();
	require((r.status == ip::Status::UnboundedRelaxation ||
			 r.status == ip::Status::Infeasible) && !r.has_solution(),
			"unbounded relaxation must not claim integer feasibility");
	++checks;

	ip::Solver empty_feasible({1});
	empty_feasible.add_le({0}, 0);
	empty_feasible.add_le({1}, 2);
	r = empty_feasible.maximize();
	require(r.status == ip::Status::Optimal && near(r.objective, 2),
			"unrestricted upper bound with bounded polyhedron");
	++checks;

	ip::Solver fractional_lower({1});
	fractional_lower.bounds(0, 0.2);
	fractional_lower.add_le({1}, 2.8);
	r = fractional_lower.maximize();
	require(r.status == ip::Status::Optimal && near(r.objective, 2) && near(r.x[0], 2),
			"integer rounding of fractional finite lower bound");
	++checks;

	p = make_problem({1.01, 1}, {3, 3});
	p.rows = {{{2, 1}, Relation::le, 4}, {{1, 2}, Relation::le, 4}};
	ip::Solver reusable = build(p);
	ip::Options options;
	options.node_limit = 0;
	r = reusable.maximize(options);
	require(r.status == ip::Status::Limit && r.nodes == 0 && !r.has_solution(),
			"zero node limit");
	++checks;
	options.node_limit = std::numeric_limits<std::uint64_t>::max();
	options.time_limit = 0;
	r = reusable.maximize(options);
	require(r.status == ip::Status::Limit && r.nodes == 0 && !r.has_solution(),
			"zero time limit");
	++checks;
	options.time_limit = ip::INF;
	options.pivot_limit = 0;
	r = reusable.maximize(options);
	require(r.status == ip::Status::Limit && r.pivots == 0 && !r.has_solution(),
			"zero pivot limit");
	++checks;
	options.pivot_limit = 1000000;
	options.node_limit = 1;
	r = reusable.maximize(options);
	OracleResult oracle = enumerate(p);
	require((r.status == ip::Status::Limit || r.status == ip::Status::Optimal) &&
			r.nodes <= 1 && r.bound + tol >= oracle.objective,
			"one node limit and valid upper bound");
	if (r.has_solution()) {
		require(feasible(p, r.x) && near(r.objective, dot(p.c, r.x)) &&
				r.objective <= oracle.objective + tol,
				"limited solve returned an invalid incumbent");
	}
	++checks;
	r = reusable.maximize();
	require(r.status == ip::Status::Optimal && near(r.objective, oracle.objective),
			"repeated solve after limit");
	++checks;

	ip::Solver infinite_tree({0, 0, 0});
	infinite_tree.bounds(2, 0, 0);
	infinite_tree.add_eq({2, -2, 1}, 1);
	options = ip::Options();
	options.node_limit = 20;
	options.strong_branching = 0;
	options.cuts = 0;
	r = infinite_tree.maximize(options);
	require((r.status == ip::Status::Limit || r.status == ip::Status::Infeasible) &&
			!r.has_solution() && r.nodes <= options.node_limit,
			"node limit on infeasible unbounded integer domain");
	++checks;
}

void randomized_tests(std::uint64_t seed, int count, bool mixed, bool rational = false) {
	std::mt19937_64 rng(seed);
	auto draw = [&](int lo, int hi) {
		return std::uniform_int_distribution<int>(lo, hi)(rng);
	};
	for (int iteration = 0; iteration < count; ++iteration) {
		int n = draw(1, 5);
		Problem p = make_problem(std::vector<double>(n), std::vector<double>(n));
		std::vector<double> witness(n);
		for (int j = 0; j < n; ++j) {
			p.c[j] = draw(-5, 5);
			p.lo[j] = draw(-2, 1);
			p.hi[j] = p.lo[j] + draw(0, 4);
			witness[j] = draw(static_cast<int>(p.lo[j]), static_cast<int>(p.hi[j]));
		}
		if (mixed) {
			int j = draw(0, n - 1);
			p.integer[j] = false;
			p.lo[j] += 0.25;
			p.hi[j] += 0.5;
			witness[j] = p.lo[j] + draw(0, static_cast<int>(4 * (p.hi[j] - p.lo[j]))) / 4.0;
			for (double& c : p.c) c /= 4;
		}
		if (rational) for (double& c : p.c) c /= 4;
		bool construct_feasible = draw(0, 9) < 6;
		int m = draw(0, 10);
		for (int i = 0; i < m; ++i) {
			Row row{std::vector<double>(n), Relation::le, 0};
			for (double& a : row.a) a = draw(-5, 5) / (rational ? 4.0 : 1.0);
			int relation = draw(0, 9);
			row.relation = relation < 4 ? Relation::le :
						   relation < 8 ? Relation::ge : Relation::eq;
			if (construct_feasible) {
				row.b = dot(row.a, witness);
				if (row.relation == Relation::le) row.b += draw(0, 5) / (rational ? 4.0 : 1.0);
				if (row.relation == Relation::ge) row.b -= draw(0, 5) / (rational ? 4.0 : 1.0);
			} else {
				row.b = draw(-12, 12);
				if (mixed) row.b /= 4;
				if (rational) row.b /= 8;
			}
			p.rows.push_back(std::move(row));
		}
		ip::Options options;
		// Check both plain branching and strong branching against the same oracle.
		options.strong_branching = iteration % 2 == 0 ? 0 : 3;
		options.cuts = iteration % 4 < 2 ? 0 : 8;
		check(p, std::string(mixed ? "mixed" : rational ? "rational" : "integer") + " seed=" +
				 std::to_string(seed) + " case=" + std::to_string(iteration), options);
		if (construct_feasible && iteration % 5 == 0) {
			options.initial_solution = witness;
			check(p, "initial solution seed=" + std::to_string(seed) +
					 " case=" + std::to_string(iteration), options);
		}
		if (iteration % 5 == 0)
			check(p, "minimize seed=" + std::to_string(seed) +
					 " case=" + std::to_string(iteration), options, true);
	}
}

void randomized_lp_tests(std::uint64_t seed, int count) {
	std::mt19937_64 rng(seed);
	auto draw = [&](int lo, int hi) {
		return std::uniform_int_distribution<int>(lo, hi)(rng);
	};
	for (int iteration = 0; iteration < count; ++iteration) {
		int n = draw(2, 3);
		Problem p = make_problem(std::vector<double>(n), std::vector<double>(n));
		p.integer.assign(n, false);
		std::vector<double> witness(n);
		for (int j = 0; j < n; ++j) {
			p.c[j] = draw(-10, 10) / 4.0;
			p.lo[j] = draw(-8, 4) / 4.0;
			p.hi[j] = p.lo[j] + draw(0, 16) / 4.0;
			witness[j] = (p.lo[j] + p.hi[j]) / 2;
		}
		bool construct_feasible = draw(0, 9) < 6;
		int m = draw(0, 8);
		for (int i = 0; i < m; ++i) {
			Row row{std::vector<double>(n), Relation::le, 0};
			for (double& a : row.a) a = draw(-5, 5) / 4.0;
			int relation = draw(0, 9);
			row.relation = relation < 4 ? Relation::le :
						   relation < 8 ? Relation::ge : Relation::eq;
			if (construct_feasible) {
				row.b = dot(row.a, witness);
				if (row.relation == Relation::le) row.b += draw(0, 5) / 4.0;
				if (row.relation == Relation::ge) row.b -= draw(0, 5) / 4.0;
			} else {
				row.b = draw(-12, 12) / 4.0;
			}
			p.rows.push_back(std::move(row));
		}
		check(p, "LP seed=" + std::to_string(seed) + " case=" + std::to_string(iteration));
		if (iteration % 5 == 0)
			check(p, "minimize LP seed=" + std::to_string(seed) +
					 " case=" + std::to_string(iteration), {}, true);
	}
}

void randomized_large_objective_tests(std::uint64_t seed, int count) {
	std::mt19937_64 rng(seed);
	auto draw = [&](int lo, int hi) {
		return std::uniform_int_distribution<int>(lo, hi)(rng);
	};
	for (int iteration = 0; iteration < count; ++iteration) {
		int n = draw(2, 7);
		Problem p = make_problem(std::vector<double>(n), std::vector<double>(n));
		const bool cancellation = iteration % 2 != 0;
		std::vector<double> equality(n);
		for (int j = 0; j < n; ++j) {
			if (cancellation) {
				p.lo[j] = -draw(1, 2);
				p.hi[j] = draw(1, 2);
				equality[j] = draw(-3, 3);
				p.c[j] = 1e12 * equality[j] + draw(-15, 15);
			} else {
				p.lo[j] = draw(-2, 0);
				p.hi[j] = p.lo[j] + draw(1, 2);
				p.c[j] = 1e12 * draw(-2, 3) + draw(-15, 15);
			}
		}
		if (cancellation) p.rows.push_back({equality, Relation::eq, 0});
		int m = draw(1, 6);
		for (int i = 0; i < m; ++i) {
			Row row{std::vector<double>(n), Relation::le, 0};
			for (double& a : row.a) a = draw(cancellation ? -8 : 0, 12);
			row.b = draw(0, 18);
			p.rows.push_back(std::move(row));
		}
		ip::Options options;
		options.cuts = iteration % 4 < 2 ? 0 : 8;
		options.strong_branching = iteration % 8 < 4 ? 0 : 3;
		check(p, "large objective seed=" + std::to_string(seed) +
				 " case=" + std::to_string(iteration), options);
		if (iteration % 5 == 0)
			check(p, "minimize large objective seed=" + std::to_string(seed) +
					 " case=" + std::to_string(iteration), options, true);
	}
}

void minimization_tests() {
	ip::Solver model({3, 2});
	model.bounds(0, -3.5, 3.5);
	model.bounds(1, -2, 4);
	model.continuous(1);
	model.add_eq({1, 2}, 2);
	const auto s = model;
	for (int repeat = 0; repeat < 2; ++repeat) {
		auto r = s.minimize();
		require(r.status == ip::Status::Optimal && r.objective == -4 &&
				r.bound == -4 && r.x == ip::Vec({-3, 2.5}), "mixed integer minimization with offset");
		r = s.maximize();
		require(r.status == ip::Status::Optimal && r.objective == 8 &&
				r.bound == 8 && r.x == ip::Vec({3, -0.5}), "minimize must not mutate the model");
		checks += 2;
	}
	ip::Options o;
	o.initial_solution = {-1, 1.5};
	require(s.minimize(o).objective == -4, "improve a supplied minimization solution");
	++checks;
	for (bool hint : {false, true}) for (int limit = 0; limit < 3; ++limit) {
		o = ip::Options();
		if (hint) o.initial_solution = {-1, 1.5};
		if (limit == 0) o.node_limit = 0;
		if (limit == 1) o.pivot_limit = 0;
		if (limit == 2) o.time_limit = 0;
		auto r = s.minimize(o);
		require(r.status == ip::Status::Limit && r.pivots == 0 &&
				r.has_solution() == hint && r.objective == (hint ? 0 : ip::INF) && r.bound == -ip::INF,
				"minimize limit before a root bound, with and without a start");
		if (hint) require(r.x == o.initial_solution, "initial solution coordinates must not be negated");
		++checks;
	}
	for (double cost : {1.0, 1.25}) {
		ip::Solver cover(ip::Vec(3, cost));
		for (int j = 0; j < 3; ++j) cover.bounds(j, 0, 1);
		cover.add_ge({1, 1, 0}, 1);
		cover.add_ge({0, 1, 1}, 1);
		cover.add_ge({1, 0, 1}, 1);
		o = ip::Options(); o.cuts = 0; o.node_limit = 1;
		auto r = cover.minimize(o);
		require(r.status == ip::Status::Limit && r.has_solution() && r.nodes == 1 &&
				r.objective == 3 * cost && near(r.bound, cost == 1 ? 2 : 1.5 * cost) &&
				r.bound <= 2 * cost && r.objective >= 2 * cost,
				"minimize must return a lower bound, rounding upward for integer objectives");
		o = ip::Options(); o.initial_solution = {1, 1, 0};
		if (cost == 1) o.node_limit = 1;
		r = cover.minimize(o);
		require(r.status == ip::Status::Optimal && r.objective == 2 * cost && r.bound == r.objective,
				"minimize with a matching incumbent");
		checks += 2;
	}
	for (int kind = 0; kind < 3; ++kind) {
		ip::Solver bad({1});
		if (kind == 0) bad.bounds(0, 1, 0);
		if (kind == 1) bad.add_le({1}, -1);
		if (kind == 2) bad.add_eq({1.5}, 0.5);
		o = ip::Options(); o.cuts = 0;
		auto r = bad.minimize(o);
		require(r.status == ip::Status::Infeasible && !r.has_solution() &&
				r.objective == ip::INF && r.bound == ip::INF, "minimize infeasible sentinels");
		++checks;
	}
	ip::Solver unbounded({-1});
	for (bool hint : {false, true}) {
		o = ip::Options();
		if (hint) o.initial_solution = {2};
		auto r = unbounded.minimize(o);
		require(r.status == ip::Status::UnboundedRelaxation && r.has_solution() == hint &&
				r.objective == (hint ? -2 : ip::INF) && r.bound == -ip::INF,
				"unbounded minimization relaxation");
		++checks;
	}
	ip::Solver conflicting({-1});
	conflicting.add_eq({1}, 1e-10);
	o = ip::Options(); o.initial_solution = {0};
	auto r = conflicting.minimize(o);
	require(r.status == ip::Status::NumericalError && r.has_solution() &&
			r.objective == 0 && r.bound == -ip::INF, "minimize numerical error retains the candidate");
	++checks;
	ip::Solver empty({});
	r = empty.minimize();
	require(r.status == ip::Status::Optimal && r.has_solution() && r.x.empty() &&
			r.objective == 0 && r.bound == 0, "zero-variable minimization");
	empty.add_le({}, -1);
	r = empty.minimize();
	require(r.status == ip::Status::Infeasible && !r.has_solution() &&
			r.objective == ip::INF && r.bound == ip::INF, "infeasible zero-variable minimization");
	checks += 2;
	o = ip::Options(); o.eps = 0;
	bool threw = false;
	try { s.minimize(o); } catch (const std::invalid_argument&) { threw = true; }
	require(threw && s.maximize().objective == 8 && s.minimize().objective == -4,
			"failed minimization must leave the model unchanged");
	++checks;
}

void improvement_tests() {
	// Validate and retain a supplied solution even when no LP work is allowed.
	ip::Solver s({3, 2});
	s.bounds(0, -3.5, 3.5);
	s.bounds(1, -2, 4);
	s.continuous(1);
	s.add_eq({1, 2}, 2);
	for (int limit = 0; limit < 3; ++limit) {
		ip::Options o;
		o.initial_solution = {-1, 1.5};
		if (limit == 0) o.node_limit = 0;
		if (limit == 1) o.pivot_limit = 0;
		if (limit == 2) o.time_limit = 0;
		auto r = s.maximize(o);
		require(r.status == ip::Status::Limit && r.has_solution() &&
				r.x == o.initial_solution && r.objective == 0 && r.bound == ip::INF,
				"initial solution must survive a limit in original coordinates");
		++checks;
	}
	ip::Options o;
	o.initial_solution = {-1, 1.5};
	auto r = s.maximize(o);
	require(r.status == ip::Status::Optimal && r.objective == 8 &&
			r.x == ip::Vec({3, -0.5}), "improve a supplied mixed integer solution");
	++checks;
	ip::Solver tight({-1, -1, -1});
	for (int j = 0; j < 3; ++j) tight.bounds(j, 0, 1);
	tight.add_ge({1, 1, 0}, 1);
	tight.add_ge({0, 1, 1}, 1);
	tight.add_ge({1, 0, 1}, 1);
	o.initial_solution = {1, 1, 0};
	o.node_limit = 1;
	r = tight.maximize(o);
	require(r.status == ip::Status::Optimal && r.objective == -2 && r.nodes == 1,
			"initial solution matching rounded root bound must skip cuts and branching");
	++checks;
	ip::Solver conflicting({1});
	conflicting.add_eq({1}, 1e-10);
	o = ip::Options();
	o.initial_solution = {0}; // Within eps of the equality, but not its integer lattice.
	r = conflicting.maximize(o);
	require(r.status == ip::Status::NumericalError && r.has_solution() && r.bound == ip::INF,
			"an infeasibility claim conflicting with an accepted start is a numerical error");
	++checks;

	// Nonnegative rows may imply bounds after shifting negative lower bounds.
	Problem p = make_problem({3, 2}, {1, 2});
	p.lo = {-2, -1};
	p.rows = {{{1, 1}, Relation::le, 0}};
	check(p, "redundant upper bounds with negative lower bounds");
	p.integer[1] = false;
	p.lo[1] = -1.25;
	p.hi[0] = 0.75;
	check(p, "retain a stronger integer upper bound");
	p = make_problem({1, 1}, {1, 1});
	p.rows = {{{1, -1}, Relation::le, 0}, {{-1, 1}, Relation::le, 0}};
	check(p, "cyclic implications must not remove both upper bounds");
	p.rows = {{{1, 1}, Relation::le, -1}};
	check(p, "negative RHS nonnegative row");
}

void validation_tests() {
	auto invalid = [&](auto&& operation, const std::string& name) {
		bool threw = false;
		try {
			operation();
		} catch (const std::invalid_argument&) {
			threw = true;
		}
		require(threw, "expected invalid_argument: " + name);
		++checks;
	};
	invalid([] { ip::Solver s({std::numeric_limits<double>::quiet_NaN()}); },
			"NaN objective");
	invalid([] { ip::Solver s({ip::INF}); }, "infinite objective");
	invalid([] { ip::Solver s({1, 2}); s.add_le({1}, 0); }, "constraint dimension");
	invalid([] { ip::Solver s({1}); s.add_le({ip::INF}, 0); }, "infinite coefficient");
	invalid([] { ip::Solver s({1}); s.add_le({1}, ip::INF); }, "infinite RHS");
	invalid([] { ip::Solver s({1}); s.bounds(0, -ip::INF); }, "infinite lower bound");
	invalid([] { ip::Solver s({1}); s.bounds(1, 0, 1); }, "bound index");
	invalid([] { ip::Solver s({1}); s.continuous(-1); }, "continuous index");
	invalid([] { ip::Solver s({1}); ip::Options o; o.eps = 0; s.maximize(o); },
			"nonpositive epsilon");
	invalid([] { ip::Solver s({1}); ip::Options o; o.integer_eps = 0.5; s.maximize(o); },
			"integer epsilon too large");
	invalid([] { ip::Solver s({1}); ip::Options o; o.time_limit = -1; s.maximize(o); },
			"negative time limit");
	for (const auto& x : std::vector<ip::Vec>{{1}, {0, 0.5}, {0, ip::INF},
			 {0, std::numeric_limits<double>::quiet_NaN()}, {-1, 1}, {2, 0}, {0, 0}}) {
		invalid([&] {
			ip::Solver s({1, 1});
			s.bounds(0, 0, 1);
			s.add_eq({1, 1}, 1);
			ip::Options o;
			o.initial_solution = x;
			o.node_limit = 0;
			s.maximize(o);
		}, "invalid initial solution");
	}
}

} // namespace

int main(int argc, char** argv) {
	try {
		int cases = argc > 1 ? std::stoi(argv[1]) : 1000;
		deterministic_tests();
		minimization_tests();
		improvement_tests();
		randomized_tests(0x13579bdf2468ace0ULL, cases, false);
		randomized_tests(0xabcdef0123456789ULL, cases / 2, true);
		randomized_tests(0x123456789abcdef0ULL, cases / 2, false, true);
		randomized_lp_tests(0xfedcba9876543210ULL, cases / 2);
		randomized_large_objective_tests(0x1020304050607080ULL, cases / 2);
		validation_tests();
		std::cout << "Passed " << checks << " solver checks\n";
		return 0;
	} catch (const std::exception& e) {
		std::cerr << "FAIL: " << e.what() << '\n';
		return 1;
	}
}
