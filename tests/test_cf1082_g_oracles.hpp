#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <numeric>
#include <queue>
#include <tuple>
#include <vector>

namespace cf1082_oracle {
using ll=long long;
using Edges=std::vector<std::tuple<int,int,ll>>;
inline ll brute(const std::vector<ll>& cost,const Edges& edges) {
    int n=int(cost.size()); ll best=0;
    for (unsigned mask=1;mask<(1u<<n);++mask) {
        ll value=0;
        for (int v=0;v<n;++v) if (mask>>v&1u) value-=cost[v];
        for (auto [u,v,w]:edges) if ((mask>>u&1u) && (mask>>v&1u)) value+=w;
        best=std::max(best,value);
    }
    return best;
}

// Original maximum-closure network, without any of the submission's reductions.
inline ll flow(const std::vector<ll>& cost,const Edges& edges) {
    int n=int(cost.size()),m=int(edges.size()),source=n+m,sink=source+1;
    struct Edge { int v,rev; ll cap; };
    std::vector<std::vector<Edge>> graph(sink+1);
    auto add=[&](int u,int v,ll cap) {
        graph[u].push_back({v,int(graph[v].size()),cap});
        graph[v].push_back({u,int(graph[u].size())-1,0});
    };
    ll total=0; for (auto [u,v,w]:edges) total+=w;
    for (int v=0;v<n;++v) add(v,sink,cost[v]);
    for (int j=0;j<m;++j) {
        auto [u,v,w]=edges[j]; add(source,n+j,w);
        add(n+j,u,total+1); add(n+j,v,total+1);
    }
    ll value=0; std::vector<int> level(sink+1),next(sink+1);
    for (;;) {
        std::fill(level.begin(),level.end(),-1);
        std::queue<int> queue; queue.push(source); level[source]=0;
        while (!queue.empty()) {
            int u=queue.front(); queue.pop();
            for (const auto& e:graph[u]) if (e.cap && level[e.v]<0)
                level[e.v]=level[u]+1,queue.push(e.v);
        }
        if (level[sink]<0) return total-value;
        std::fill(next.begin(),next.end(),0);
        std::function<ll(int,ll)> send=[&](int u,ll cap) -> ll {
            if (u==sink) return cap;
            for (int& j=next[u];j<int(graph[u].size());++j) {
                auto& e=graph[u][j];
                if (e.cap && level[e.v]==level[u]+1) {
                    ll pushed=send(e.v,std::min(cap,e.cap));
                    if (pushed) { e.cap-=pushed; graph[e.v][e.rev].cap+=pushed; return pushed; }
                }
            }
            return 0;
        };
        while (ll pushed=send(source,total+1)) value+=pushed;
    }
}
} // namespace cf1082_oracle
