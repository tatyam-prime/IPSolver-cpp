// https://atcoder.jp/contests/abc002/tasks/abc002_4
#include "ip_solver.hpp"
#include <iostream>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int n, m;
    std::cin >> n >> m;
    std::vector<std::vector<int>> edge(n, std::vector<int>(n));
    while (m--) {
        int u, v;
        std::cin >> u >> v;
        --u; --v;
        edge[u][v] = edge[v][u] = 1;
    }
    ip::Solver s(ip::Vec(n, 1)); // maximize the number of selected vertices
    for (int i=0; i<n; ++i) s.bounds(i, 0, 1);
    for (int i=0; i<n; ++i) for (int j=i+1; j<n; ++j) if (!edge[i][j]) {
        ip::Vec a(n);
        a[i] = a[j] = 1;
        s.add_le(a, 1); // nonadjacent vertices cannot both be selected
    }
    auto r = s.solve();
    if (r.status != ip::Status::Optimal) return 1;
    std::cout << std::llround(r.objective) << '\n';
}
