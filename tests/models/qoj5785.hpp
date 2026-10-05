#pragma once
// Formulation variants and diagnostics for solver tests and benchmarks.
#include "ip_solver.hpp"
#include <iostream>
#include <stdexcept>
using namespace std;

// A_n is tridiagonal; if n%3==2 its null vector is (1,-1,0,...).
vector<int> mine_line(const vector<int>& b) {
	int n=int(b.size()); vector<int> p(n),h(n);
	h[0]=1;
	for (int i=0;i+1<n;++i) {
		p[i+1]=b[i]-p[i]-(i?p[i-1]:0);
		h[i+1]=-h[i]-(i?h[i-1]:0);
	}
	int residual=b.back()-p.back()-(n>1?p[n-2]:0);
	int coefficient=h.back()+(n>1?h[n-2]:0);
	if (!coefficient) {
		if (residual) throw runtime_error("inconsistent clues");
	} else for (int i=0;i<n;++i) p[i]+=h[i]*(residual/coefficient);
	return p;
}
struct MineAnswer { int mines, variables, rows; vector<vector<int>> layout; ip::Result result; };
MineAnswer mine_layer(const vector<vector<int>>& clue, ip::Options o={}) {
	int r=int(clue.size()), c=int(clue[0].size());
	bool vr=r%3==2, hc=c%3==2;
	vector<vector<int>> q(r,vector<int>(c)), y;
	for (const auto& row:clue) y.push_back(mine_line(row));
	for (int j=0;j<c;++j) {
		vector<int> b(r); for (int i=0;i<r;++i) b[i]=y[i][j];
		auto v=mine_line(b); for (int i=0;i<r;++i) q[i][j]=v[i];
	}
	auto h=[](int i) { return i%3==0?1:i%3==1?-1:0; };
	int n=(hc?r:0)+(vr?c-int(hc):0), rows=0, offset=0;
	ip::Vec objective(n);
	auto terms=[&](int i,int j) {
		vector<pair<int,int>> a;
		if (hc && h(j)) a.push_back({i,h(j)});
		if (vr && h(i) && (!hc || j)) a.push_back({(hc?r:0)+j-int(hc),h(i)});
		return a;
	};
	for (int j=0;j<c;++j) {
		offset+=q[r/2][j]; for (auto [k,a]:terms(r/2,j)) objective[k]+=a;
	}
	ip::Solver s(objective);
	for (int k=0;k<n;++k) s.bounds(k,hc && k>=r?-1:0,hc && k>=r?2:1);
	for (int i=0;i<r;++i) for (int j=0;j<c;++j) {
		auto a=terms(i,j);
		if (a.empty()) {
			if (q[i][j]<0 || q[i][j]>1) throw runtime_error("invalid fixed mine");
		} else {
			ip::Vec row(n); for (auto [k,v]:a) row[k]=v;
			s.add_le(row,1-q[i][j]); s.add_ge(row,-q[i][j]); rows+=2;
		}
	}
	// The remaining constraints are a signed bipartite incidence matrix (TU).
	o.cuts=0;
	auto result=s.maximize(o);
	if (result.status!=ip::Status::Optimal) return {-1,n,rows,{},std::move(result)};
	for (int i=0;i<r;++i) for (int j=0;j<c;++j)
		for (auto [k,a]:terms(i,j)) q[i][j]+=a*int(llround(result.x[k]));
	int answer=0; for (int x:q[r/2]) answer+=x;
	result.objective+=offset; result.bound+=offset;
	return {answer,n,rows,std::move(q),std::move(result)};
}
