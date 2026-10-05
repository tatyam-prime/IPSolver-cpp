#include "../tests/models/abc187_f.hpp"
#include "../tests/test_abc187_f_oracles.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <random>
#include <string>

using Edges=abc187_oracle::Edges;
struct Case { std::string name; int n; Edges edges; };

const char* status_name(ip::Status s) {
	switch (s) {
		case ip::Status::Optimal: return "Optimal";
		case ip::Status::Infeasible: return "Infeasible";
		case ip::Status::UnboundedRelaxation: return "UnboundedRelaxation";
		case ip::Status::Limit: return "Limit";
		case ip::Status::NumericalError: return "NumericalError";
	}
	return "Unknown";
}

int main(int argc,char** argv) {
	int random_cases=argc>1?std::stoi(argv[1]):100;
	std::vector<Case> cases;
	auto add_graph=[&](std::string name,int n,auto adjacent) {
		Edges edges;
		for (int i=0;i<n;++i) for (int j=i+1;j<n;++j) if (adjacent(i,j)) edges.push_back({i,j});
		cases.push_back({std::move(name),n,std::move(edges)});
	};
	add_graph("empty18",18,[](int,int){ return false; });
	add_graph("complete18",18,[](int,int){ return true; });
	add_graph("multipartite729",18,[](int i,int j){ return i/3!=j/3; });
	add_graph("cycle17",17,[](int i,int j){ return j==i+1 || (i==0 && j==16); });
	add_graph("complement_cycle17",17,[](int i,int j){ return j!=i+1 && !(i==0 && j==16); });
	add_graph("cycle5",5,[](int i,int j){ return j==i+1 || (i==0 && j==4); });
	std::vector<std::vector<bool>> conflict(18,std::vector<bool>(18));
	auto edge=[&](int u,int v) { conflict[u][v]=conflict[v][u]=true; };
	for (int i=0;i<5;++i) {
		int j=(i+1)%5; edge(i,j); edge(i,5+j); edge(j,5+i); edge(10,5+i);
	}
	add_graph("mycielski11_plus_isolates",18,[&](int i,int j){ return !conflict[i][j]; });
	for (int i=11;i<18;++i) for (int j=0;j<i;++j) edge(i,j);
	add_graph("mycielski11_join7",18,[&](int i,int j){ return !conflict[i][j]; });
	std::mt19937_64 rng(1871020261004ULL);
	for (int t=0;t<random_cases;++t) {
		int density=5+5*(t%19);
		add_graph("random"+std::to_string(t)+"_p"+std::to_string(density),18,
				  [&](int,int){ return int(rng()%100)<density; });
	}
	std::cout<<"case,n,edges,patterns,rows,setting,status,groups,expected,objective,bound,lp_solves,pivots,ms\n";
	bool incorrect=false; uint64_t most_pivots=0;
	for (const auto& instance:cases) {
		int expected=abc187_oracle::coloring(instance.n,instance.edges);
		for (int setting=0;setting<6;++setting) {
			const char* names[]={"default","no_hint","no_gmi","no_strong","plain","pivot_budget1"};
			ip::Options options; options.time_limit=2.5;
			bool hint=setting!=1 && setting!=4;
			if (setting==2 || setting==4) options.cuts=0;
			if (setting==3 || setting==4) options.strong_branching=0;
			if (setting==5) options.pivot_limit=1;
			auto start=std::chrono::steady_clock::now();
			auto answer=clique_cover(instance.n,instance.edges,options,hint);
			double ms=1000*std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
			const auto& r=answer.result;
			bool wrong=r.status==ip::Status::Optimal && answer.groups!=expected;
			incorrect|=wrong;
			std::cout<<instance.name<<','<<instance.n<<','<<instance.edges.size()<<','
					 <<answer.variables<<','<<answer.rows<<','<<names[setting]<<','
					 <<(wrong?"IncorrectOptimal":status_name(r.status))<<','<<answer.groups<<','
					 <<expected<<','<<std::setprecision(12)<<r.objective<<','<<r.bound<<','
					 <<r.nodes<<','<<r.pivots<<','<<ms<<'\n';
			if (setting==0 && r.pivots>most_pivots) {
				most_pivots=r.pivots;
				if (argc>2) {
					std::ofstream file(argv[2]);
					file<<instance.n<<' '<<instance.edges.size()<<'\n';
					for (auto [u,v]:instance.edges) file<<u+1<<' '<<v+1<<'\n';
				}
			}
		}
	}
	return incorrect?1:0;
}
