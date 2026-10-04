#define IP_EXAMPLE_TEST
#ifdef IP_TEST_STANDALONE
#include "../build/submissions/atcoder/abc165_c.cpp"
#else
#include "../examples/atcoder/abc165_c.cpp"
#endif
#include <chrono>
#include <random>
#include <stdexcept>
#include <string>

long long sequence_oracle(int n,int m,const std::vector<Requirement>& q) {
	std::vector<int> a(n);
	long long best=0;
	auto dfs=[&](auto&& self,int j,int lo)->void {
		if (j==n) {
			long long score=0;
			for (auto r:q) if (a[r.b]-a[r.a]==r.c) score+=r.d;
			best=std::max(best,score); return;
		}
		for (int x=lo;x<=m;++x) { a[j]=x; self(self,j+1,x); }
	};
	dfs(dfs,0,1); return best;
}

#ifndef IP_ABC165_ORACLES_ONLY
int main(int argc,char** argv) {
	int random_cases=argc>1?std::stoi(argv[1]):400;
	int stress_cases=argc>2?std::stoi(argv[2]):24;
	double time_limit=argc>3?std::stod(argv[3]):2;
	std::mt19937 rng(1652026104);
	int checks=0;
	auto check=[&](int n,int m,const std::vector<Requirement>& q,long long expected,
				   const std::string& label,ip::Options o={},bool hint=true,
				   bool continuous=true,bool mutex=true) {
		auto start=std::chrono::steady_clock::now();
		auto r=many_requirements(n,m,q,o,hint,continuous,mutex);
		double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
		if (r.result.status!=ip::Status::Optimal || r.score!=expected) {
			std::cerr<<label<<" status="<<int(r.result.status)<<" expected="<<expected<<" got="<<r.score<<'\n';
			std::cerr<<n<<' '<<m<<' '<<q.size()<<'\n';
			for (auto x:q) std::cerr<<x.a+1<<' '<<x.b+1<<' '<<x.c<<' '<<x.d<<'\n';
			throw std::runtime_error("ABC165 C oracle mismatch");
		}
		++checks;
		if (label.find("stress")!=std::string::npos)
			std::cout<<label<<", "<<elapsed*1000<<" ms, "<<r.result.nodes<<" LP, "<<r.result.pivots<<" pivots\n";
	};
	check(3,4,{{0,2,3,100},{0,1,2,10},{1,2,2,10}},110,"sample 1");
	check(4,6,{{1,3,1,86568},{0,3,0,90629},{1,2,0,90310},{2,3,1,29211},
			   {2,3,3,78537},{2,3,2,8580},{0,1,1,96263},{0,3,2,2156},
			   {0,1,0,94325},{0,3,3,94328}},357500,"sample 2");
	check(10,10,{{0,9,9,1}},1,"sample 3");
	check(2,1,{{0,1,0,100000}},100000,"M=1");
	for (int tc=0;tc<random_cases+stress_cases;++tc) {
		bool stress=tc>=random_cases;
		int n=stress?10:2+int(rng()%6),m=stress?10:1+int(rng()%6);
		std::vector<Requirement> pool;
		for (int a=0;a<n;++a) for (int b=a+1;b<n;++b) for (int c=0;c<m;++c)
			pool.push_back({a,b,c,1+int(rng()%100000)});
		std::shuffle(pool.begin(),pool.end(),rng);
		int count=stress?50:1+int(rng()%std::min<int>(50,pool.size()));
		pool.resize(count);
		if (stress && tc%3==0) for (auto& r:pool) r.d=100000;
		auto expected=sequence_oracle(n,m,pool);
		ip::Options o; o.time_limit=time_limit;
		if (!stress) { o.cuts=tc%2?8:0; o.strong_branching=tc%3?3:0; }
		else o.cuts=0;
		check(n,m,pool,expected,(stress?"stress ":"random ")+std::to_string(tc),o,tc%4!=0);
		if (!stress && tc<80) check(n,m,pool,expected,"integer gaps",o,true,false);
	}
	std::cout<<"ABC165 C: "<<checks<<" checks passed\n";
	return 0;
}
#endif
