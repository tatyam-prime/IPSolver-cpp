#include "ip_solver.hpp"
#include <iostream>

int main() {
    ip::Solver s({3, 2}); // maximize 3*x + 2*y; x,y >= 0, integer
    s.add_le({2, 1}, 4);
    s.add_le({1, 2}, 4);
    auto r = s.solve();
    if (r.status == ip::Status::Optimal) {
        std::cout << r.objective << '\n'; // 6
        for (double x : r.x) std::cout << x << ' '; // 2 0
        std::cout << '\n';
    }
}
