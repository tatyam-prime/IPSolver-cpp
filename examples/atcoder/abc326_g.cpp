// https://atcoder.jp/contests/abc326/tasks/abc326_g
#include "ip_solver.hpp"
#include <array>
#include <iostream>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int n, m;
    std::cin >> n >> m;
    std::vector<int> c(n), a(m);
    for (int& v:c) std::cin >> v;
    for (int& v:a) std::cin >> v;
    std::vector<std::vector<int>> l(m, std::vector<int>(n));
    std::vector<std::array<int,6>> id(n);
    for (auto& v:id) v.fill(-1);
    for (auto& v:l) for (int& x:v) std::cin >> x;

    ip::Vec objective(a.begin(), a.end()); // achievement variables come first
    for (int j=0; j<n; ++j) {
        int previous=1;
        for (int k=2; k<=5; ++k) {
            bool used=false;
            for (int i=0; i<m; ++i) used |= l[i][j]==k;
            if (used) {
                id[j][k]=int(objective.size());
                objective.push_back(-(k-previous)*c[j]);
                previous=k;
            }
        }
    }
    int variables=int(objective.size());
    ip::Solver s(objective);
    for (int i=0; i<variables; ++i) s.bounds(i, 0, 1);
    auto implies=[&](int from, int to) {
        ip::Vec row(variables);
        row[from]=1; row[to]=-1;
        s.add_le(std::move(row), 0);
    };
    for (int j=0; j<n; ++j) {
        int previous=-1;
        for (int k=2; k<=5; ++k) if (id[j][k]>=0) {
            if (previous>=0) implies(id[j][k], previous);
            previous=id[j][k];
        }
    }
    for (int i=0; i<m; ++i) for (int j=0; j<n; ++j)
        if (l[i][j]>1) implies(i, id[j][l[i][j]]);
    auto r=s.solve(); // This closure polytope has integral LP vertices.
    if (r.status!=ip::Status::Optimal) return 1;
    std::cout << std::llround(r.objective) << '\n';
}
