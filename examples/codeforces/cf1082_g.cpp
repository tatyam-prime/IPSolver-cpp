// https://codeforces.com/problemset/problem/1082/G
#include "ip_solver.hpp"
#include <iostream>
#include <map>
using namespace std;
using ll=long long;
struct ProfitEdge { int u,v; ll weight; };
struct GraphProfitAnswer {
    ll weight=0;
    int variables=0,rows=0,components=0;
    ip::Result result;
};

GraphProfitAnswer graph_profit(vector<ll> cost,const vector<ProfitEdge>& edges,
                              ip::Options options={},bool reduce=true,bool hint=true) {
    auto began=chrono::steady_clock::now();
    auto expired=[&] { return chrono::duration<double>(chrono::steady_clock::now()-began).count()>=options.time_limit; };
    auto timed_out=[] { GraphProfitAnswer a; a.result.status=ip::Status::Limit; return a; };
    if (expired()) return timed_out();
    int n=int(cost.size()); vector<map<int,ll>> g(n);
    for (auto [u,v,w]:edges) { g[u][v]+=w; g[v][u]+=w; }
    vector<bool> alive(n,true); ll fixed=0;
    auto erase_vertex=[&](int v,bool selected) {
        if (selected) fixed-=cost[v];
        for (auto [u,w]:g[v]) {
            g[u].erase(v);
            if (selected) cost[u]-=w;
        }
        g[v].clear(); alive[v]=false;
    };
    if (reduce) for (;;) {
        if (expired()) return timed_out();
        int chosen=-1,type=-1,other=-1;
        for (int v=0;v<n;++v) if (alive[v]) {
            ll total=0; for (auto [u,w]:g[v]) total+=w;
            if (cost[v]<=0) { chosen=v; type=1; break; }
            if (cost[v]>=total) { chosen=v; type=0; break; }
            if (g[v].size()<=2) { chosen=v; type=2; break; }
        }
        if (chosen<0) for (int u=0;u<n && chosen<0;++u) if (alive[u])
            for (auto [v,w]:g[u]) if (u<v && w>=cost[u]+cost[v]) {
                chosen=u; other=v; type=1; break;
            }
        if (chosen<0) break;
        if (type<2) {
            erase_vertex(chosen,type==1);
            if (other>=0) erase_vertex(other,true);
            continue;
        }
        // max_v (a*xu*v+b*xw*v-c*v)=p*xu+q*xw+(max(0,a+b-c)-p-q)*xu*xw.
        // Here c>0 and the last coefficient is nonnegative; merge parallel edges.
        auto it=g[chosen].begin(); int u=it->first; ll a=it->second;
        int v=-1; ll b=0; ++it;
        if (it!=g[chosen].end()) { v=it->first; b=it->second; }
        g[u].erase(chosen);
        if (v<0) cost[u]-=a-cost[chosen];
        else {
            g[v].erase(chosen);
            ll p=max(0LL,a-cost[chosen]),q=max(0LL,b-cost[chosen]);
            ll joined=max(0LL,a+b-cost[chosen])-p-q;
            cost[u]-=p; cost[v]-=q;
            if (joined) { g[u][v]+=joined; g[v][u]+=joined; }
        }
        g[chosen].clear(); alive[chosen]=false;
    }
    GraphProfitAnswer answer; answer.weight=fixed; answer.result.status=ip::Status::Optimal;
    vector<bool> seen(n);
    for (int start=0;start<n;++start) if (!seen[start] && !g[start].empty()) {
        vector<int> vertices{start}; seen[start]=true;
        for (int j=0;j<int(vertices.size());++j) for (auto [v,w]:g[vertices[j]])
            if (!seen[v]) { seen[v]=true; vertices.push_back(v); }
        vector<int> id(n,-1); ip::Vec objective;
        ll all=0;
        for (int v:vertices) { id[v]=int(objective.size()); objective.push_back(-double(cost[v])); all-=cost[v]; }
        vector<ProfitEdge> remaining;
        for (int u:vertices) for (auto [v,w]:g[u]) if (u<v) {
            remaining.push_back({id[u],id[v],w}); objective.push_back(double(w)); all+=w;
        }
        int nv=int(vertices.size()),ne=int(remaining.size()),k=nv+ne;
        ip::Solver solver(objective);
        for (int i=0;i<nv;++i) solver.bounds(i,0,1);
        for (int j=0;j<ne;++j) for (int v:{remaining[j].u,remaining[j].v}) {
            ip::Vec row(k); row[nv+j]=1; row[v]=-1;
            solver.add_le(std::move(row),0);
        }
        ip::Options local=options; local.cuts=0; // The closure LP is integral.
        local.node_limit=options.node_limit>answer.result.nodes?options.node_limit-answer.result.nodes:0;
        local.pivot_limit=options.pivot_limit>answer.result.pivots?options.pivot_limit-answer.result.pivots:0;
        local.time_limit=max(0.0,options.time_limit-chrono::duration<double>(chrono::steady_clock::now()-began).count());
        local.initial_solution.clear();
        if (hint) local.initial_solution.assign(k,all>0?1:0);
        auto result=solver.maximize(local);
        answer.variables+=k; answer.rows+=2*ne+nv; ++answer.components;
        answer.result.nodes+=result.nodes; answer.result.pivots+=result.pivots;
        if (result.status!=ip::Status::Optimal) {
            answer.result.status=result.status; answer.result.objective=-ip::INF;
            return answer;
        }
        ll value=0;
        for (int j=0;j<k;++j) value+=llround(objective[j])*llround(result.x[j]);
        answer.weight+=value;
    }
    answer.result.objective=answer.result.bound=double(answer.weight);
    return answer;
}

#ifndef IP_EXAMPLE_TEST
int main() {
    ios::sync_with_stdio(false); cin.tie(nullptr);
    int n,m; cin>>n>>m; vector<ll> cost(n);
    for (ll& v:cost) cin>>v;
    vector<ProfitEdge> edges(m);
    for (auto& [u,v,w]:edges) { cin>>u>>v>>w; --u; --v; }
    auto answer=graph_profit(cost,edges);
    if (answer.result.status!=ip::Status::Optimal) return 1;
    cout<<answer.weight<<'\n';
}
#endif
