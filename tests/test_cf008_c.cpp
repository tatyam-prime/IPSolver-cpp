// Independent subset-DP oracle for tests/test_cf008_c.py; no IP solver used.
#include <algorithm>
#include <iostream>
#include <vector>

int main() {
	int cases;
	std::cin >> cases;
	while (cases--) {
		int sx, sy, n;
		std::cin >> sx >> sy >> n;
		std::vector<int> x(n+1, sx), y(n+1, sy);
		for (int i=0; i<n; ++i) std::cin >> x[i] >> y[i];
		std::vector<std::vector<int>> d(n+1, std::vector<int>(n+1));
		for (int i=0; i<=n; ++i) for (int j=0; j<=n; ++j)
			d[i][j]=(x[i]-x[j])*(x[i]-x[j])+(y[i]-y[j])*(y[i]-y[j]);
		std::vector<int> memo(1<<n, -1);
		memo[0]=0;
		auto solve = [&](auto&& self, unsigned mask) -> int {
			int& best=memo[mask];
			if (best>=0) return best;
			int i=__builtin_ctz(mask);
			unsigned rest=mask^(1u<<i);
			best=2*d[i][n]+self(self,rest);
			for (unsigned left=rest; left; left&=left-1) {
				int j=__builtin_ctz(left);
				best=std::min(best,d[n][i]+d[i][j]+d[j][n]+self(self,rest^(1u<<j)));
			}
			return best;
		};
		std::cout << solve(solve,(1u<<n)-1) << '\n';
	}
}
