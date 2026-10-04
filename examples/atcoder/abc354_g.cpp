// https://atcoder.jp/contests/abc354/tasks/abc354_g
#include "ip_solver.hpp"
#include <iostream>
#include <map>
#include <string>

ip::Result select_strings(const std::vector<std::string>& input,
                          const std::vector<long long>& weight) {
    // Equal strings conflict: only the most valuable copy is needed.
    std::map<std::string, long long> unique;
    for (int i=0; i<int(input.size()); ++i)
        unique[input[i]]=std::max(unique[input[i]], weight[i]);
    std::vector<std::string> s;
    ip::Vec c;
    for (const auto& item:unique) {
        s.push_back(item.first);
        c.push_back(double(item.second));
        c.push_back(-double(item.second));
    }
    int n=int(s.size());
    std::vector<std::vector<int>> contains(n, std::vector<int>(n));
    for (int i=0; i<n; ++i) for (int j=0; j<n; ++j)
        contains[i][j]=i!=j && s[j].find(s[i])!=std::string::npos;
    ip::Solver solver(c);
    for (int i=0; i<n; ++i) {
        solver.bounds(2*i, 0, 1); // p_i; 0 <= q_i <= p_i makes q_i binary too
        ip::Vec a(2*n);
        a[2*i+1]=1; a[2*i]=-1;
        solver.add_le(a, 0);
    }
    for (int i=0; i<n; ++i) for (int j=0; j<n; ++j) if (contains[i][j]) {
        bool redundant=false;
        for (int k=0; k<n; ++k)
            if (contains[i][k] && contains[k][j]) { redundant=true; break; }
        if (redundant) continue;
        ip::Vec a(2*n);
        a[2*i]=1; a[2*j+1]=-1;
        solver.add_le(a, 0); // p_i <= q_j for each cover relation i < j
    }
    // Select i iff p_i-q_i=1. Difference constraints give an integral LP.
    ip::Options options;
    options.cuts=0;
    return solver.maximize(options);
}

#ifndef IP_EXAMPLE_TEST
int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int n;
    std::cin >> n;
    std::vector<std::string> s(n);
    std::vector<long long> a(n);
    for (auto& x:s) std::cin >> x;
    for (auto& x:a) std::cin >> x;
    auto r=select_strings(s, a);
    if (r.status!=ip::Status::Optimal) return 1;
    std::cout << std::llround(r.objective) << '\n';
}
#endif
