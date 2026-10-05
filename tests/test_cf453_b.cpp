#include "models/cf453_b.hpp"
#include <chrono>
#include <random>
#include <stdexcept>
#include <string>

// Original per-position DP: include all values 1..2*max(a)-1, without grouping
// positions, pruning equal prime masks, or using the solver's objective gains.
int harmony_dp(const std::vector<int>& a) {
	int largest=*std::max_element(a.begin(),a.end()),limit=2*largest-1;
	std::vector<int> primes,masks(limit+1);
	for (int p=2;p<=limit;++p) {
		bool prime=true;
		for (int d=2;d*d<=p;++d) if (p%d==0) prime=false;
		if (prime) primes.push_back(p);
	}
	for (int v=1;v<=limit;++v) for (int j=0;j<int(primes.size());++j)
		if (v%primes[j]==0) masks[v]|=1<<j;
	int size=1<<int(primes.size()),all=size-1,inf=1000000;
	std::vector<int> dp(size,inf),next(size);
	dp[0]=0;
	for (int x:a) {
		std::fill(next.begin(),next.end(),inf);
		for (int v=1;v<=limit;++v) {
			int free=all^masks[v],cost=std::abs(x-v);
			for (int mask=free;;mask=(mask-1)&free) {
				next[mask|masks[v]]=std::min(next[mask|masks[v]],dp[mask]+cost);
				if (!mask) break;
			}
		}
		dp.swap(next);
	}
	return *std::min_element(dp.begin(),dp.end());
}

bool valid_harmony(const std::vector<int>& b) {
	for (int i=0;i<int(b.size());++i) {
		if (b[i]<1) return false;
		for (int j=0;j<i;++j) if (std::gcd(b[i],b[j])!=1) return false;
	}
	return true;
}

#ifndef IP_CF453_ORACLES_ONLY
int main(int argc,char** argv) {
	if (argc>1 && std::string(argv[1])=="--oracle") {
		int n; std::cin>>n;
		std::vector<int> a(n); for (int& x:a) std::cin>>x;
		std::cout<<harmony_dp(a)<<'\n'; return 0;
	}
	int random_cases=argc>1?std::stoi(argv[1]):150;
	int stress_cases=argc>2?std::stoi(argv[2]):12;
	double time_limit=argc>3?std::stod(argv[3]):3.5;
	std::mt19937_64 rng(4532026104ULL);
	int checks=0; double worst=0;
	auto check=[&](const std::vector<int>& a,int expected,const std::string& label,
				   ip::Options options={},bool hint=true,bool grouped=true,bool compress=true) {
		options.time_limit=time_limit;
		auto start=std::chrono::steady_clock::now();
		auto r=harmony_chest(a,options,hint,grouped,compress);
		double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
		worst=std::max(worst,elapsed);
		int cost=0; for (int i=0;i<int(a.size());++i) cost+=std::abs(a[i]-r.b[i]);
		int baseline=std::accumulate(a.begin(),a.end(),0)-int(a.size());
		if (r.result.status!=ip::Status::Optimal || r.cost!=expected || !valid_harmony(r.b) ||
			cost!=expected || baseline-cost!=std::llround(r.result.objective)) {
			std::cerr<<label<<" status="<<int(r.result.status)<<" expected="<<expected<<" got="<<r.cost<<'\n';
			std::cerr<<a.size()<<'\n';for (int x:a) std::cerr<<x<<' '; std::cerr<<'\n';
			throw std::runtime_error("CF453 B mismatch");
		}
		++checks;
		if (label.find("stress")!=std::string::npos)
			std::cout<<label<<", "<<r.variables<<" vars, "<<r.rows<<" rows, "<<elapsed*1000<<" ms, "
					 <<r.result.nodes<<" LP, "<<r.result.pivots<<" pivots\n";
	};
	check({1,1,1,1,1},0,"sample 1");
	check({1,6,4,2,8},3,"sample 2");
	for (int x=1;x<=30;++x) check({x},0,"singleton");
	for (int tc=0;tc<random_cases;++tc) {
		int n=1+int(rng()%8),limit=tc%8?12:30;
		std::vector<int> a(n); for (int& x:a) x=1+int(rng()%unsigned(limit));
		int expected=harmony_dp(a);
		ip::Options o; o.cuts=tc%2?8:0; o.strong_branching=tc%3?3:0;
		check(a,expected,"random "+std::to_string(tc),o,tc%4!=0);
		if (tc<30) check(a,expected,"individual positions",o,true,false,false);
	}
	for (int tc=0;tc<stress_cases;++tc) {
		std::vector<int> a(100);
		for (int& x:a) x=tc%4==0?30:tc%4==1?1+tc%30:tc%4==2?1+int(rng()%30):20+int(rng()%11);
		int expected=harmony_dp(a);
		check(a,expected,"stress "+std::to_string(tc));
	}
	std::cout<<"CF453 B: "<<checks<<" checks passed; worst "<<worst*1000<<" ms\n";
	return 0;
}
#endif
