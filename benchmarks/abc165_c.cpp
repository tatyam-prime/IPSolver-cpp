#define IP_ABC165_ORACLES_ONLY
#include "../tests/test_abc165_c.cpp"
#include <fstream>

const char* status_name(ip::Status s) {
	switch (s) {
		case ip::Status::Optimal:return "Optimal";
		case ip::Status::Infeasible:return "Infeasible";
		case ip::Status::UnboundedRelaxation:return "UnboundedRelaxation";
		case ip::Status::Limit:return "Limit";
		default:return "NumericalError";
	}
}
int main(int argc,char** argv) {
	int cases=argc>1?std::stoi(argv[1]):6;
	std::mt19937 rng(1652026104);
	std::vector<std::pair<std::string,std::vector<Requirement>>> inputs;
	for (int tc=0;tc<cases;++tc) {
		std::vector<Requirement> q;
		for (int a=0;a<10;++a) for (int b=a+1;b<10;++b) for (int c=0;c<10;++c)
			q.push_back({a,b,c,tc%3==0?100000:1+int(rng()%100000)});
		std::shuffle(q.begin(),q.end(),rng); q.resize(50);
		inputs.push_back({"random"+std::to_string(tc),std::move(q)});
	}
	for (std::string name:{"pair_numeric","clique_numeric"}) {
		std::ifstream f("tests/data/abc165_c_"+name+".in");
		int n,m,k; f>>n>>m>>k;
		if (!f || n!=10 || m!=10) throw std::runtime_error("missing fixture");
		std::vector<Requirement> q(k);
		for (auto& r:q) { f>>r.a>>r.b>>r.c>>r.d; --r.a; --r.b; }
		inputs.push_back({name,std::move(q)});
	}
	std::cout<<"case,mode,status,variables,rows,lp_solves,pivots,seconds,objective,bound,oracle,certificate\n";
	bool error=false;
	for (const auto& input:inputs) {
		auto expected=sequence_oracle(10,10,input.second);
		for (std::string mode:{"default","no_hint","no_cuts","no_strong","integer_gaps", "no_mutex","conflict_cliques","dense_conflicts"}) {
			ip::Options o; o.time_limit=2;
			if (mode=="no_cuts") o.cuts=0;
			if (mode=="no_strong") o.strong_branching=0;
			int conflict=mode=="dense_conflicts"?2:mode=="conflict_cliques"?1:0;
			auto start=std::chrono::steady_clock::now();
			auto r=many_requirements(10,10,input.second,o,mode!="no_hint",mode!="integer_gaps",mode!="no_mutex",conflict);
			double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
			bool proved=r.result.status==ip::Status::Optimal;
			if ((proved && r.score!=expected) || r.result.status==ip::Status::Infeasible) error=true;
			std::cout<<input.first<<','<<mode<<','<<status_name(r.result.status)<<','<<r.variables<<','<<r.rows<<','
					 <<r.result.nodes<<','<<r.result.pivots<<','<<elapsed<<',';
			if (r.result.has_solution()) std::cout<<r.score;
			std::cout<<','<<r.result.bound<<','<<expected<<','<<(proved&&r.score==expected?"exact":"unproved")<<'\n'<<std::flush;
		}
	}
	return error?1:0;
}
