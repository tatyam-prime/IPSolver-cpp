#include "models/abc180_e.hpp"
#include <cstdlib>
#include <iomanip>
#include <random>
#include <set>
#include <string>

// Independent Held-Karp DP uses the original directed travel cost.
long long aerial_oracle(const std::vector<City>& p) {
	int n=int(p.size()), states=1<<(n-1);
	std::vector<std::vector<long long>> d(n,std::vector<long long>(n));
	for (int i=0; i<n; ++i) for (int j=0; j<n; ++j)
		d[i][j]=std::abs(p[i][0]-p[j][0])+std::abs(p[i][1]-p[j][1])+std::max(0,p[j][2]-p[i][2]);
	std::vector<long long> dp(size_t(states)*(n-1),INT64_MAX/4);
	for (int j=1; j<n; ++j) dp[size_t(1<<(j-1))*(n-1)+j-1]=d[0][j];
	for (int mask=1; mask<states; ++mask) for (int last=1; last<n; ++last) if (mask>>(last-1)&1) {
		long long value=dp[size_t(mask)*(n-1)+last-1];
		for (int next=1; next<n; ++next) if (!(mask>>(next-1)&1)) {
			long long& out=dp[size_t(mask|(1<<(next-1)))*(n-1)+next-1];
			out=std::min(out,value+d[last][next]);
		}
	}
	long long answer=INT64_MAX;
	for (int j=1; j<n; ++j) answer=std::min(answer,dp[size_t(states-1)*(n-1)+j-1]+d[j][0]);
	return answer;
}

int main(int argc,char** argv) {
	if (argc>1 && std::string(argv[1])=="--oracle") {
		int cases; std::cin>>cases;
		while (cases--) {
			int n; std::cin>>n; std::vector<City> p(n);
			for (auto& city:p) for (int& x:city) std::cin>>x;
			std::cout<<aerial_oracle(p)<<'\n';
		}
		return 0;
	}
	int random_cases=argc>1 ? std::atoi(argv[1]) : 500;
	int max_cases=argc>2 ? std::atoi(argv[2]) : 40;
	std::mt19937 rng(1802026104);
	int checks=0;
	double worst=0;
	uint64_t max_nodes=0, max_pivots=0;
	int max_solves=0, max_cuts=0;
	auto check=[&](const std::vector<City>& p,const std::string& label,long long known=-1) {
		long long expected=aerial_oracle(p);
		if (known>=0 && known!=expected) throw std::runtime_error("sample oracle mismatch");
		ip::Options options; options.time_limit=1.8;
		auto before=std::chrono::steady_clock::now();
		auto got=aerial_cities(p,options);
		double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-before).count();
		if (got.result.status!=ip::Status::Optimal || got.cost!=expected) {
			std::cerr<<label<<": status "<<int(got.result.status)<<", cost "<<got.cost
					 <<", expected "<<expected<<", nodes "<<got.result.nodes
					 <<", pivots "<<got.result.pivots<<", calls "<<got.solves<<'\n';
			std::cerr<<p.size()<<'\n';
			for (const auto& city:p) std::cerr<<city[0]<<' '<<city[1]<<' '<<city[2]<<'\n';
			return false;
		}
		auto sorted=got.tour; std::sort(sorted.begin(),sorted.end());
		for (int i=0; i<int(p.size()); ++i) if (sorted[i]!=i) throw std::runtime_error("invalid tour");
		long long cost=0;
		for (int i=0; i<int(p.size()); ++i) {
			auto a=p[got.tour[i]], b=p[got.tour[(i+1)%p.size()]];
			cost+=std::abs(a[0]-b[0])+std::abs(a[1]-b[1])+std::max(0,b[2]-a[2]);
		}
		if (cost!=expected) throw std::runtime_error("reconstructed tour mismatch");
		worst=std::max(worst,elapsed); max_nodes=std::max(max_nodes,got.result.nodes);
		max_pivots=std::max(max_pivots,got.result.pivots);
		max_solves=std::max(max_solves,got.solves); max_cuts=std::max(max_cuts,got.subtour_cuts);
		++checks;
		if (p.size()==17 || label.find("structured")!=std::string::npos)
			std::cout<<label<<","<<p.size()<<","<<got.cost<<","<<elapsed<<","<<got.result.nodes
					 <<","<<got.result.pivots<<","<<got.solves<<","<<got.subtour_cuts<<'\n';
		return true;
	};
	std::cout<<"label,n,cost,seconds,nodes,pivots,solves,subtour_cuts\n";
	if (!check({{{0,0,0}},{{1,2,3}}},"sample 1",9) ||
		!check({{{0,0,0}},{{1,1,1}},{{-1,-1,-1}}},"sample 2",10) ||
		!check({{{14142,13562,373095}},{{-17320,508075,68877}},{{223606,-79774,9979}},
				{{-24494,-89742,783178}},{{26457,513110,-64591}},{{-282842,7124,-74619}},
				{{31622,-77660,-168379}},{{-33166,-24790,-3554}},{{346410,16151,37755}},
				{{-36055,51275,463989}},{{37416,-573867,73941}},{{-3872,-983346,207417}},
				{{412310,56256,-17661}},{{-42426,40687,-119285}},{{43588,-989435,-40674}},
				{{-447213,-59549,-99579}},{{45825,7569,45584}}},"sample 3",6519344)) return 1;
	for (int mode=0; mode<6; ++mode) {
		std::vector<City> p;
		for (int i=0; i<17; ++i) {
			if (mode==0) p.push_back({i*10000,i*20000,i*30000});
			if (mode==1) p.push_back({i*1000,0,0});
			if (mode==2) p.push_back({0,0,i*1000});
			if (mode==3) p.push_back({(i%5)*100000,(i/5)*100000,(i%2)*1000000});
			if (mode==4) p.push_back({(i/6)*900000-900000,(i%6)*3,(i%4)*5});
			if (mode==5) p.push_back({(i/3)*300000-900000,(i%3)*3,(i%3)*5});
		}
		if (!check(p,"structured "+std::to_string(mode))) return 1;
	}
	for (int tc=0; tc<random_cases+max_cases; ++tc) {
		int n=tc<random_cases ? 2+int(rng()%8) : 17;
		std::set<City> occupied;
		std::vector<City> p;
		while (int(p.size())<n) {
			int range=tc%3==0 ? 10 : tc%3==1 ? 1000 : 1000000;
			City city;
			for (int& x:city) x=int(rng()%unsigned(2*range+1))-range;
			if (occupied.insert(city).second) p.push_back(city);
		}
		if (!check(p,(tc<random_cases ? "random " : "maximum ")+std::to_string(tc))) return 1;
	}
	std::cerr<<"ABC180E: "<<checks<<" passed; max "<<worst<<" seconds, "<<max_nodes
			 <<" nodes, "<<max_pivots<<" pivots, "<<max_solves<<" solves, "<<max_cuts<<" subtour cuts\n";
}
