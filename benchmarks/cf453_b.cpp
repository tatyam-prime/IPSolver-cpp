#define IP_CF453_ORACLES_ONLY
#include "../tests/test_cf453_b.cpp"

const char* harmony_status(ip::Status s) {
    switch (s) {
        case ip::Status::Optimal:return "Optimal";
        case ip::Status::Infeasible:return "Infeasible";
        case ip::Status::Limit:return "Limit";
        case ip::Status::UnboundedRelaxation:return "UnboundedRelaxation";
        default:return "NumericalError";
    }
}
int main(int argc,char** argv) {
    int random_cases=argc>1?std::stoi(argv[1]):6;
    std::mt19937_64 rng(4532026104ULL);
    std::vector<std::pair<std::string,std::vector<int>>> inputs;
    inputs.push_back({"ones100",std::vector<int>(100,1)});
    inputs.push_back({"thirty100",std::vector<int>(100,30)});
    std::vector<int> balanced(100),high(100);
    for (int i=0;i<100;++i) balanced[i]=1+i%30,high[i]=20+i%11;
    inputs.push_back({"balanced100",balanced}); inputs.push_back({"high100",high});
    inputs.push_back({"mixed_factors6",{6,10,15,21,28,30}});
    for (int tc=0;tc<random_cases;++tc) {
        std::vector<int> a(tc%2?100:8+int(rng()%13));
        for (int& x:a) x=1+int(rng()%30);
        inputs.push_back({"random"+std::to_string(tc),std::move(a)});
    }
    std::cout<<"case,mode,status,variables,rows,lp_solves,pivots,seconds,cost,oracle,certificate\n";
    bool error=false;
    for (const auto& input:inputs) {
        int expected=harmony_dp(input.second);
        for (std::string mode:{"default","no_hint","no_cuts","no_strong","per_position",
                              "no_mask_compression","per_position_uncompressed"}) {
            ip::Options o; o.time_limit=3.5;
            if (mode=="no_cuts") o.cuts=0;
            if (mode=="no_strong") o.strong_branching=0;
            auto start=std::chrono::steady_clock::now();
            bool grouped=mode!="per_position" && mode!="per_position_uncompressed";
            bool compress=mode!="no_mask_compression" && mode!="per_position_uncompressed";
            auto r=harmony_chest(input.second,o,mode!="no_hint",grouped,compress);
            double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
            bool proved=r.result.status==ip::Status::Optimal && r.cost==expected && valid_harmony(r.b);
            if ((r.result.status==ip::Status::Optimal && !proved) || r.result.status==ip::Status::Infeasible) error=true;
            std::cout<<input.first<<','<<mode<<','<<harmony_status(r.result.status)<<','<<r.variables<<','<<r.rows<<','
                     <<r.result.nodes<<','<<r.result.pivots<<','<<elapsed<<',';
            if (r.result.has_solution()) std::cout<<r.cost;
            std::cout<<','<<expected<<','<<(proved?"exact":"unproved")<<'\n'<<std::flush;
        }
    }
    return error?1:0;
}
