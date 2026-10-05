#pragma once
// Formulation variants and diagnostics for solver tests and benchmarks.
// https://codeforces.com/problemset/problem/1138/B
#include "ip_solver.hpp"
#include <iostream>
#include <string>
using namespace std;

struct CircusAnswer { vector<int> first; ip::Result result; };
CircusAnswer circus(const string& c,const string& a,ip::Options o={}) {
	vector<int> groups[3]; int total=0,n=int(c.size());
	for (int i=0;i<n;++i) { groups[c[i]+a[i]-2*'0'].push_back(i); total+=a[i]-'0'; }
	ip::Solver s(ip::Vec(3));
	for (int k=0;k<3;++k) s.bounds(k,0,groups[k].size());
	s.add_eq({1,1,1},n/2); s.add_eq({0,1,2},total);
	auto r=s.maximize(o); vector<int> first;
	if (r.has_solution()) for (int k=0;k<3;++k)
		for (int j=0;j<int(llround(r.x[k]));++j) first.push_back(groups[k][j]);
	return {std::move(first),std::move(r)};
}
