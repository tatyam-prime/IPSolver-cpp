#define IP_BENCH_ONLY
#include "../benchmarks/qoj.cpp"

int main(int argc,char** argv) {
	int r=argc>1?stoi(argv[1]):17, c=argc>2?stoi(argv[2]):r;
	Grid x(r,vector<int>(c)); for (int i=0;i<r;++i) for (int j=0;j<c;++j) x[i][j]=(i+j)%2;
	auto a=clues(x); int expected=accumulate(x[r/2].begin(),x[r/2].end(),0), failures=0;
	cout<<"Known feasible checkerboard; expected middle row "<<expected<<'\n';
	for (bool continuous:{false,true}) for (double eps:{1e-9,1e-8,1e-7}) {
		int n=r*c; ip::Vec objective(n);
		for (int j=0;j<c;++j) objective[(r/2)*c+j]=1;
		ip::Solver s(objective);
		for (int k=0;k<n;++k) { s.bounds(k,0,1); if (continuous) s.continuous(k); }
		for (int i=0;i<r;++i) for (int j=0;j<c;++j) {
			ip::Vec row(n);
			for (int u=max(0,i-1);u<=min(r-1,i+1);++u)
				for (int v=max(0,j-1);v<=min(c-1,j+1);++v) row[u*c+v]=1;
			s.add_eq(row,a[i][j]);
		}
		ip::Options o; o.cuts=0; o.time_limit=2; o.eps=eps;
		auto result=s.maximize(o);
		bool wrong=result.status==ip::Status::Infeasible || (result.status==ip::Status::Optimal && abs(result.objective-expected)>1e-5);
		cout<<(continuous?"LP":"IP")<<",eps="<<eps<<','<<status(result.status)<<",LPs="<<result.nodes<<",pivots="<<result.pivots<<",objective="<<result.objective<<",incorrect="<<wrong<<'\n';
		failures+=wrong;
#ifndef IP_NO_INITIAL
		if (!continuous && eps==1e-9) {
			o.initial_solution.resize(n); for (int i=0;i<r;++i) for (int j=0;j<c;++j) o.initial_solution[i*c+j]=x[i][j];
			auto hint=s.maximize(o);
			cout<<"IP,known_hint,"<<status(hint.status)<<",LPs="<<hint.nodes<<",pivots="<<hint.pivots<<",objective="<<hint.objective<<'\n';
		}
#endif
	}
	return failures?1:0;
}
