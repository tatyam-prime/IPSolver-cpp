#define IP_ABC165_ORACLES_ONLY
#include "test_abc165_c.cpp"
#include <fstream>

void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

void mine_layer(bool continuous, double eps, bool reverse = false, int shift = 0) {
    constexpr int side = 17, n = side * side;
    ip::Vec objective(n), witness(n);
    for (int i = 0; i < side; ++i) for (int j = 0; j < side; ++j) {
        witness[i * side + j] = (i + j) % 2 + shift;
        objective[i * side + j] = i == side / 2;
    }
    ip::Solver s(objective);
    for (int j = 0; j < n; ++j) {
        s.bounds(j, shift, shift + 1);
        if (continuous) s.continuous(j);
    }
    std::vector<ip::Vec> rows;
    ip::Vec rhs;
    for (int k = 0; k < n; ++k) {
        int cell = reverse ? n - 1 - k : k, i = cell / side, j = cell % side;
        ip::Vec row(n);
        for (int u = std::max(0, i - 1); u <= std::min(side - 1, i + 1); ++u)
            for (int v = std::max(0, j - 1); v <= std::min(side - 1, j + 1); ++v)
                row[u * side + v] = 1;
        double b = std::inner_product(row.begin(), row.end(), witness.begin(), 0.0);
        // Also exercise reversed signs and redundant equalities.
        if (reverse) { for (auto& v : row) v = -v; b = -b; }
        s.add_eq(row, b);
        if (reverse && k % 7 == 0) s.add_eq(row, b);
        rows.push_back(std::move(row)); rhs.push_back(b);
    }
    ip::Options o;
    o.cuts = 0;
    o.eps = eps;
    o.pivot_limit = 100000;
    auto check = [&](const ip::Result& r) {
        // For side=17 the middle-row total is fixed by the window equations.
        require(r.status == ip::Status::Optimal && r.has_solution() &&
                std::abs(r.objective - (8 + side * shift)) < 1e-6,
                "Mine Layer must not report false infeasibility or use an invalid bound");
        require(int(r.x.size()) == n, "invalid Mine Layer solution size");
        for (double x : r.x) {
            require(std::isfinite(x) && x >= shift - 1e-6 && x <= shift + 1 + 1e-6,
                    "Mine Layer variable bound");
            if (!continuous) require(std::abs(x - std::round(x)) < 1e-6, "Mine Layer integrality");
        }
        for (int i = 0; i < n; ++i)
            require(std::abs(std::inner_product(rows[i].begin(), rows[i].end(), r.x.begin(), 0.0) - rhs[i]) < 1e-6,
                    "Mine Layer original equality residual");
        require(r.nodes <= o.node_limit && r.pivots <= o.pivot_limit, "Mine Layer budget");
    };
    check(s.solve(o));
    if (!continuous && eps == 1e-9 && !reverse) {
        o.initial_solution = witness;
        check(s.solve(o));
    }
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "usage: test_numerical tests/data");
        for (bool continuous : {false, true}) for (double eps : {1e-9, 1e-8, 1e-7})
            mine_layer(continuous, eps);
        mine_layer(false, 1e-9, true, -2);
        mine_layer(true, 1e-9, true);
        for (std::string name : {"pair", "clique"}) {
            std::ifstream f(std::string(argv[1]) + "/abc165_c_" + name + "_numeric.in");
            int n, m, k;
            require(bool(f >> n >> m >> k) && n == 10 && m == 10 && k == 50, "invalid fixture");
            std::vector<Requirement> q(k);
            for (auto& x : q) {
                require(bool(f >> x.a >> x.b >> x.c >> x.d), "invalid requirement");
                --x.a; --x.b;
            }
            auto expected = sequence_oracle(n, m, q);
            for (int cuts : {0, 8}) for (int conflict : {1, 2}) {
                ip::Options o;
                o.cuts = cuts;
                auto r = many_requirements(n, m, q, o, true, true, true, conflict);
                require(r.result.status == ip::Status::Optimal && r.score == expected &&
                        r.result.objective == expected && r.result.bound == expected,
                        "ABC165 C strengthened model must match the independent oracle");
            }
            if (name == "clique") {
                // The first cold rebuild starts around LP 1471 / pivot 6811 on this fixture.
                // Both limits include the original search and rebuilding work.
                for (bool nodes : {false, true}) {
                    ip::Options o;
                    o.cuts = 0;
                    if (nodes) o.node_limit = 1500;
                    else o.pivot_limit = 6820;
                    auto r = many_requirements(n, m, q, o, true, true, true, 1).result;
                    require(r.status == ip::Status::Limit && r.nodes <= o.node_limit && r.pivots <= o.pivot_limit,
                            "rebuilding must respect global limits");
                    require(nodes ? r.nodes == o.node_limit : r.pivots == o.pivot_limit,
                            "rebuilding must not reset counters");
                    require(r.has_solution() && r.objective <= expected && r.bound >= expected,
                            "rebuilding must retain a valid incumbent and bound on limit");
                }
            }
        }
        std::cout << "Numerical regressions passed: Mine Layer, strengthened ABC165 C, rebuild budgets\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
