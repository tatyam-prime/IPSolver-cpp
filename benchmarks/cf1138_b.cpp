#define IP_CIRCUS_HELPERS_ONLY
#include "../tests/test_cf1138_b.cpp"

CircusAnswer circus_raw(const string& c,const string& a,ip::Options o) {
    int n=int(c.size()),total=0; ip::Solver s(ip::Vec(n,0)); ip::Vec value(n);
    for (int i=0;i<n;++i) { s.bounds(i,0,1); value[i]=c[i]+a[i]-2*'0'; total+=a[i]-'0'; }
    s.add_eq(ip::Vec(n,1),n/2); s.add_eq(value,total);
    auto r=s.solve(o); vector<int> first;
    if (r.has_solution()) for (int i=0;i<n;++i) if (r.x[i]>.5) first.push_back(i);
    return {std::move(first),std::move(r)};
}
const char* circus_status(ip::Status s) {
    switch(s) { case ip::Status::Optimal:return "Optimal"; case ip::Status::Infeasible:return "Infeasible";
    case ip::Status::Limit:return "Limit"; case ip::Status::NumericalError:return "NumericalError";
    default:return "UnboundedRelaxation"; }
}
int main() {
    mt19937 rng(11382026); cout<<"n,family,mode,status,variables,lp_solves,pivots,seconds,verified\n";
    for (int n:{2,10,50,100,500,5000}) for (string family:{"random","zero","one","parity","impossible"}) {
        string c(n,'0'),a(n,'0');
        for (int i=0;i<n;++i) {
            if (family=="random") { c[i]+=rng()%2; a[i]+=rng()%2; }
            if (family=="one") c[i]=a[i]='1';
            if (family=="parity") c[i]=a[i]=i<n/2+(n%4==0)?'1':'0';
            if (family=="impossible") a[i]='1';
        }
        bool expected=circus_count_oracle(c,a);
        for (string mode:{"aggregate","raw"}) {
            if (n>500 && mode=="raw") continue;
            ip::Options o; o.time_limit=2;
            auto start=chrono::steady_clock::now(); auto result=mode=="aggregate"?circus(c,a,o):circus_raw(c,a,o);
            double elapsed=chrono::duration<double>(chrono::steady_clock::now()-start).count();
            bool proved=result.result.status==ip::Status::Optimal || result.result.status==ip::Status::Infeasible;
            if (proved) circus_validate(c,a,result,expected);
            cout<<n<<','<<family<<','<<mode<<','<<circus_status(result.result.status)<<','<<(mode=="aggregate"?3:n)<<','
                <<result.result.nodes<<','<<result.result.pivots<<','<<elapsed<<','<<proved<<'\n'<<flush;
        }
    }
}
