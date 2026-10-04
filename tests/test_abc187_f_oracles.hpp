#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <numeric>
#include <utility>
#include <vector>

namespace abc187_oracle {
using Edges=std::vector<std::pair<int,int>>;

// Partition DP on all vertex subsets; no maximal-clique enumeration.
inline int subset_dp(int n,const Edges& edges) {
	std::vector<unsigned> adj(n);
	for (auto [u,v]:edges) { adj[u]|=1u<<v; adj[v]|=1u<<u; }
	unsigned size=1u<<n;
	std::vector<bool> clique(size,true);
	std::vector<int> memo(size,-1); memo[0]=0;
	for (unsigned mask=1;mask<size;++mask) {
		int v=__builtin_ctz(mask); unsigned rest=mask&(mask-1);
		clique[mask]=clique[rest] && !(rest&~adj[v]);
	}
	std::function<int(unsigned)> solve=[&](unsigned mask) {
		int& best=memo[mask]; if (best>=0) return best;
		if (clique[mask]) return best=1;
		int v=__builtin_ctz(mask); unsigned bit=1u<<v, candidates=(mask^bit)&adj[v];
		best=n;
		for (unsigned sub=candidates;;sub=(sub-1)&candidates) {
			if (clique[sub]) best=std::min(best,1+solve(mask^(sub|bit)));
			if (!sub) break;
		}
		return best;
	};
	return solve(size-1);
}

// Exact coloring of the complement via DSATUR, independent of the IP model.
inline int coloring(int n,const Edges& edges) {
	unsigned all=(1u<<n)-1;
	std::vector<unsigned> adj(n,all);
	for (int i=0;i<n;++i) adj[i]^=1u<<i;
	for (auto [u,v]:edges) { adj[u]&=~(1u<<v); adj[v]&=~(1u<<u); }
	std::vector<int> color(n,-1),order(n); std::iota(order.begin(),order.end(),0);
	std::sort(order.begin(),order.end(),[&](int u,int v) {
		return __builtin_popcount(adj[u])>__builtin_popcount(adj[v]);
	});
	int best=0;
	for (int v:order) {
		unsigned forbidden=0;
		for (unsigned left=adj[v];left;left&=left-1) {
			int u=__builtin_ctz(left); if (color[u]>=0) forbidden|=1u<<color[u];
		}
		int c=0; while (forbidden>>c&1u) ++c;
		color[v]=c; best=std::max(best,c+1);
	}
	std::fill(color.begin(),color.end(),-1);
	std::function<void(unsigned,int)> dfs=[&](unsigned remaining,int used) {
		if (!remaining) { best=std::min(best,used); return; }
		if (used>=best) return;
		int selected=-1,saturation=-1,degree=-1;
		unsigned forbidden=0;
		for (unsigned left=remaining;left;left&=left-1) {
			int v=__builtin_ctz(left); unsigned mask=0;
			for (unsigned neighbors=adj[v]&~remaining;neighbors;neighbors&=neighbors-1)
				mask|=1u<<color[__builtin_ctz(neighbors)];
			int a=__builtin_popcount(mask),b=__builtin_popcount(adj[v]&remaining);
			if (a>saturation || (a==saturation && b>degree)) {
				selected=v; saturation=a; degree=b; forbidden=mask;
			}
		}
		unsigned next=remaining^(1u<<selected);
		for (int c=0;c<used;++c) if (!(forbidden>>c&1u)) {
			color[selected]=c; dfs(next,used);
		}
		if (used+1<best) { color[selected]=used; dfs(next,used+1); }
		color[selected]=-1;
	};
	dfs(all,0);
	return best;
}
} // namespace abc187_oracle
