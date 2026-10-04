#define IP_EXAMPLE_TEST
#ifdef IP_TEST_STANDALONE
#include "../build/submissions/atcoder/abc354_g.cpp"
#else
#include "../examples/atcoder/abc354_g.cpp"
#endif
#include <chrono>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <queue>

using Strings = std::vector<std::string>;
using Weights = std::vector<long long>;

// Weighted Dilworth: sum(weights) minus bipartite max flow on all strict
// substring pairs. This oracle uses the full relation, without potential vars.
long long flow_oracle(const Strings& input, const Weights& weight) {
    std::map<std::string,long long> unique;
    for (int i=0; i<int(input.size()); ++i) unique[input[i]]=std::max(unique[input[i]],weight[i]);
    Strings s;
    Weights w;
    for (const auto& entry:unique) { s.push_back(entry.first); w.push_back(entry.second); }
    int n=int(s.size()), source=2*n, sink=source+1, vertices=sink+1;
    struct Edge { int v, rev; long long cap; };
    std::vector<std::vector<Edge>> graph(vertices);
    auto add=[&](int u,int v,long long cap) {
        graph[u].push_back({v,int(graph[v].size()),cap});
        graph[v].push_back({u,int(graph[u].size())-1,0});
    };
    long long total=std::accumulate(w.begin(),w.end(),0LL);
    for (int i=0; i<n; ++i) { add(source,i,w[i]); add(n+i,sink,w[i]); }
    for (int i=0; i<n; ++i) for (int j=0; j<n; ++j)
        if (i!=j && s[j].find(s[i])!=std::string::npos) add(i,n+j,total+1);
    std::vector<int> level(vertices), next(vertices);
    long long flow=0;
    for (;;) {
        std::fill(level.begin(),level.end(),-1);
        std::queue<int> queue;
        queue.push(source); level[source]=0;
        while (!queue.empty()) {
            int u=queue.front(); queue.pop();
            for (const auto& e:graph[u]) if (e.cap && level[e.v]<0)
                level[e.v]=level[u]+1, queue.push(e.v);
        }
        if (level[sink]<0) break;
        std::fill(next.begin(),next.end(),0);
        auto dfs=[&](auto&& self,int u,long long cap)->long long {
            if (u==sink) return cap;
            for (int& j=next[u]; j<int(graph[u].size()); ++j) {
                auto& e=graph[u][j];
                if (e.cap && level[e.v]==level[u]+1) {
                    long long sent=self(self,e.v,std::min(cap,e.cap));
                    if (sent) { e.cap-=sent; graph[e.v][e.rev].cap+=sent; return sent; }
                }
            }
            return 0;
        };
        while (long long sent=dfs(dfs,source,total+1)) flow+=sent;
    }
    return total-flow;
}

long long exhaustive(const Strings& s, const Weights& w) {
    int n=int(s.size());
    std::vector<uint32_t> conflict(n);
    for (int i=0; i<n; ++i) for (int j=0; j<n; ++j) if (i!=j &&
        (s[i].find(s[j])!=std::string::npos || s[j].find(s[i])!=std::string::npos))
        conflict[i] |= uint32_t(1)<<j;
    long long best=0;
    for (uint32_t mask=0; mask<(uint32_t(1)<<n); ++mask) {
        long long value=0;
        bool feasible=true;
        for (int i=0; i<n; ++i) if (mask>>i&1) {
            if (conflict[i]&mask) { feasible=false; break; }
            value+=w[i];
        }
        if (feasible) best=std::max(best,value);
    }
    return best;
}

void check(const Strings& s, const Weights& w, long long expected) {
    auto r=select_strings(s,w);
    if (r.status!=ip::Status::Optimal || !r.has_solution() ||
        std::llround(r.objective)!=expected || r.nodes!=1)
        throw std::runtime_error("ABC354 G oracle mismatch");
}

int main(int argc, char** argv) {
    int iterations=argc>1 ? std::stoi(argv[1]) : 2000;
    check({"atcoder","at","coder","code"},{5,2,3,4},6);
    check({"abcd","abc","ab","a","b","c","d","ab","bc","cd"},
          {100,10,50,30,60,90,80,70,40,20},260);
    check({"a","a","a"},{1,1000000000,2},1000000000);
    std::mt19937_64 rng(35420261004ULL);
    for (int t=0; t<iterations; ++t) {
        int n=1+int(rng()%14);
        Strings s(n);
        Weights w(n);
        for (int i=0; i<n; ++i) {
            int len=1+int(rng()%8);
            for (int j=0; j<len; ++j) s[i]+=char('a'+rng()%3);
            if (i && rng()%4==0) {
                const auto& previous=s[rng()%i];
                int start=int(rng()%previous.size());
                s[i]=previous.substr(start,1+rng()%(previous.size()-start));
            }
            w[i]=1+static_cast<long long>(rng()%(t%2 ? 1000000000 : 100));
        }
        auto expected=exhaustive(s,w);
        if (flow_oracle(s,w)!=expected) throw std::runtime_error("flow oracle mismatch");
        check(s,w,expected);
    }
    // 99 nested strings: total length 4950, all pairwise comparable.
    Strings chain;
    Weights weight;
    for (int i=1; i<=99; ++i) { chain.emplace_back(i,'a'); weight.push_back(1000000000-i); }
    auto start=std::chrono::steady_clock::now();
    check(chain,weight,999999999);
    auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    double worst=seconds;
    for (int t=0; t<200; ++t) {
        Strings s;
        Weights w;
        std::string base;
        for (int j=0; j<100; ++j) base+=char('a'+rng()%4);
        for (int i=0; i<100; ++i) {
            int len=1+int(rng()%45), pos=int(rng()%(101-len));
            s.push_back(base.substr(pos,len));
            if (t%3==0 && i%2) s.back()="z"+s.back();
            w.push_back(1+static_cast<long long>(rng()%1000000000));
        }
        auto expected=flow_oracle(s,w);
        start=std::chrono::steady_clock::now();
        check(s,w,expected);
        worst=std::max(worst,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
    }
    std::cout << "ABC354 G: " << iterations+204 << " checks; chain99 " << seconds*1000
              << " ms; worst max-size solve " << worst*1000 << " ms\n";
}
