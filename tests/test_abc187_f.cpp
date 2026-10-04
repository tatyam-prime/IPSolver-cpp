#define IP_EXAMPLE_TEST
#ifdef IP_TEST_STANDALONE
#include "../build/submissions/atcoder/abc187_f.cpp"
#else
#include "../examples/atcoder/abc187_f.cpp"
#endif
#include "test_abc187_f_oracles.hpp"
#include <chrono>
#include <fstream>
#include <random>
#include <set>
#include <stdexcept>
#include <string>

using abc187_oracle::Edges;

void check(bool ok,const char* message) {
	if (!ok) throw std::runtime_error(message);
}

int main(int argc,char** argv) {
	if (argc>1 && std::string(argv[1])=="--oracle") {
		int n,m;
		while (std::cin>>n>>m) {
			Edges edges(m);
			for (auto& [u,v]:edges) { std::cin>>u>>v; --u; --v; }
			std::cout<<abc187_oracle::coloring(n,edges)<<'\n';
		}
		return 0;
	}
	int small=argc>1?std::stoi(argv[1]):2000;
	int stress=argc>2?std::stoi(argv[2]):500;
	int exhaustive_n=argc>3?std::stoi(argv[3]):5;
	int cases=0,largest=0;
	double worst=0; uint64_t nodes=0,pivots=0;
	std::mt19937_64 rng(18720261004ULL);
	auto verify=[&](int n,const Edges& edges,ip::Options options={},bool hint=true,int expected=-1) {
		if (expected<0) expected=abc187_oracle::coloring(n,edges);
		if (n<=12) check(abc187_oracle::subset_dp(n,edges)==expected,"independent oracles disagree");
		auto start=std::chrono::steady_clock::now();
		auto answer=clique_cover(n,edges,options,hint);
		double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
		worst=std::max(worst,elapsed); nodes=std::max(nodes,answer.result.nodes);
		pivots=std::max(pivots,answer.result.pivots); largest=std::max(largest,answer.variables);
		if (answer.result.status!=ip::Status::Optimal || answer.groups!=expected) {
			std::ofstream file("tests/data/abc187_f_failure.in");
			file<<n<<' '<<edges.size()<<'\n';
			for (auto [u,v]:edges) file<<u+1<<' '<<v+1<<'\n';
			std::cerr<<"expected="<<expected<<" groups="<<answer.groups
					 <<" status="<<int(answer.result.status)<<" nodes="<<answer.result.nodes
					 <<" pivots="<<answer.result.pivots<<'\n';
			throw std::runtime_error("ABC187 F solver mismatch; saved failing graph");
		}
		std::vector<unsigned> adj(n);
		for (auto [u,v]:edges) { adj[u]|=1u<<v; adj[v]|=1u<<u; }
		std::set<uint32_t> unique;
		for (uint32_t mask:answer.patterns) {
			check(mask!=0 && unique.insert(mask).second,"duplicate or empty pattern");
			for (int v=0;v<n;++v) if (mask>>v&1u)
				check(!((mask^(1u<<v))&~adj[v]),"pattern is not a clique");
			else check((mask&~adj[v])!=0,"pattern is not inclusion-maximal");
		}
		if (n<=12) for (unsigned mask=1;mask<(1u<<n);++mask) {
			bool clique=true,maximal=true;
			for (int v=0;v<n;++v) if (mask>>v&1u) clique&=!((mask^(1u<<v))&~adj[v]);
			else maximal&=(mask&~adj[v])!=0;
			if (clique && maximal) check(unique.count(mask),"missing maximal clique");
		}
		unsigned covered=0; int groups=0;
		for (int j=0;j<answer.variables;++j) {
			int value=int(std::llround(answer.result.x[j]));
			check(value>=0,"negative pattern count"); groups+=value;
			if (value) covered|=answer.patterns[j];
		}
		check(covered==(1u<<n)-1 && groups==expected,"invalid covering certificate");
		check(std::llround(answer.result.objective)==-expected,"incorrect result objective");
		++cases;
	};
	verify(3,{{0,1},{0,2}},{},true,2);
	verify(4,{{0,1},{0,2},{0,3},{1,2},{1,3},{2,3}},{},true,1);
	verify(10,{{8,9},{1,9},{7,8},{2,3},{4,7},{0,7},{4,5},{1,4},{2,5},{5,8},{0,8}},{},true,5);
	verify(18,{}, {},true,18);
	for (int n=1;n<=exhaustive_n;++n) {
		Edges possible;
		for (int i=0;i<n;++i) for (int j=i+1;j<n;++j) possible.push_back({i,j});
		for (unsigned mask=0;mask<(1u<<possible.size());++mask) {
			Edges edges;
			for (int j=0;j<int(possible.size());++j) if (mask>>j&1u) edges.push_back(possible[j]);
			verify(n,edges);
		}
	}
	for (int t=0;t<small+stress;++t) {
		int n=t<small?1+rng()%12:18;
		int density=1+rng()%99; Edges edges;
		for (int i=0;i<n;++i) for (int j=i+1;j<n;++j)
			if (int(rng()%100)<density) edges.push_back({i,j});
		ip::Options options; options.time_limit=2.5;
		if (t<small) { options.cuts=t%2?8:0; options.strong_branching=t%3?3:0; }
		verify(n,edges,options,t%4!=0);
	}
	// Moon-Moser extremal graph: 3^6=729 inclusion-maximal cliques.
	Edges multipartite;
	for (int i=0;i<18;++i) for (int j=i+1;j<18;++j) if (i/3!=j/3) multipartite.push_back({i,j});
	verify(18,multipartite,{},true,3);
	// Odd-cycle complement: fractional covering optimum 5/2, integer optimum 3.
	Edges odd;
	for (int i=0;i<5;++i) odd.push_back({i,(i+1)%5});
	verify(5,odd,{},true,3);
	// Complement of the 11-vertex Mycielski lift of a five-cycle, plus a join K7.
	std::vector<std::vector<bool>> conflict(18,std::vector<bool>(18));
	auto add_conflict=[&](int u,int v) { conflict[u][v]=conflict[v][u]=true; };
	for (int i=0;i<5;++i) {
		int j=(i+1)%5; add_conflict(i,j); add_conflict(i,5+j);
		add_conflict(j,5+i); add_conflict(10,5+i);
	}
	for (int i=11;i<18;++i) for (int j=0;j<i;++j) add_conflict(i,j);
	Edges lifted;
	for (int i=0;i<18;++i) for (int j=i+1;j<18;++j) if (!conflict[i][j]) lifted.push_back({i,j});
	verify(18,lifted,{},true,11);
	std::cout<<"ABC187 F: "<<cases<<" cases; largest patterns="<<largest
			 <<"; worst call="<<worst*1000<<" ms; max nodes="<<nodes<<" pivots="<<pivots<<'\n';
}
