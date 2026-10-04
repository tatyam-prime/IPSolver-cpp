// https://atcoder.jp/contests/abc180/tasks/abc180_e
#include "ip_solver.hpp"
#include <array>
#include <iostream>

using City = std::array<int,3>;
struct AerialAnswer {
	long long cost=-1;
	std::vector<int> tour;
	ip::Result result;
	int solves=0, subtour_cuts=0;
};

AerialAnswer aerial_cities(const std::vector<City>& p, ip::Options options={}) {
	int n=int(p.size());
	std::vector<std::vector<int>> d(n,std::vector<int>(n)), id(n,std::vector<int>(n,-1));
	for (int i=0; i<n; ++i) for (int j=0; j<n; ++j)
		d[i][j]=2*std::abs(p[i][0]-p[j][0])+2*std::abs(p[i][1]-p[j][1])+std::abs(p[i][2]-p[j][2]);
	AerialAnswer answer;
	if (n==2) {
		answer.cost=d[0][1]; answer.tour={0,1};
		answer.result.status=ip::Status::Optimal;
		answer.result.objective=answer.result.bound=-2*answer.cost;
		return answer;
	}
	std::vector<std::pair<int,int>> edge;
	ip::Vec objective;
	for (int i=0; i<n; ++i) for (int j=i+1; j<n; ++j) {
		id[i][j]=id[j][i]=int(edge.size());
		edge.push_back({i,j}); objective.push_back(-d[i][j]);
	}
	ip::Solver solver(objective);
	for (int e=0; e<int(edge.size()); ++e) solver.bounds(e,0,1);
	for (int i=0; i<n; ++i) {
		ip::Vec row(edge.size());
		for (int j=0; j<n; ++j) if (i!=j) row[id[i][j]]=1;
		solver.add_eq(row,2);
	}
	// A feasible tour gives an upper bound; IP proves optimality.
	long long best=INT64_MAX;
	std::vector<int> seed;
	for (int start=0; start<n; ++start) {
		std::vector<int> tour{start}, used(n); used[start]=1;
		while (int(tour.size())<n) {
			int next=-1;
			for (int j=0; j<n; ++j) if (!used[j] &&
				(next<0 || d[tour.back()][j]<d[tour.back()][next])) next=j;
			tour.push_back(next); used[next]=1;
		}
		bool changed=true;
		while (changed) {
			changed=false;
			for (int i=0; i<n; ++i) for (int j=i+2; j<n; ++j) if (i || j<n-1) {
				int a=tour[i], b=tour[(i+1)%n], c=tour[j], e=tour[(j+1)%n];
				if (d[a][c]+d[b][e]<d[a][b]+d[c][e]) {
					std::reverse(tour.begin()+i+1,tour.begin()+j+1); changed=true;
				}
			}
		}
		long long cost=0;
		for (int i=0; i<n; ++i) cost+=d[tour[i]][tour[(i+1)%n]];
		if (cost<best) { best=cost; seed=std::move(tour); }
	}
	options.initial_solution.assign(edge.size(),0);
	for (int i=0; i<n; ++i) options.initial_solution[id[seed[i]][seed[(i+1)%n]]]=1;
	options.cuts=0;
	auto started=std::chrono::steady_clock::now();
	uint64_t nodes=0, pivots=0;
	std::vector<bool> added(1<<n);
	for (;;) {
		auto o=options;
		o.node_limit-=nodes; o.pivot_limit-=pivots;
		if (std::isfinite(o.time_limit)) o.time_limit=std::max(0.0,o.time_limit-
			std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count());
		auto r=solver.maximize(o);
		++answer.solves; nodes+=r.nodes; pivots+=r.pivots;
		r.nodes=nodes; r.pivots=pivots;
		if (r.status!=ip::Status::Optimal) { answer.result=std::move(r); return answer; }
		std::vector<std::vector<int>> adj(n);
		for (int e=0; e<int(edge.size()); ++e) if (r.x[e]>0.5) {
			auto [i,j]=edge[e]; adj[i].push_back(j); adj[j].push_back(i);
		}
		std::vector<bool> seen(n);
		std::vector<std::vector<int>> cycles;
		for (int i=0; i<n; ++i) if (!seen[i]) {
			std::vector<int> cycle;
			int previous=-1, current=i;
			do {
				cycle.push_back(current); seen[current]=true;
				int next=adj[current][0]==previous ? adj[current][1] : adj[current][0];
				previous=current; current=next;
			} while (current!=i);
			cycles.push_back(std::move(cycle));
		}
		if (cycles.size()==1) {
			answer.tour=std::move(cycles[0]);
			answer.cost=std::llround(-r.objective)/2;
			answer.result=std::move(r); return answer;
		}
		for (const auto& cycle:cycles) {
			int mask=0; for (int i:cycle) mask|=1<<i;
			if (mask&1) mask^=(1<<n)-1; // S and its complement have the same cut.
			if (added[mask]) continue;
			added[mask]=true; ++answer.subtour_cuts;
			ip::Vec row(edge.size());
			for (int e=0; e<int(edge.size()); ++e) {
				auto [i,j]=edge[e]; row[e]=((mask>>i)^(mask>>j))&1;
			}
			solver.add_ge(row,2); // Every tour crosses each nontrivial cut twice.
		}
	}
}

#ifndef IP_EXAMPLE_TEST
int main() {
	std::ios::sync_with_stdio(false); std::cin.tie(nullptr);
	int n; std::cin>>n; std::vector<City> p(n);
	for (auto& city:p) for (int& x:city) std::cin>>x;
	auto answer=aerial_cities(p);
	if (answer.result.status!=ip::Status::Optimal) return 1;
	std::cout<<answer.cost<<'\n';
}
#endif
