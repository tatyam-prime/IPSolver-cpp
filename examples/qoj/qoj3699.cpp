// https://qoj.ac/problem/3699
#include "ip_solver.hpp"
#include <iostream>
#include <map>
#include <set>

using namespace std;

int vertex_cover(int n, const vector<pair<int, int>>& edges) {
	vector<set<int>> g(n);
	for (auto [u, v] : edges) {
		g[u].insert(v);
		g[v].insert(u);
	}
	int fixed = 0;
	for (;;) {
		int take = -1;
		// All edges touch the first 30 vertices, which form a feasible cover.
		int upper = 0;
		for (int v = 0; v < min(n, 30); ++v)
			upper += !g[v].empty();
		for (int v = 0; v < n; ++v)
			if (int(g[v].size()) > upper) {
				take = v;
				break;
			}
		for (int v = 0; v < n; ++v)
			if (g[v].size() == 1) {
				take = *g[v].begin();
				break;
			}
		if (take < 0)
			break;
		++fixed;
		for (int v : g[take])
			g[v].erase(take);
		g[take].clear();
	}
	int answer = fixed;
	vector<bool> seen(n);
	for (int start = 0; start < n; ++start)
		if (!seen[start] && !g[start].empty()) {
			vector<int> component{start};
			seen[start] = true;
			for (int k = 0; k < int(component.size()); ++k)
				for (int v : g[component[k]])
					if (!seen[v]) {
						seen[v] = true;
						component.push_back(v);
					}
			// False twins are independent and an optimum may select all of them.
			map<vector<int>, int> groups;
			vector<int> id(n, -1), weight;
			for (int v : component) {
				vector<int> key(g[v].begin(), g[v].end());
				auto [it, added] = groups.emplace(key, int(groups.size()));
				if (added)
					weight.push_back(0);
				id[v] = it->second;
				++weight[id[v]];
			}
			int k = int(weight.size());
			vector<set<int>> adj(k);
			for (int u : component)
				for (int v : g[u])
					adj[id[u]].insert(id[v]);
			ip::Vec objective(weight.begin(), weight.end());
			ip::Solver s(objective);
			for (int i = 0; i < k; ++i)
				s.bounds(i, 0, 1);
			for (int i = 0; i < k; ++i)
				for (int j : adj[i])
					if (i < j) {
						ip::Vec a(k);
						a[i] = a[j] = 1;
						s.add_le(a, 1);
					}
			// Safe greedy clique cuts; duplicates are removed.
			{
				set<vector<int>> cliques;
				for (int i = 0; i < k; ++i) {
					vector<int> clique{i};
					for (int j : adj[i]) {
						bool ok = true;
						for (int v : clique)
							if (!adj[v].count(j)) {
								ok = false;
								break;
							}
						if (ok)
							clique.push_back(j);
					}
					sort(clique.begin(), clique.end());
					if (clique.size() > 2 && cliques.insert(clique).second) {
						ip::Vec a(k);
						for (int v : clique)
							a[v] = 1;
						s.add_le(a, 1);
					}
				}
			}
			ip::Options options;
			{
				options.initial_solution.assign(k, 0);
				vector<int> order(k);
				iota(order.begin(), order.end(), 0);
				sort(order.begin(), order.end(), [&](int u, int v) {
					return weight[u] * (1 + adj[v].size()) > weight[v] * (1 + adj[u].size());
				});
				for (int u : order) {
					bool ok = true;
					for (int v : adj[u])
						if (options.initial_solution[v])
							ok = false;
					if (ok)
						options.initial_solution[u] = 1;
				}
			}
			auto r = s.maximize(options);
			int independent = 0;
			for (int i = 0; i < k; ++i)
				independent += weight[i] * int(llround(r.x[i]));
			answer += int(component.size()) - independent;
		}
	return answer;
}

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);
	int n, m;
	while (cin >> n >> m) {
		vector<pair<int, int>> edges(m);
		for (auto& [u, v] : edges) {
			cin >> u >> v;
			--u;
			--v;
		}
		cout << vertex_cover(n, edges) << '\n';
	}
}
