#define IP_EXAMPLE_TEST
#ifdef IP_TEST_STANDALONE
#include "../build/submissions/codeforces/cf1082_g.cpp"
#else
#include "../examples/codeforces/cf1082_g.cpp"
#endif
#include "test_cf1082_g_oracles.hpp"
#include <chrono>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>

void profit_check(bool ok,const char* message) { if (!ok) throw runtime_error(message); }
cf1082_oracle::Edges oracle_edges(const vector<ProfitEdge>& edges) {
	cf1082_oracle::Edges result;
	for (auto [u,v,w]:edges) result.push_back({u,v,w});
	return result;
}

int main(int argc,char** argv) {
	if (argc>1 && string(argv[1])=="--oracle") {
		int n,m;
		while (cin>>n>>m) {
			vector<ll> cost(n); for (ll& c:cost) cin>>c;
			cf1082_oracle::Edges edges(m);
			for (auto& [u,v,w]:edges) { cin>>u>>v>>w; --u; --v; }
			cout<<cf1082_oracle::flow(cost,edges)<<'\n';
		}
		return 0;
	}
	int small=argc>1?stoi(argv[1]):3000,stress=argc>2?stoi(argv[2]):100;
	double seconds=argc>3?stod(argv[3]):1.8;
	mt19937_64 rng(108220261004ULL);
	int checks=0; double worst=0; uint64_t max_nodes=0,max_pivots=0;
	auto verify=[&](const vector<ll>& cost,const vector<ProfitEdge>& edges,bool reduced=true) {
		auto original=oracle_edges(edges); ll expected=cf1082_oracle::flow(cost,original);
		if (cost.size()<=12) profit_check(cf1082_oracle::brute(cost,original)==expected,"flow vs brute");
		ip::Options options; options.time_limit=seconds;
		auto start=chrono::steady_clock::now(); auto answer=graph_profit(cost,edges,options,reduced,checks%2);
		worst=max(worst,1000*chrono::duration<double>(chrono::steady_clock::now()-start).count());
		max_nodes=max(max_nodes,answer.result.nodes); max_pivots=max(max_pivots,answer.result.pivots);
		if (answer.result.status!=ip::Status::Optimal || answer.weight!=expected) {
			ofstream file("tests/test_cf1082_g_failure.in");
			file<<cost.size()<<' '<<edges.size()<<'\n'; for (ll c:cost) file<<c<<' '; file<<'\n';
			for (auto [u,v,w]:edges) file<<u+1<<' '<<v+1<<' '<<w<<'\n';
			cerr<<"expected="<<expected<<" value="<<answer.weight<<" status="<<int(answer.result.status)
				<<" variables="<<answer.variables<<" rows="<<answer.rows<<" LP="<<answer.result.nodes
				<<" pivots="<<answer.result.pivots<<'\n';
			throw runtime_error("CF1082 G mismatch; input saved");
		}
		++checks;
	};
	verify({1,5,2,2},{{0,2,4},{0,3,4},{2,3,5},{2,1,2},{3,1,2}});
	verify({9,7,8},{{0,1,1},{1,2,2},{0,2,3}});
	// Resource limits are shared by disconnected components.
	vector<ll> budget_cost(8,3); vector<ProfitEdge> budget_edges;
	for (int start:{0,4}) for (int i=0;i<4;++i) for (int j=i+1;j<4;++j)
		budget_edges.push_back({start+i,start+j,2});
	auto budget_probe=[&](ip::Options options,ip::Status expected) {
		auto answer=graph_profit(budget_cost,budget_edges,options);
		profit_check(answer.result.status==expected,"shared budget status");
		profit_check(answer.result.nodes<=options.node_limit && answer.result.pivots<=options.pivot_limit,
					 "shared budget exceeded"); ++checks;
	};
	ip::Options budget;
	budget.node_limit=1; budget_probe(budget,ip::Status::Limit);
	budget.node_limit=2; budget_probe(budget,ip::Status::Optimal);
	budget.node_limit=0; budget_probe(budget,ip::Status::Limit);
	budget=ip::Options{}; budget.time_limit=0; budget_probe(budget,ip::Status::Limit);
	vector<ll> one_cost(4,3); vector<ProfitEdge> one_edges;
	for (int i=0;i<4;++i) for (int j=i+1;j<4;++j) one_edges.push_back({i,j,2});
	budget=ip::Options{}; budget.pivot_limit=graph_profit(one_cost,one_edges).result.pivots;
	budget_probe(budget,ip::Status::Limit);
	// Degree-2 elimination creates and merges a parallel edge on each triangle.
	for (ll a=1;a<=4;++a) for (ll b=1;b<=4;++b) for (ll c=1;c<=8;++c) {
		vector<ProfitEdge> edges={{0,1,a},{0,2,b},{1,2,3}};
		verify({c,3,4},edges); verify({c,3,4},edges,false);
	}
	for (int t=0;t<small;++t) {
		int n=1+rng()%12; ll bound=t%3?10:1000000000;
		vector<ll> cost(n); for (ll& c:cost) c=1+rng()%bound;
		vector<ProfitEdge> edges;
		int density=5+rng()%91;
		for (int u=0;u<n;++u) for (int v=u+1;v<n;++v)
			if (int(rng()%100)<density) edges.push_back({u,v,ll(1+rng()%bound)});
		verify(cost,edges); verify(cost,edges,false);
	}
	for (int t=0;t<stress;++t) {
		int n=1000; vector<ll> cost(n); for (ll& c:cost) c=1+rng()%1000000000;
		vector<ProfitEdge> edges; std::map<pair<int,int>,bool> used;
		while (edges.size()<1000) {
			int u=rng()%n,v=rng()%n; if (u>v) swap(u,v);
			if (u!=v && used.emplace(make_pair(u,v),true).second)
				edges.push_back({u,v,ll(1+rng()%1000000000)});
		}
		verify(cost,edges);
	}
	// Degree-2 elimination merges 498 edges into one edge of weight 499e9.
	vector<ll> large_cost(500,1000000000); vector<ProfitEdge> large_edges={{0,1,1000000000}};
	for (int v=2;v<500;++v) {
		large_edges.push_back({0,v,1000000000}); large_edges.push_back({1,v,1000000000});
	}
	verify(large_cost,large_edges);
	// Largest 3-regular core, then the exact 1000-edge maximum model size.
	for (int t=0;t<7;++t) {
		int k=666; vector<ll> cost(1000,1000000000);
		vector<ProfitEdge> edges;
		for (int i=0;i<k;++i) edges.push_back({i,(i+1)%k,666666666});
		for (int i=0;i<k/2;++i) edges.push_back({i,i+k/2,666666666});
		for (int i=0;i<k;++i) cost[i]=999999999+(t<3?t-1:t==6?0:ll(rng()%3)-1);
		if (t==6) edges.push_back({0,2,1});
		verify(cost,edges);
	}
	cout<<"CF1082 G: "<<checks<<" cases; worst call "<<worst<<"ms; max LP="
		<<max_nodes<<" pivots="<<max_pivots<<'\n';
}
