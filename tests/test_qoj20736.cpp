#include "models/qoj20736.hpp"
#include <chrono>
#include <random>
#include <stdexcept>
#include <string>

void check(bool ok, const string& message) {
	if (!ok) throw runtime_error(message);
}

// Enumerate the actual permutations, without using adjacency-count constraints.
long long permutation_oracle(vector<int> a) {
	sort(a.begin(),a.end());
	long long best=0;
	do {
		long long value=0;
		for (int i=1;i<int(a.size());++i) value+=lcm(a[i-1],a[i]);
		best=max(best,value);
	} while (next_permutation(a.begin(),a.end()));
	return best;
}

// Hamiltonian-path subset DP on individual occurrences, including duplicates.
int subset_oracle(const vector<int>& a) {
	int n=int(a.size()); vector<vector<int>> dp(1<<n,vector<int>(n,-1));
	for (int i=0;i<n;++i) dp[1<<i][i]=0;
	for (int mask=1;mask<(1<<n);++mask) for (int i=0;i<n;++i) if (dp[mask][i]>=0)
		for (int j=0;j<n;++j) if (!(mask>>j&1))
			dp[mask|(1<<j)][j]=max(dp[mask|(1<<j)][j],dp[mask][i]+lcm(a[i],a[j]));
	return *max_element(dp.back().begin(),dp.back().end());
}

int main(int argc, char** argv) {
	int iterations=argc>1?stoi(argv[1]):200;
	mt19937 rng(20736);
	int checks=0;
	double worst=0; uint64_t most_pivots=0;
	auto verify=[&](const vector<int>& a, long long expected, ip::Options options=ip::Options{}) {
		auto started=chrono::steady_clock::now();
		auto answer=maximize_lcm(a,options);
		worst=max(worst,chrono::duration<double>(chrono::steady_clock::now()-started).count());
		most_pivots=max(most_pivots,answer.result.pivots);
		check(answer.result.status==ip::Status::Optimal,"QOJ20736 status at check "+to_string(checks));
		check(answer.value==expected,"QOJ20736 value at check "+to_string(checks));
		check(answer.result.objective==double(expected),"QOJ20736 objective");
		++checks;
	};
	vector<int> a;
	auto enumerate=[&](auto&& self, int low, int left) -> void {
		if (!a.empty()) verify(a,permutation_oracle(a));
		if (!left) return;
		for (int x=low;x<=7;++x) {
			a.push_back(x); self(self,x,left-1); a.pop_back();
		}
	};
	enumerate(enumerate,1,7);
	// Independent short-path DP, shuffled input, and several solver settings.
	for (int it=0;it<iterations;++it) {
		vector<int> input(1+rng()%12); for (int& x:input) x=1+rng()%7;
		ip::Options options; options.cuts=it%2?8:0; options.strong_branching=it%3?3:0;
		verify(input,subset_oracle(input),options);
	}
	for (int x=1;x<=7;++x) verify(vector<int>(100000,x),99999LL*x);
	// With two types, maximize the number of cross edges and minimize low loops.
	for (int x=1;x<=7;++x) for (int y=x+1;y<=7;++y) for (int cx:{1,49999,50000,99999}) {
		int cy=100000-cx, cross=min(99999,2*min(cx,cy));
		int low_loops=max(0,cx-cy-1), high_loops=99999-cross-low_loops;
		vector<int> input(cx,x); input.insert(input.end(),cy,y);
		verify(input,1LL*cross*lcm(x,y)+1LL*low_loops*x+1LL*high_loops*y);
	}
	// Favorable disjoint pairs must still be connected into one path.
	for (int c:{1,2}) {
		vector<int> input;
		for (int x:{2,3,5,7}) input.insert(input.end(),c,x);
		verify(input,permutation_oracle(input));
	}
	// This relaxed optimum isolates the ones; a connectivity cut is necessary.
	vector<int> disconnected={1,1,2,2,3,3};
	auto separated=maximize_lcm(disconnected);
	check(separated.result.status==ip::Status::Optimal && separated.value==23,"separation result");
	check(separated.solves>1 && separated.connectivity_cuts>0,"connectivity separation exercised");
	verify(disconnected,permutation_oracle(disconnected));
	ip::Options limited; limited.node_limit=1;
	auto interrupted=maximize_lcm(disconnected,limited);
	check(interrupted.result.status==ip::Status::Limit && interrupted.result.nodes<=1,"shared LP budget");
	limited=ip::Options{}; limited.pivot_limit=1;
	interrupted=maximize_lcm(disconnected,limited);
	check(interrupted.result.status==ip::Status::Limit && interrupted.result.pivots<=1,"shared pivot budget");
	// Large arbitrary multiplicities: check input permutation invariance.
	for (int it=0;it<30;++it) {
		vector<int> input(100000);
		int types=1+rng()%7;
		for (int& x:input) x=1+rng()%types;
		auto answer=maximize_lcm(input);
		check(answer.result.status==ip::Status::Optimal,"large random status");
		sort(input.begin(),input.end()); verify(input,answer.value);
	}
	cout<<"QOJ20736: "<<checks<<" checks passed; worst call "<<1000*worst
		<<" ms / "<<most_pivots<<" pivots\n";
}
