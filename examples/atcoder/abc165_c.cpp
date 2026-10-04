// https://atcoder.jp/contests/abc165/tasks/abc165_c
#include "ip_solver.hpp"
#include <iostream>

struct Requirement { int a,b,c,d; };
struct RequirementsAnswer { long long score; int variables,rows; ip::Result result; };

RequirementsAnswer many_requirements(int n,int m,const std::vector<Requirement>& q,
									ip::Options options={},bool hint=true,
									bool continuous_gaps=true,bool mutex=true,
									int conflict_cuts=0) {
	int gaps=n-1,vars=gaps+int(q.size()),cap=m-1,rows=0;
	ip::Vec objective(vars);
	for (int i=0;i<int(q.size());++i) objective[gaps+i]=q[i].d;
	ip::Solver s(objective);
	for (int j=0;j<gaps;++j) {
		s.bounds(j,0,cap);
		if (continuous_gaps) s.continuous(j);
	}
	for (int i=gaps;i<vars;++i) s.bounds(i,0,1);
	ip::Vec total(vars);
	std::fill(total.begin(),total.begin()+gaps,1);
	s.add_le(total,cap); ++rows;
	for (int i=0;i<int(q.size());++i) {
		auto r=q[i];
		if (r.c<cap) {
			ip::Vec row(vars);
			for (int j=r.a;j<r.b;++j) row[j]=1;
			row[gaps+i]=cap-r.c;
			s.add_le(row,cap); ++rows;
		}
		if (r.c>0) {
			ip::Vec row(vars);
			for (int j=r.a;j<r.b;++j) row[j]=-1;
			row[gaps+i]=r.c;
			s.add_le(row,0); ++rows;
		}
	}
	if (mutex) for (int a=0;a<n;++a) for (int b=a+1;b<n;++b) {
		ip::Vec row(vars); int count=0;
		for (int i=0;i<int(q.size());++i) if (q[i].a==a && q[i].b==b)
			row[gaps+i]=1,++count;
		if (count>1) { s.add_le(row,1); ++rows; }
	}
#ifdef IP_EXAMPLE_TEST
	if (conflict_cuts) {
		int k=int(q.size());
		std::vector<std::vector<bool>> conflict(k,std::vector<bool>(k));
		std::vector<int> degree(k),order(k);
		std::iota(order.begin(),order.end(),0);
		for (int i=0;i<k;++i) for (int j=0;j<i;++j) {
			auto x=q[i],y=q[j];
			bool bad=(x.b<=y.a || y.b<=x.a) && x.c+y.c>cap;
			bad|=x.a<=y.a && y.b<=x.b && x.c<y.c;
			bad|=y.a<=x.a && x.b<=y.b && y.c<x.c;
			if (bad) conflict[i][j]=conflict[j][i]=true,++degree[i],++degree[j];
		}
		std::sort(order.begin(),order.end(),[&](int i,int j) { return degree[i]>degree[j]; });
		std::vector<std::vector<int>> cliques;
		for (int i=0;i<k;++i) {
			if (conflict_cuts==2) {
				for (int j=0;j<i;++j) if (conflict[i][j]) cliques.push_back({j,i});
			} else {
				std::vector<int> clique{i};
				for (int j:order) if (std::all_of(clique.begin(),clique.end(),[&](int t) { return conflict[j][t]; }))
					clique.push_back(j);
				std::sort(clique.begin(),clique.end());
				if (clique.size()>1 && std::find(cliques.begin(),cliques.end(),clique)==cliques.end())
					cliques.push_back(std::move(clique));
			}
		}
		for (const auto& clique:cliques) {
			ip::Vec row(vars); for (int i:clique) row[gaps+i]=1;
			s.add_le(row,1); ++rows;
		}
	}
#else
	(void)conflict_cuts;
#endif
	if (hint) {
		auto score=[&](const std::vector<int>& a) {
			int value=0;
			for (auto r:q) if (a[r.b]-a[r.a]==r.c) value+=r.d;
			return value;
		};
		std::vector<int> best(n,1);
		int best_score=score(best);
		uint32_t state=1652026104;
		// Coordinate improvement from deterministic monotone starting points.
		for (int start=0;start<24;++start) {
			std::vector<int> a(n,1);
			if (start) {
				for (int& x:a) { state^=state<<13; state^=state>>17; state^=state<<5; x=1+int(state%unsigned(m)); }
				std::sort(a.begin(),a.end());
			}
			int value=score(a);
			for (;;) {
				int gain=0,position=-1,chosen=0;
				for (int j=0;j<n;++j) {
					int old=a[j],lo=j?a[j-1]:1,hi=j+1<n?a[j+1]:m;
					for (int x=lo;x<=hi;++x) {
						a[j]=x; int next=score(a);
						if (next-value>gain) gain=next-value,position=j,chosen=x;
					}
					a[j]=old;
				}
				if (!gain) break;
				a[position]=chosen; value+=gain;
			}
			if (value>best_score) best_score=value,best=std::move(a);
		}
		options.initial_solution.assign(vars,0);
		for (int j=0;j<gaps;++j) options.initial_solution[j]=best[j+1]-best[j];
		for (int i=0;i<int(q.size());++i)
			options.initial_solution[gaps+i]=best[q[i].b]-best[q[i].a]==q[i].c;
	}
	auto r=s.maximize(options);
	long long score=r.has_solution()?std::llround(r.objective):-1;
	return {score,vars,rows,std::move(r)};
}

#ifndef IP_EXAMPLE_TEST
int main() {
	std::ios::sync_with_stdio(false); std::cin.tie(nullptr);
	int n,m,k; std::cin>>n>>m>>k;
	std::vector<Requirement> q(k);
	for (auto& r:q) { std::cin>>r.a>>r.b>>r.c>>r.d; --r.a; --r.b; }
	ip::Options options; options.cuts=0;
	auto r=many_requirements(n,m,q,options);
	if (r.result.status!=ip::Status::Optimal) return 1;
	std::cout<<r.score<<'\n';
}
#endif
