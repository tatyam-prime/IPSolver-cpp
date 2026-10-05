#include "models/cf1138_b.hpp"
#include <chrono>
#include <random>
#include <stdexcept>

void circus_check(bool ok,const string& message) { if (!ok) throw runtime_error(message); }
bool circus_brute(const string& c,const string& a) {
	int n=int(c.size()); unsigned cm=0,am=0;
	for (int i=0;i<n;++i) { cm|=unsigned(c[i]-'0')<<i; am|=unsigned(a[i]-'0')<<i; }
	unsigned all=(1u<<n)-1;
	for (unsigned x=0;x<=all;++x) if (__builtin_popcount(x)==n/2 &&
		__builtin_popcount(x&cm)==__builtin_popcount((all^x)&am)) return true;
	return false;
}
bool circus_count_oracle(const string& c,const string& a) {
	int count[4]={},n=int(c.size()),total=0;
	for (int i=0;i<n;++i) { ++count[2*(c[i]-'0')+a[i]-'0']; total+=a[i]-'0'; }
	for (int both=0;both<=count[3];++both) {
		int one=total-2*both,neither=n/2-one-both;
		if (one>=0 && one<=count[1]+count[2] && neither>=0 && neither<=count[0]) return true;
	}
	return false;
}
void circus_validate(const string& c,const string& a,const CircusAnswer& r,bool feasible) {
	int n=int(c.size());
	circus_check(r.result.status==(feasible?ip::Status::Optimal:ip::Status::Infeasible),"circus wrong status");
	if (!feasible) { circus_check(!r.result.has_solution(),"infeasible has solution"); return; }
	circus_check(int(r.first.size())==n/2,"circus size");
	vector<bool> selected(n); int clown=0,acrobat=0;
	for (int i:r.first) {
		circus_check(i>=0 && i<n && !selected[i],"circus indices"); selected[i]=true; clown+=c[i]-'0';
	}
	for (int i=0;i<n;++i) if (!selected[i]) acrobat+=a[i]-'0';
	circus_check(clown==acrobat,"circus balance");
}
#ifndef IP_CIRCUS_HELPERS_ONLY
int main(int argc,char** argv) {
	int iterations=argc>1?stoi(argv[1]):2000,cases=0; mt19937 rng(1138);
	double max_ms=0; uint64_t max_nodes=0,max_pivots=0;
	auto test=[&](const string& c,const string& a,bool expected,ip::Options o={}) {
		auto start=chrono::steady_clock::now(); auto r=circus(c,a,o);
		max_ms=max(max_ms,1000*chrono::duration<double>(chrono::steady_clock::now()-start).count());
		max_nodes=max(max_nodes,r.result.nodes); max_pivots=max(max_pivots,r.result.pivots);
		circus_validate(c,a,r,expected); ++cases;
	};
	test("0011","0101",true); test("000000","111111",false);
	test("0011","1100",true); test("00100101","01111100",true);
	for (int n=2;n<=10;n+=2) for (int z=0;z<=n;++z) for (int u=0;u<=n-z;++u)
		for (int v=0;v<=n-z-u;++v) {
			int b=n-z-u-v;
			string c=string(z+u,'0')+string(v+b,'1');
			string a=string(z,'0')+string(u,'1')+string(v,'0')+string(b,'1');
			bool expected=circus_brute(c,a);
			circus_check(circus_count_oracle(c,a)==expected,"count oracle vs brute");
			ip::Options o; o.cuts=cases%2?8:0; o.strong_branching=cases%3?3:0;
			test(c,a,expected,o);
		}
	for (int it=0;it<iterations;++it) {
		int n=2+2*(rng()%9); string c(n,'0'),a(n,'0');
		for (int i=0;i<n;++i) { c[i]+=rng()%2; a[i]+=rng()%2; }
		bool expected=circus_brute(c,a); circus_check(circus_count_oracle(c,a)==expected,"random count oracle");
		test(c,a,expected);
	}
	for (int it=0;it<1000;++it) {
		int n=5000; string c(n,'0'),a(n,'0');
		for (int i=0;i<n;++i) {
			if (it%10<4) { c[i]+=it%2; a[i]+=(it/2)%2; }
			else if (it%10==4) { c[i]+=i<n/2+1; a[i]=c[i]; }
			else { c[i]+=rng()%2; a[i]+=rng()%2; }
		}
		test(c,a,circus_count_oracle(c,a));
	}
	cout<<"CF1138B: "<<cases<<" cases passed; worst full call "<<max_ms<<"ms, max LPs "<<max_nodes<<", pivots "<<max_pivots<<'\n';
}
#endif
