#include <climits>
#define main cf417_d_submission_main
#ifdef IP_TEST_STANDALONE
#include "../build/submissions/codeforces/cf417_d.cpp"
#else
#include "../examples/codeforces/cf417_d.cpp"
#endif
#undef main
#include <chrono>
#include <cstdlib>
#include <random>

ll subset_oracle(int m, ll b, const std::vector<Friend>& a) {
    ll best=LLONG_MAX;
    for (int subset=0; subset<(1<<int(a.size())); ++subset) {
        int mask=0;
        ll cost=0, monitors=0;
        for (int i=0; i<int(a.size()); ++i) if (subset>>i&1) {
            mask|=a[i].mask;
            cost+=a[i].cost;
            monitors=std::max(monitors, a[i].monitors);
        }
        if (mask==(1<<m)-1) best=std::min(best, cost+monitors*b);
    }
    return best==LLONG_MAX ? -1 : best;
}

ll dp_oracle(int m, ll b, std::vector<Friend> a) {
    std::sort(a.begin(), a.end(), [](const Friend& x, const Friend& y) {
        return x.monitors<y.monitors;
    });
    const ll inf=LLONG_MAX/4;
    std::vector<ll> dp(1<<m, inf);
    dp[0]=0;
    ll best=inf;
    for (const auto& x:a) {
        for (int mask=(1<<m)-1; mask>=0; --mask) if (dp[mask]!=inf)
            dp[mask|x.mask]=std::min(dp[mask|x.mask], dp[mask]+x.cost);
        if (dp.back()!=inf) best=std::min(best, dp.back()+x.monitors*b);
    }
    return best==inf ? -1 : best;
}

int main(int argc, char** argv) {
    if (argc>1 && std::string(argv[1])=="--gmi-probe") {
        int n,m;
        ll b;
        std::cin>>n>>m>>b;
        std::vector<unsigned> masks(n);
        for (int i=0; i<n; ++i) {
            ll cost,monitors;
            int count;
            std::cin>>cost>>monitors>>count;
            while (count--) { int p; std::cin>>p; masks[i]|=1u<<(p-1); }
        }
        ip::Solver s(ip::Vec(n,-1));
        for (int i=0; i<n; ++i) s.bounds(i,0,1);
        for (int p=0; p<m; ++p) {
            ip::Vec row(n);
            for (int i=0; i<n; ++i) row[i]=(masks[i]>>p)&1;
            s.add_ge(row,1);
        }
        ip::Options options;
        options.cuts=argc>2 ? std::atoi(argv[2]) : 8;
        options.time_limit=1;
        auto r=s.maximize(options);
        std::cout<<"cuts="<<options.cuts<<" status="<<int(r.status)
                 <<" objective="<<r.objective<<" nodes="<<r.nodes<<" pivots="<<r.pivots<<'\n';
        return 0;
    }
    int random_cases=argc>1 ? std::atoi(argv[1]) : 600;
    int stress_cases=argc>2 ? std::atoi(argv[2]) : 32;
    std::mt19937_64 rng(41720261004);
    int checks=0;
    double slowest=0;
    auto check=[&](int m, ll b, const std::vector<Friend>& a, ll expected,
                   const std::string& label) {
        auto start=std::chrono::steady_clock::now();
        ll got;
        try { got=cunning_gena(m,b,a); }
        catch (const std::exception& e) {
            std::cerr<<label<<": "<<e.what()<<'\n';
            std::cerr<<a.size()<<' '<<m<<' '<<b<<'\n';
            for (const auto& x:a) {
                std::cerr<<x.cost<<' '<<x.monitors<<' '<<__builtin_popcount(unsigned(x.mask))<<'\n';
                for (int p=0; p<m; ++p) if (x.mask>>p&1) std::cerr<<p+1<<' ';
                std::cerr<<'\n';
            }
            std::exit(1);
        }
        double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        slowest=std::max(slowest,elapsed);
        if (got!=expected) {
            std::cerr<<label<<": expected "<<expected<<", got "<<got<<'\n';
            std::cerr<<a.size()<<' '<<m<<' '<<b<<'\n';
            for (const auto& x:a) {
                std::cerr<<x.cost<<' '<<x.monitors<<' '<<__builtin_popcount(unsigned(x.mask))<<'\n';
                for (int p=0; p<m; ++p) if (x.mask>>p&1) std::cerr<<p+1<<' ';
                std::cerr<<'\n';
            }
            std::exit(1);
        }
        ++checks;
        if (label.find("stress")!=std::string::npos)
            std::cout<<label<<": "<<elapsed*1000<<" ms\n";
    };
    check(2,1,{{100,1,2},{100,2,1}},202,"sample 1");
    check(2,5,{{100,1,1},{100,1,2},{200,1,3}},205,"sample 2");
    check(2,1,{{1,1,1}},-1,"sample 3");
    check(1,1000000000,{{1,1000000000,1}},1000000000000000001LL,"precision > 2^53");
    check(2,1000000000,{{1000000000,999999999,1},{1000000000,999999999,2},
                             {1,1000000000,3}},1000000000000000001LL,"precision monitor tradeoff");
    check(3,1,{{1,1,1},{1,1,2},{1,1,4},{1,1000000000,7}},4,"expensive monitor excluded");
    check(3,1,{{3,1,7},{2,1,3},{2,1,4},{3,1,7}},4,"duplicate domination");
    for (int tc=0; tc<random_cases; ++tc) {
        int n=1+int(rng()%12), m=1+int(rng()%8);
        ll b=tc%4 ? 1+ll(rng()%30) : 1+ll(rng()%1000000000);
        std::vector<Friend> a(n);
        for (auto& x:a) {
            x.cost=tc%3 ? 1+ll(rng()%100) : 1000000000-ll(rng()%100);
            x.monitors=tc%4 ? 1+ll(rng()%20) : 1000000000-ll(rng()%100);
            x.mask=1+int(rng()%((1<<m)-1));
        }
        check(m,b,a,subset_oracle(m,b,a),"random "+std::to_string(tc));
    }
    for (int tc=0; tc<stress_cases; ++tc) {
        int n=100,m=20;
        ll b=tc%8<4 ? 1 : 1000000000;
        std::vector<Friend> a(n);
        for (auto& x:a) {
            x.cost=tc%4==0 ? 1 : tc%4==1 ? 1000000000-ll(rng()%100) : 1+ll(rng()%1000000000);
            x.monitors=tc%3==0 ? 1000000000 : tc%3==1 ? 999999950+ll(rng()%51) : 1+ll(rng()%100);
            x.mask=0;
            int density=(tc%4+1)*15;
            for (int p=0; p<m; ++p) if (rng()%100<unsigned(density)) x.mask|=1<<p;
            if (!x.mask) x.mask=1<<int(rng()%m);
        }
        check(m,b,a,dp_oracle(m,b,a),"stress "+std::to_string(tc));
    }
    for (int tc=0; tc<8; ++tc) {
        int m=20;
        std::vector<Friend> a(100);
        for (auto& x:a) {
            x.cost=tc%2 ? 1000000000-ll(rng()%100) : 1;
            x.monitors=tc<4 ? 1000000000 : 999999900+ll(rng()%101);
            x.mask=0;
            int size=tc%4<2 ? 10 : 3;
            while (__builtin_popcount(unsigned(x.mask))<size) x.mask|=1<<int(rng()%m);
        }
        check(m,1,a,dp_oracle(m,1,a),"stress fixed size "+std::to_string(tc));
    }
    std::cout<<"cf417_d: "<<checks<<" checks passed; slowest "<<slowest*1000<<" ms\n";
}
