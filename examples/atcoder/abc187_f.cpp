// https://atcoder.jp/contests/abc187/tasks/abc187_f
#include "ip_solver.hpp"
#include <cstdint>
#include <functional>
#include <iostream>

struct CliqueCoverAnswer {
    int groups=-1, variables=0, rows=0;
    std::vector<uint32_t> patterns;
    ip::Result result;
};

CliqueCoverAnswer clique_cover(int n, const std::vector<std::pair<int,int>>& edges,
                             ip::Options options={}, bool hint=true) {
    std::vector<uint32_t> adj(n);
    for (auto [u,v]:edges) { adj[u]|=1u<<v; adj[v]|=1u<<u; }
    std::vector<uint32_t> patterns;
    // Bron-Kerbosch: enumerate inclusion-maximal cliques, including singletons.
    std::function<void(uint32_t,uint32_t,uint32_t)> dfs=[&](uint32_t r,uint32_t p,uint32_t x) {
        if (!(p|x)) { patterns.push_back(r); return; }
        int pivot=-1, score=-1;
        for (uint32_t left=p|x; left; left&=left-1) {
            int u=__builtin_ctz(left), value=__builtin_popcount(p&adj[u]);
            if (value>score) { score=value; pivot=u; }
        }
        for (uint32_t left=p&~adj[pivot]; left; left&=left-1) {
            uint32_t bit=left&-left; int v=__builtin_ctz(bit);
            dfs(r|bit,p&adj[v],x&adj[v]); p^=bit; x|=bit;
        }
    };
    uint32_t all=(1u<<n)-1;
    dfs(0,all,0);
    int k=int(patterns.size()), upper=0;
    ip::Vec initial(k);
    for (uint32_t remaining=all; remaining;) {
        int best=0;
        for (int j=1;j<k;++j)
            if (__builtin_popcount(patterns[j]&remaining)>
                __builtin_popcount(patterns[best]&remaining)) best=j;
        ++initial[best]; ++upper; remaining&=~patterns[best];
    }
    ip::Solver solver(ip::Vec(k,-1));
    for (int v=0;v<n;++v) {
        ip::Vec row(k);
        for (int j=0;j<k;++j) row[j]=(patterns[j]>>v)&1u;
        solver.add_ge(std::move(row),1);
    }
    // Bounds all pattern counts with one row, rather than k upper-bound rows.
    solver.add_le(ip::Vec(k,1),upper);
    if (hint) options.initial_solution=std::move(initial);
    auto result=solver.solve(options);
    int groups=-1;
    if (result.has_solution()) {
        groups=0;
        for (double value:result.x) groups+=int(std::llround(value));
    }
    return {groups,k,n+1,std::move(patterns),std::move(result)};
}

#ifndef IP_EXAMPLE_TEST
int main() {
    std::ios::sync_with_stdio(false); std::cin.tie(nullptr);
    int n,m; std::cin>>n>>m;
    std::vector<std::pair<int,int>> edges(m);
    for (auto& [u,v]:edges) { std::cin>>u>>v; --u; --v; }
    auto answer=clique_cover(n,edges);
    if (answer.result.status!=ip::Status::Optimal) return 1;
    std::cout<<answer.groups<<'\n';
}
#endif
