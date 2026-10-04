#include "ip_solver.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "usage: test_cut_recovery fixture.in");
        std::ifstream input(argv[1]);
        int n, m;
        long long unused;
        require(bool(input >> n >> m >> unused), "cannot read fixture");
        require(n == 100 && m == 20, "unexpected fixture dimensions");
        std::vector<unsigned> mask(n);
        for (auto& x : mask) {
            int count;
            require(bool(input >> unused >> unused >> count), "invalid friend");
            while (count--) {
                int p;
                require(bool(input >> p) && p >= 1 && p <= m, "invalid problem");
                x |= 1u << (p - 1);
            }
        }
        unsigned all = (1u << m) - 1;
        ip::Vec initial(n);
        // Independent certificate: no pair covers everything, and a triple does.
        for (int i = 0; i < n; ++i) for (int j = 0; j <= i; ++j) {
            require((mask[i] | mask[j]) != all, "fixture optimum is below three");
            for (int k = 0; k < j; ++k) if ((mask[i] | mask[j] | mask[k]) == all) {
                initial.assign(n, 0);
                initial[i] = initial[j] = initial[k] = 1;
            }
        }
        require(std::accumulate(initial.begin(), initial.end(), 0.0) == 3,
                "fixture has no three-person cover");
        ip::Solver s(ip::Vec(n, -1));
        for (int i = 0; i < n; ++i) s.bounds(i, 0, 1);
        for (int p = 0; p < m; ++p) {
            ip::Vec row(n);
            for (int i = 0; i < n; ++i) row[i] = (mask[i] >> p) & 1;
            s.add_ge(row, 1);
        }
        auto check = [&](ip::Options o, ip::Status status) {
            auto r = s.solve(o);
            require(r.status == status, "unexpected recovery status");
            require(r.nodes <= o.node_limit && r.pivots <= o.pivot_limit,
                    "recovery reset a global budget");
            require(r.bound >= -3 && r.objective <= -3, "invalid cover bounds");
            if (r.has_solution()) {
                require(int(r.x.size()) == n, "invalid solution size");
                unsigned covered = 0;
                double value = 0;
                for (int i = 0; i < n; ++i) {
                    require(r.x[i] == 0 || r.x[i] == 1, "nonbinary cover");
                    if (r.x[i]) covered |= mask[i];
                    value -= r.x[i];
                }
                require(covered == all && value == r.objective, "invalid incumbent");
            }
            if (status == ip::Status::Optimal)
                require(r.objective == -3 && r.bound == -3, "incorrect cover optimum");
            return r;
        };
        ip::Options o;
        auto r = check(o, ip::Status::Optimal);
        require(r.pivots < 100000, "cuts consumed too many pivots before recovery");
        std::cout << "cut recovery: " << r.nodes << " LP solves, " << r.pivots << " pivots\n";
        // Pivot pricing may avoid the old stall; limits still apply to the whole search.
        auto full = r;
        o.pivot_limit = full.pivots / 2;
        r = check(o, ip::Status::Limit);
        require(r.pivots == o.pivot_limit, "pivot budget was not shared across recovery");
        o = ip::Options();
        o.node_limit = full.nodes / 2;
        r = check(o, ip::Status::Limit);
        require(r.nodes == o.node_limit, "node budget was not shared across recovery");
        o = ip::Options();
        o.time_limit = 0;
        o.initial_solution = initial;
        r = check(o, ip::Status::Limit);
        require(r.x == initial && r.nodes == 0, "initial cover lost at time limit");
        // A valid incumbent plus a matching lower bound needs only the root LP.
        s.add_ge(ip::Vec(n, 1), 3);
        o.time_limit = ip::INF;
        o.node_limit = 1;
        check(o, ip::Status::Optimal);
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
