// https://atcoder.jp/contests/abc338/tasks/abc338_c
#include "ip_solver.hpp"
#include <iostream>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int n;
    std::cin >> n;
    std::vector<int> q(n), a(n), b(n);
    for (int& x:q) std::cin >> x;
    for (int& x:a) std::cin >> x;
    for (int& x:b) std::cin >> x;
    ip::Solver s({1, 1}); // maximize servings of A + servings of B
    int ua=1000000, ub=1000000;
    for (int i=0; i<n; ++i) {
        s.add_le({double(a[i]), double(b[i])}, q[i]);
        if (a[i]) ua=std::min(ua, q[i]/a[i]);
        if (b[i]) ub=std::min(ub, q[i]/b[i]);
    }
    s.bounds(0, 0, ua);
    s.bounds(1, 0, ub);
    auto r=s.maximize();
    if (r.status != ip::Status::Optimal) return 1;
    std::cout << std::llround(r.objective) << '\n';
}
