#define IP_EXAMPLE_TEST
#include "../examples/codeforces/cf1082_g.cpp"
#include "../tests/test_cf1082_g_oracles.hpp"
#include <fstream>
#include <iomanip>
#include <numeric>
#include <random>
#include <string>

struct Case { string name; vector<ll> cost; vector<ProfitEdge> edges; };
const char* status_name(ip::Status status) {
	switch (status) {
		case ip::Status::Optimal: return "Optimal";
		case ip::Status::Limit: return "Limit";
		case ip::Status::Infeasible: return "Infeasible";
		case ip::Status::UnboundedRelaxation: return "UnboundedRelaxation";
		case ip::Status::NumericalError: return "NumericalError";
	}
	return "Unknown";
}
int main(int argc,char** argv) {
	int random_cases=argc>1?stoi(argv[1]):10;
	vector<Case> cases;
	cases.push_back({"empty1000",vector<ll>(1000,1000000000),{}});
	for (string name:{"path1000","cycle1000","star1000"}) {
		Case c{name,vector<ll>(1000,1000000000),{}};
		for (int i=1;i<1000;++i) c.edges.push_back({name=="star1000"?0:i-1,i,1000000000});
		if (name=="cycle1000") c.edges.push_back({999,0,1000000000});
		cases.push_back(std::move(c));
	}
	Case joined{"parallel_weight499e9",vector<ll>(1000,1000000000),{{0,1,1000000000}}};
	for (int v=2;v<500;++v) { joined.edges.push_back({0,v,1000000000}); joined.edges.push_back({1,v,1000000000}); }
	cases.push_back(std::move(joined));
	mt19937_64 rng(108210042026ULL);
	for (int t=0;t<6;++t) {
		Case c{"core666_"+to_string(t),vector<ll>(1000,1000000000),{}};
		vector<int> permutation(666); iota(permutation.begin(),permutation.end(),0);
		if (t>=4) shuffle(permutation.begin(),permutation.end(),rng);
		for (int i=0;i<666;++i) c.edges.push_back({permutation[i],permutation[(i+1)%666],666666666});
		for (int i=0;i<333;++i) c.edges.push_back({permutation[i],permutation[i+333],666666666});
		for (int i=0;i<666;++i) c.cost[i]=999999999+(t<3?t-1:t==3?0:ll(rng()%3)-1);
		c.edges.push_back({permutation[0],permutation[2],1});
		cases.push_back(std::move(c));
	}
	Case disconnected{"two_core332",vector<ll>(1000,1000000000),{}};
	for (int start:{0,332}) {
		for (int i=0;i<332;++i) disconnected.edges.push_back({start+i,start+(i+1)%332,666666666});
		for (int i=0;i<166;++i) disconnected.edges.push_back({start+i,start+i+166,666666666});
		for (int i=0;i<332;++i) disconnected.cost[start+i]=999999999;
	}
	cases.push_back(std::move(disconnected));
	for (int t=0;t<random_cases;++t) {
		Case c{"random1000_"+to_string(t),vector<ll>(1000),{}};
		for (ll& cost:c.cost) cost=1+rng()%1000000000;
		map<pair<int,int>,bool> used;
		while (c.edges.size()<1000) {
			int u=rng()%1000,v=rng()%1000; if (u>v) swap(u,v);
			if (u!=v && used.emplace(make_pair(u,v),true).second) c.edges.push_back({u,v,ll(1+rng()%1000000000)});
		}
		cases.push_back(std::move(c));
	}
	cout<<"case,n,m,setting,status,profit,expected,variables,rows,components,lp_solves,pivots,ms\n";
	bool incorrect=false; double slowest=0;
	for (const auto& c:cases) {
		cf1082_oracle::Edges edges;
		for (auto [u,v,w]:c.edges) edges.push_back({u,v,w});
		ll expected=cf1082_oracle::flow(c.cost,edges);
		for (int setting=0;setting<4;++setting) {
			const char* names[]={"default","no_hint","no_reductions","no_reductions_no_hint"};
			ip::Options options; options.time_limit=1.8;
			auto start=chrono::steady_clock::now();
			auto answer=graph_profit(c.cost,c.edges,options,setting<2,setting%2==0);
			double ms=1000*chrono::duration<double>(chrono::steady_clock::now()-start).count();
			bool wrong=answer.result.status==ip::Status::Optimal && answer.weight!=expected;
			incorrect|=wrong;
			cout<<c.name<<','<<c.cost.size()<<','<<c.edges.size()<<','<<names[setting]<<','
				<<(wrong?"IncorrectOptimal":status_name(answer.result.status))<<','
				<<(answer.result.status==ip::Status::Optimal?answer.weight:-1)<<','<<expected<<','
				<<answer.variables<<','<<answer.rows<<','<<answer.components<<','<<answer.result.nodes<<','
				<<answer.result.pivots<<','<<setprecision(12)<<ms<<'\n';
			cout.flush();
			if (setting==0 && ms>slowest) {
				slowest=ms;
				if (argc>2) {
					ofstream file(argv[2]); file<<c.cost.size()<<' '<<c.edges.size()<<'\n';
					for (ll cost:c.cost) file<<cost<<' '; file<<'\n';
					for (auto [u,v,w]:c.edges) file<<u+1<<' '<<v+1<<' '<<w<<'\n';
				}
			}
		}
	}
	return incorrect?1:0;
}
