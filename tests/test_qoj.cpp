#define IP_EXAMPLE_TEST
#include "../examples/qoj/qoj10266.cpp"
#include "../examples/qoj/qoj3699.cpp"
#include "../examples/qoj/qoj5785.cpp"
#include <chrono>
#include <fstream>
#include <random>
#include <string>
using Grid=vector<vector<int>>;
void check(bool ok,const string& message) { if (!ok) throw runtime_error(message); }
int packing_dp(const vector<int>& a) {
    if (a.empty()) return 0;
    vector<pair<int,int>> dp(1<<a.size(),{100,0}); dp[0]={1,0};
    for (int mask=1;mask<int(dp.size());++mask) for (int j=0;j<int(a.size());++j) if (mask>>j&1) {
        auto p=dp[mask^(1<<j)];
        if (p.second+a[j]>12) { ++p.first; p.second=a[j]; } else p.second+=a[j];
        dp[mask]=min(dp[mask],p);
    }
    return dp.back().first;
}
int packing_lower(const vector<int>& a) {
    array<int,13> count{}; int volume=0, lower=0;
    for (int x:a) { ++count[x]; volume+=x; }
    for (int k=1;k<=12;++k) {
        int number=0; for (int j=k;j<=12;++j) number+=count[j];
        int per=12/k; lower=max(lower,(number+per-1)/per);
    }
    int waste=0;
    for (int k=7;k<=12;++k) {
        int capacity=0,supply=0;
        for (int j=k;j<=12;++j) capacity+=(12-j)*count[j];
        for (int j=1;j<=12-k;++j) supply+=j*count[j];
        waste=max(waste,capacity-supply);
    }
    return max(lower,(volume+waste+11)/12);
}
int cover_brute(int n,const vector<pair<int,int>>& e) {
    int best=n;
    for (unsigned mask=0;mask<(1u<<n);++mask) if (__builtin_popcount(mask)<best) {
        bool ok=true; for (auto [u,v]:e) if (!(mask>>u&1) && !(mask>>v&1)) { ok=false; break; }
        if (ok) best=__builtin_popcount(mask);
    }
    return best;
}
int cover_core_oracle(int n,const vector<pair<int,int>>& e,int k) {
    vector<unsigned> neighbors(n); vector<pair<int,int>> inner;
    for (auto [u,v]:e) { if (u>v) swap(u,v); if (v<k) inner.push_back({u,v}); else neighbors[v]|=1u<<u; }
    int best=k;
    for (unsigned mask=0;mask<(1u<<k);++mask) {
        int cost=__builtin_popcount(mask); if (cost>=best) continue;
        bool ok=true; for (auto [u,v]:inner) if (!(mask>>u&1) && !(mask>>v&1)) { ok=false; break; }
        if (!ok) continue;
        for (int v=k;v<n;++v) cost+=(neighbors[v]&~mask)!=0;
        best=min(best,cost);
    }
    return best;
}
// Independent meet-in-the-middle oracle for arbitrary graphs on <=30 vertices.
int cover_mitm(int n,const vector<pair<int,int>>& e) {
    int l=n/2,r=n-l; vector<unsigned> adj(n);
    for (auto [u,v]:e) { adj[u]|=1u<<v; adj[v]|=1u<<u; }
    vector<int> best(1<<l); vector<bool> valid(1<<r,true); vector<unsigned> banned(1<<r);
    for (unsigned mask=1;mask<best.size();++mask) {
        int j=__builtin_ctz(mask); unsigned t=mask^(1u<<j);
        best[mask]=max(best[t],1+best[t&~adj[j]]);
    }
    int answer=0;
    for (unsigned mask=0;mask<valid.size();++mask) {
        if (mask) {
            int j=__builtin_ctz(mask); unsigned t=mask^(1u<<j);
            valid[mask]=valid[t] && !(adj[l+j]>>(l)&t);
            banned[mask]=banned[t]|adj[l+j];
        }
        if (valid[mask]) answer=max(answer,__builtin_popcount(mask)+best[(~banned[mask])&((1u<<l)-1)]);
    }
    return n-answer;
}
int checked_pack(const vector<int>& a,ip::Options o={},bool hint=true) {
    auto result=pack12(a,o,hint); check(result.result.status==ip::Status::Optimal,"packing status");
    return result.bins;
}
int checked_cover(int n,const vector<pair<int,int>>& e,ip::Options o={},bool hint=true,bool reduce=true) {
    auto result=vertex_cover(n,e,o,hint,reduce); check(result.result.status==ip::Status::Optimal,"cover status");
    return result.cover;
}
Grid clues(const Grid& x) {
    int r=int(x.size()),c=int(x[0].size()); Grid a(r,vector<int>(c));
    for (int i=0;i<r;++i) for (int j=0;j<c;++j)
        for (int u=max(0,i-1);u<=min(r-1,i+1);++u)
            for (int v=max(0,j-1);v<=min(c-1,j+1);++v) a[i][j]+=x[u][v];
    return a;
}
// Row-mask DP directly enforces the original 3x3 clues, without elimination.
int mine_dp(const Grid& a) {
    int r=int(a.size()),c=int(a[0].size()),z=1<<c;
    map<pair<int,int>,int> dp;
    for (int b=0;b<z;++b) dp[{0,b}]=0;
    for (int i=0;i<r;++i) {
        map<pair<int,int>,int> next;
        for (auto [key,value]:dp) for (int d=0;d<(i+1<r?z:1);++d) {
            auto [b,x]=key; bool ok=true;
            for (int j=0;j<c;++j) {
                int mask=((1<<min(c,j+2))-1)^((1<<max(0,j-1))-1);
                int sum=__builtin_popcount(unsigned(b&mask))+__builtin_popcount(unsigned(x&mask))+__builtin_popcount(unsigned(d&mask));
                if (sum!=a[i][j]) { ok=false; break; }
            }
            if (ok) {
                int score=value+(i==r/2?__builtin_popcount(unsigned(x)):0);
                auto k=make_pair(x,d); auto it=next.find(k);
                if (it==next.end() || it->second<score) next[k]=score;
            }
        }
        dp=std::move(next);
    }
    int best=-1; for (auto [key,v]:dp) best=max(best,v); return best;
}
double seconds(chrono::steady_clock::time_point t) { return chrono::duration<double>(chrono::steady_clock::now()-t).count(); }
#ifndef IP_ORACLES_ONLY
int main(int argc,char** argv) {
    int iterations=argc>1?stoi(argv[1]):1000;
    mt19937 rng(102665785); int count=0,certified=0;
    double packing_worst=0,cover_worst=0,mine_worst=0;
    uint64_t packing_pivots=0,cover_pivots=0,mine_pivots=0;
    vector<vector<int>> sample={{8,2,2,3,9},{11,1,1},{9,8,2,3,4,2,2,12},{2,2,2,2,2,2,3,3,3,3},{5,2,8,4,1,11},{12,12,9,8,2,8,4,4},{5,5,6,6,6},{10,12,5,4,8,2,2,11},{5,1,5,7,9,4,4},{12,1,1,9,3,5}};
    int answers[]={2,2,4,2,3,5,3,5,4,3};
    for (int i=0;i<10;++i) { check(checked_pack(sample[i])==answers[i],"packing sample"); ++count; }
    for (int it=0;it<iterations;++it) {
        vector<int> a(1+rng()%15); for (int& x:a) x=1+rng()%12;
        ip::Options o; o.cuts=it%2?8:0; o.strong_branching=it%3?3:0;
        check(checked_pack(a,o,it%2)==packing_dp(a),"packing DP "+to_string(it)); ++count;
    }
    for (int it=0;it<100;++it) {
        vector<int> a(1000); for (int& x:a) x=1+rng()%12;
        auto t=chrono::steady_clock::now(); auto result=pack12(a);
        check(result.result.status==ip::Status::Optimal,"large packing status");
        packing_worst=max(packing_worst,seconds(t)); packing_pivots=max(packing_pivots,result.result.pivots);
        int lb=packing_lower(a); check(result.bins>=lb,"packing lower bound"); certified+=result.bins==lb; ++count;
    }
    check(checked_cover(3,{{0,1},{0,2}})==1,"cover sample 1"); ++count;
    check(checked_cover(6,{{0,1},{0,2},{0,3},{1,4},{1,5}})==2,"cover sample 2"); ++count;
    for (int it=0;it<iterations;++it) {
        int n=2+rng()%16; vector<pair<int,int>> e;
        int density=5+rng()%90;
        for (int i=0;i<n;++i) for (int j=i+1;j<n;++j) if (int(rng()%100)<density) e.push_back({i,j});
        ip::Options o; o.cuts=it%2?8:0; o.strong_branching=it%3?3:0;
        int expected=cover_brute(n,e);
        check(cover_mitm(n,e)==expected,"MITM oracle vs brute");
        check(checked_cover(n,e,o,it%2,it%3)==expected,"cover brute "+to_string(it)); ++count;
    }
    for (int it=0;it<100;++it) {
        int n=500,k=10; vector<pair<int,int>> e;
        for (int i=0;i<k;++i) for (int j=i+1;j<n;++j) if (rng()%100<unsigned(1+it%40)) e.push_back({i,j});
        auto t=chrono::steady_clock::now(); auto result=vertex_cover(n,e);
        check(result.result.status==ip::Status::Optimal,"large cover status");
        cover_worst=max(cover_worst,seconds(t)); cover_pivots=max(cover_pivots,result.result.pivots);
        check(result.cover==cover_core_oracle(n,e,k),"cover core oracle"); ++count;
    }
    for (int it=0;it<100;++it) {
        vector<pair<int,int>> e; int density=5+it%90;
        for (int i=0;i<30;++i) for (int j=i+1;j<30;++j) if (int(rng()%100)<density) e.push_back({i,j});
        ip::Options o; o.time_limit=2;
        auto t=chrono::steady_clock::now(); auto result=vertex_cover(500,e,o);
        cover_worst=max(cover_worst,seconds(t)); cover_pivots=max(cover_pivots,result.result.pivots);
        check(result.result.status==ip::Status::Optimal && result.cover==cover_mitm(30,e),"30-core MITM"); ++count;
    }
    Grid sample1={{2,2,1},{3,4,3},{2,3,2}}, sample2={{1,2,1,1},{2,3,3,2},{2,2,2,1}};
    check(mine_layer(sample1).mines==1,"mine sample 1"); check(mine_layer(sample2).mines==1,"mine sample 2"); count+=2;
    for (int it=0;it<iterations;++it) {
        int r=3+2*(rng()%4),c=3+rng()%3; Grid x(r,vector<int>(c));
        for (auto& row:x) for (int& v:row) v=rng()%2;
        auto a=clues(x); auto result=mine_layer(a);
        check(result.result.status==ip::Status::Optimal,"mine status");
        check(clues(result.layout)==a,"mine reconstructed clues");
        for (auto& row:result.layout) for (int v:row) check(v==0 || v==1,"mine binary");
        check(result.mines==mine_dp(a),"mine row DP"); ++count;
    }
    for (int it=0;it<100;++it) {
        int r=it%2?49:47,c=it%3?49:47; Grid x(r,vector<int>(c));
        for (auto& row:x) for (int& v:row) v=it<4?it%2:rng()%2;
        auto a=clues(x); auto t=chrono::steady_clock::now(); auto result=mine_layer(a);
        check(result.result.status==ip::Status::Optimal,"large mine status");
        mine_worst=max(mine_worst,seconds(t)); mine_pivots=max(mine_pivots,result.result.pivots);
        check(clues(result.layout)==a,"large mine clues");
        for (auto& row:result.layout) for (int v:row) check(v==0 || v==1,"large mine binary");
        int expected=accumulate(x[r/2].begin(),x[r/2].end(),0);
        check(result.mines==expected,"large mine middle row invariant");
        check(result.result.nodes==1,"TU mine LP"); ++count;
    }
    cout<<"QOJ: "<<count<<" checks passed; large packing lower-bound certificates "<<certified<<"/100\n";
    cout<<"Worst total call ms / pivots: packing "<<1000*packing_worst<<" / "<<packing_pivots<<", cover "<<1000*cover_worst<<" / "<<cover_pivots<<", mine "<<1000*mine_worst<<" / "<<mine_pivots<<'\n';
    return 0;
}
#endif
