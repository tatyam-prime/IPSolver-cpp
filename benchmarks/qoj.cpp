#define IP_ORACLES_ONLY
#include "../tests/test_qoj.cpp"

const char* status(ip::Status s) {
    switch (s) {
        case ip::Status::Optimal:return "Optimal";
        case ip::Status::Infeasible:return "Infeasible";
        case ip::Status::UnboundedRelaxation:return "UnboundedRelaxation";
        case ip::Status::Limit:return "Limit";
        default:return "NumericalError";
    }
}
int benchmark_errors=0;
template<class F> void record(const string& name,const string& mode,F solve,int expected=numeric_limits<int>::min()) {
    auto t=chrono::steady_clock::now(); auto a=solve(); double elapsed=seconds(t);
    if (expected!=numeric_limits<int>::min() && a.result.status==ip::Status::Optimal)
        check(int(llround(a.result.objective))==expected,"benchmark certificate "+name);
    if (expected!=numeric_limits<int>::min() && a.result.status==ip::Status::Infeasible) ++benchmark_errors;
    cout<<name<<','<<mode<<','<<status(a.result.status)<<','<<a.variables<<','<<a.rows<<','
        <<a.result.nodes<<','<<a.result.pivots<<','<<elapsed<<',';
    if (a.result.has_solution()) cout<<a.result.objective;
    cout<<',';
    if (expected!=numeric_limits<int>::min())
        cout<<(a.result.status==ip::Status::Optimal?"exact":a.result.status==ip::Status::Infeasible?"incorrect_infeasible":"unproved");
    else cout<<"solver_only";
    cout<<'\n'<<flush;
}
MineAnswer mine_direct(const Grid& a,ip::Options o) {
    int r=int(a.size()),c=int(a[0].size()),n=r*c;
    ip::Vec objective(n); for (int j=0;j<c;++j) objective[(r/2)*c+j]=1;
    ip::Solver s(objective); for (int k=0;k<n;++k) s.bounds(k,0,1);
    for (int i=0;i<r;++i) for (int j=0;j<c;++j) {
        ip::Vec row(n);
        for (int u=max(0,i-1);u<=min(r-1,i+1);++u)
            for (int v=max(0,j-1);v<=min(c-1,j+1);++v) row[u*c+v]=1;
        s.add_eq(row,a[i][j]);
    }
    auto result=s.maximize(o);
    return {result.has_solution()?int(llround(result.objective)):-1,n,2*n,{},std::move(result)};
}
#ifndef IP_BENCH_ONLY
int main(int argc,char** argv) {
    string suite=argc>1?argv[1]:"all"; int batches=argc>2?stoi(argv[2]):19;
    mt19937 rng(20261004);
    cout<<"case,mode,status,variables,rows,lp_solves,pivots,seconds,objective,certificate\n";
    vector<string> modes={"default","no_hint","no_cuts","no_strong"};
#ifdef IP_NO_INITIAL
    modes={"no_hint","no_cuts"};
#endif
    if (suite=="all" || suite=="packing") {
        for (int b=0;b<batches;++b) {
            vector<vector<int>> items(100,vector<int>(1000));
            for (auto& a:items) for (int& x:a) x=1+rng()%12;
            for (auto mode:modes) {
                uint64_t nodes=0,pivots=0; int certified=0; bool optimal=true;
                auto t=chrono::steady_clock::now();
                for (const auto& a:items) {
                    ip::Options o; o.time_limit=2;
                    if (mode=="no_cuts") o.cuts=0;
                    if (mode=="no_strong") o.strong_branching=0;
                    auto answer=pack12(a,o,mode!="no_hint");
                    optimal&=answer.result.status==ip::Status::Optimal;
                    nodes+=answer.result.nodes; pivots+=answer.result.pivots;
                    int lb=packing_lower(a); check(answer.bins>=lb,"packing lower certificate");
                    certified+=answer.bins==lb;
                }
                cout<<"packing100_"<<b<<','<<mode<<','<<(optimal?"Optimal":"Unproved")
                    <<",77,14,"<<nodes<<','<<pivots<<','<<seconds(t)<<",,"<<certified<<"/100_lower_bound\n"<<flush;
            }
        }
    }
    if (suite=="all" || suite=="cover") {
        vector<pair<string,vector<pair<int,int>>>> cases;
        vector<pair<int,int>> triangle,clique;
        for (int i=0;i<30;++i) for (int j=i+1;j<30;++j) {
            clique.push_back({i,j}); if (i/3==j/3) triangle.push_back({i,j});
        }
        cases.push_back({"cover_triangles",triangle}); cases.push_back({"cover_clique",clique});
        for (int density:{10,30,50,70,90}) {
            vector<pair<int,int>> e;
            for (int i=0;i<30;++i) for (int j=i+1;j<30;++j) if (int(rng()%100)<density) e.push_back({i,j});
            cases.push_back({"cover_core30_d"+to_string(density),e});
        }
        for (int density:{1,2,5,10,50}) {
            vector<pair<int,int>> e;
            for (int i=0;i<30;++i) for (int j=i+1;j<500;++j) if (int(rng()%100)<density) e.push_back({i,j});
            cases.push_back({"cover500_d"+to_string(density),e});
        }
        // A 30-core bipartite instance without leaves or forced high degrees.
        vector<pair<int,int>> cycle;
        for (int i=0;i<30;++i) { cycle.push_back({i,30+i}); cycle.push_back({(i+1)%30,30+i}); }
        cases.push_back({"cover500_cycle",cycle});
        for (const auto& item:cases) for (const auto& mode:modes) {
            const auto& name=item.first; const auto& e=item.second;
            ip::Options o; o.time_limit=2;
            if (mode=="no_cuts") o.cuts=0;
            if (mode=="no_strong") o.strong_branching=0;
            int expected=numeric_limits<int>::min();
            if (name.find("core30")!=string::npos || name=="cover_clique" || name=="cover_triangles") expected=-cover_mitm(30,e);
            if (name=="cover500_cycle") expected=-30;
            record(name,mode,[&] { return vertex_cover(500,e,o,mode!="no_hint"); },expected);
        }
        for (const auto& item:cases) if (item.first.find("core30")!=string::npos || item.first=="cover_clique") {
            const auto& name=item.first; const auto& e=item.second;
            ip::Options o; o.time_limit=2;
            record(name,"no_reduction",[&] { return vertex_cover(500,e,o,true,false); },-cover_mitm(30,e));
        }
    }
    if (suite=="all" || suite=="mine") {
        for (int r:{5,11,17,29,47,49}) for (string family:{"random","periodic","zero","one"}) {
            Grid x(r,vector<int>(r));
            for (int i=0;i<r;++i) for (int j=0;j<r;++j)
                x[i][j]=family=="random"?rng()%2:family=="periodic"?(i+j)%2:family=="one";
            auto a=clues(x); int expected=accumulate(x[r/2].begin(),x[r/2].end(),0);
            string name="mine"+to_string(r)+"_"+family;
            ip::Options o; o.time_limit=2; o.cuts=0;
            record(name,"reduced",[&] { return mine_layer(a,o); },expected);
            if (r<=29) record(name,"direct",[&] { return mine_direct(a,o); },expected);
        }
    }
    return benchmark_errors?1:0;
}
#endif
