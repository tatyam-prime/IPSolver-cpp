#include "ip_solver.hpp"
#include <array>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
using namespace std;

// QOJ 10266: full capacity-12 patterns; surplus objects may be discarded.
struct PackingAnswer { int bins, variables, rows; ip::Result result; };
PackingAnswer pack12(const vector<int>& items, ip::Options o={}, bool hint=true) {
    using Pattern=array<int,13>;
    static const vector<Pattern> patterns=[] {
        vector<Pattern> p; Pattern a{};
        function<void(int,int)> dfs=[&](int k,int left) {
            if (k==1) { a[1]=left; p.push_back(a); return; }
            for (a[k]=0;a[k]*k<=left;++a[k]) dfs(k-1,left-a[k]*k);
            a[k]=0;
        };
        dfs(12,12); return p;
    }();
    Pattern count{}; int volume=0;
    for (int x:items) { ++count[x]; volume+=x; }
    vector<Pattern> bins; vector<int> space;
    for (int k=12;k;--k) for (int t=0;t<count[k];++t) {
        int j=0; while (j<int(bins.size()) && space[j]<k) ++j;
        if (j==int(bins.size())) { bins.push_back({}); space.push_back(12); }
        ++bins[j][k]; space[j]-=k;
    }
    int n=int(patterns.size()), upper=int(bins.size());
    ip::Solver s(ip::Vec(n,-1));
    for (int k=1;k<=12;++k) {
        ip::Vec row(n); for (int j=0;j<n;++j) row[j]=patterns[j][k];
        s.add_ge(row,count[k]);
    }
    // An upper bound also bounds every pattern count without n extra rows.
    s.add_le(ip::Vec(n,1),upper);
    s.add_ge(ip::Vec(n,1),(volume+11)/12);
#ifndef IP_NO_INITIAL
    if (hint) {
        map<Pattern,int> id; for (int j=0;j<n;++j) id[patterns[j]]=j;
        o.initial_solution.assign(n,0);
        for (int j=0;j<upper;++j) { bins[j][1]+=space[j]; ++o.initial_solution[id.at(bins[j])]; }
    }
#else
    (void)hint;
#endif
    auto r=s.maximize(o);
    int answer=r.has_solution()?0:-1; for (double x:r.x) answer+=int(llround(x));
    return {answer,n,14,std::move(r)};
}
#ifndef IP_EXAMPLE_TEST
int main() {
    ios::sync_with_stdio(false); cin.tie(nullptr);
    int t; cin>>t;
    while (t--) {
        int n; cin>>n; vector<int> a(n); for (int& x:a) cin>>x;
        auto r=pack12(a); if (r.result.status!=ip::Status::Optimal) return 1;
        cout<<r.bins<<'\n';
    }
}
#endif
