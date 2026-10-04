#define IP_EXAMPLE_TEST
#include "../examples/atcoder/abc338_f.cpp"
#include <cstdlib>
#include <fstream>
#include <random>
#include <string>

constexpr Weight oracle_inf=1LL<<60;
// Independent Floyd-Warshall + directed Held-Karp; no helper from the IP model.
Weight negative_oracle(int n,const std::vector<DirectedEdge>& edges) {
	std::vector<Weight> distance(n*n,oracle_inf);
	for (int i=0;i<n;++i) distance[i*n+i]=0;
	for (auto edge:edges) distance[edge.from*n+edge.to]=edge.cost;
	for (int k=0;k<n;++k) for (int i=0;i<n;++i) for (int j=0;j<n;++j)
		if (distance[i*n+k]!=oracle_inf && distance[k*n+j]!=oracle_inf)
			distance[i*n+j]=std::min(distance[i*n+j],distance[i*n+k]+distance[k*n+j]);
	int masks=1<<n;
	std::vector<Weight> dp(size_t(masks)*n,oracle_inf);
	for (int i=0;i<n;++i) dp[size_t(1<<i)*n+i]=0;
	for (int mask=1;mask<masks;++mask) {
		unsigned remaining=unsigned(masks-1)^unsigned(mask);
		for (unsigned present=unsigned(mask);present;present&=present-1) {
			int last=__builtin_ctz(present);
			Weight current=dp[size_t(mask)*n+last];
			if (current==oracle_inf) continue;
			for (unsigned left=remaining;left;left&=left-1) {
				int next=__builtin_ctz(left);
				if (distance[last*n+next]==oracle_inf) continue;
				Weight& out=dp[size_t(mask|(1<<next))*n+next];
				out=std::min(out,current+distance[last*n+next]);
			}
		}
	}
	return *std::min_element(dp.end()-n,dp.end());
}

void save_case(int n,const std::vector<DirectedEdge>& edges,const std::string& path) {
	std::ofstream file(path); file<<n<<' '<<edges.size()<<'\n';
	for (auto e:edges) file<<e.from+1<<' '<<e.to+1<<' '<<e.cost<<'\n';
}

int main(int argc,char** argv) {
	if (argc>1 && std::string(argv[1])=="--oracle") {
		int cases; std::cin>>cases;
		while (cases--) {
			int n,m; std::cin>>n>>m; std::vector<DirectedEdge> edges(m);
			for (auto& e:edges) { std::cin>>e.from>>e.to>>e.cost; --e.from; --e.to; }
			Weight cost=negative_oracle(n,edges);
			if (cost==oracle_inf) std::cout<<"No\n"; else std::cout<<cost<<'\n';
		}
		return 0;
	}
	int small=argc>1?std::atoi(argv[1]):500,large=argc>2?std::atoi(argv[2]):12;
	bool zero_cycles=argc>3 && std::atoi(argv[3]);
	std::mt19937 rng(3382026104);
	int checks=0,max_solves=0,max_cuts=0;
	uint64_t max_nodes=0,max_pivots=0;
	double worst=0;
	auto check=[&](int n,const std::vector<DirectedEdge>& edges,const std::string& label,
				   Weight known=oracle_inf) {
		Weight expected=negative_oracle(n,edges);
		if (known!=oracle_inf && expected!=known) throw std::runtime_error("oracle sample mismatch");
		ip::Options options; options.time_limit=5.5;
		auto started=std::chrono::steady_clock::now();
		auto got=negative_tour(n,edges,options);
		double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
		bool pass=expected==oracle_inf ? !got.feasible :
			got.feasible && got.result.status==ip::Status::Optimal && got.cost==expected;
		if (!pass) {
			std::cerr<<label<<": feasible "<<got.feasible<<", status "<<int(got.result.status)
					 <<", cost "<<got.cost<<", expected "<<expected<<", nodes "<<got.result.nodes
					 <<", pivots "<<got.result.pivots<<", calls "<<got.solves<<'\n';
			save_case(n,edges,"/private/tmp/abc338_f_failure.in"); return false;
		}
		if (got.feasible) {
			auto order=got.order; std::sort(order.begin(),order.end());
			if (order.size()!=size_t(n)) throw std::runtime_error("invalid order length");
			for (int i=0;i<n;++i) if (order[i]!=i) throw std::runtime_error("invalid order");
			std::vector<Weight> original(n*n,oracle_inf);
			for (auto e:edges) original[e.from*n+e.to]=e.cost;
			std::vector<bool> visited(n);
			Weight cost=0;
			for (int vertex:got.walk) {
				if (vertex<0 || vertex>=n) throw std::runtime_error("invalid walk vertex");
				visited[vertex]=true;
			}
			for (int i=1;i<int(got.walk.size());++i) {
				Weight value=original[got.walk[i-1]*n+got.walk[i]];
				if (value==oracle_inf) throw std::runtime_error("nonedge in walk");
				cost+=value;
			}
			if (cost!=expected || std::find(visited.begin(),visited.end(),false)!=visited.end())
				throw std::runtime_error("expanded walk mismatch");
			if (std::llround(got.result.objective)!=-expected) throw std::runtime_error("objective offset mismatch");
		}
		++checks; worst=std::max(worst,seconds);
		max_nodes=std::max(max_nodes,got.result.nodes); max_pivots=std::max(max_pivots,got.result.pivots);
		max_solves=std::max(max_solves,got.solves); max_cuts=std::max(max_cuts,got.subtour_cuts);
		if (n==20 || label.find("sample")!=std::string::npos)
			std::cout<<label<<','<<n<<','<<edges.size()<<','<<(got.feasible?std::to_string(got.cost):"No")
					 <<','<<seconds<<','<<got.result.nodes<<','<<got.result.pivots<<','<<got.solves
					 <<','<<got.subtour_cuts<<','<<got.variables<<'\n';
		return true;
	};
	std::cout<<"label,n,m,cost,seconds,nodes,pivots,solves,subtour_cuts,variables\n";
	if (!check(3,{{0,1,5},{1,0,-3},{1,2,-4},{2,0,100}},"sample 1",-2) ||
		!check(3,{{0,1,0},{1,0,0}},"sample 2") ||
		!check(5,{{0,1,-246288},{3,4,-222742},{2,0,246288},{2,3,947824},
				   {4,1,-178721},{3,2,-947824},{4,3,756570},{1,4,707902},{4,0,36781}},
			   "sample 3",-449429)) return 1;
	for (int mode=0;mode<11;++mode) {
		int n=20; std::vector<DirectedEdge> edges;
		if (mode<3) for (int i=0;i+1<n;++i) edges.push_back({i,i+1,(mode-1)*1000000});
		if (mode==3) for (int i=0;i<n;++i) for (int j=0;j<n;++j) if (i!=j)
			edges.push_back({i,j,(j-i)*50000});
		if (mode==4) for (int i=0;i<n;++i) edges.push_back({i,(i+1)%n,0});
		if (mode==5) for (int i=0;i<n/2;++i) edges.push_back({i,(i+1)%(n/2),0});
		if (mode==6) for (int i=1;i<n;++i) {
			edges.push_back({0,i,(i-10)*50000}); edges.push_back({i,0,-(i-10)*50000});
		}
		if (mode==7) for (int i=0;i<n;++i) {
			int j=(i+1)%n;
			edges.push_back({i,j,(j-i)*50000});
			for (int k=0;k<n;++k) if (i!=k && k!=j && rng()%5==0)
				edges.push_back({i,k,(k-i)*50000});
		}
		if (mode==8) for (int i=0;i<n;++i) for (int j=0;j<n;++j)
			if (i!=j && (i/4==j/4 || j/4==i/4+1))
				edges.push_back({i,j,Weight(i/4==j/4?0:1000000)});
		if (mode>=9) for (int i=0;i<n;++i) for (int j=0;j<n;++j) if (i!=j) {
			Weight base=i/2==j/2?0:mode==9?1:1+rng()%400001;
			edges.push_back({i,j,base+(j-i)*20000});
		}
		if (!check(n,edges,"structured "+std::to_string(mode))) return 1;
	}
	// All 4095 nonempty digraphs on four vertices: every present arc is a
	// potential difference, so every directed cycle has exactly zero cost.
	if (zero_cycles) for (int mask=1;mask<(1<<12);++mask) {
		std::vector<DirectedEdge> edges;
		int bit=0;
		for (int i=0;i<4;++i) for (int j=0;j<4;++j) if (i!=j)
			if (mask>>bit++&1) edges.push_back({i,j,(j-i)*300000});
		if (!check(4,edges,"zero-cycle "+std::to_string(mask))) return 1;
	}
	for (int tc=0;tc<small+large;++tc) {
		int n=tc<small?2+int(rng()%8):20;
		std::vector<Weight> potential(n);
		for (Weight& p:potential) p=int(rng()%600001)-300000;
		std::vector<DirectedEdge> edges;
		int density=tc%4==0?100:tc%4==1?50:tc%4==2?20:8;
		for (int i=0;i<n;++i) for (int j=0;j<n;++j) if (i!=j && int(rng()%100)<density) {
			Weight base=tc%3==0 ? rng()%11 : tc%3==1 ? rng()%1001 : rng()%400001;
			edges.push_back({i,j,base+potential[j]-potential[i]});
		}
		if (edges.empty()) edges.push_back({0,1,potential[1]-potential[0]});
		if (!check(n,edges,(tc<small?"random ":"maximum ")+std::to_string(tc))) return 1;
	}
	std::cerr<<"ABC338F: "<<checks<<" passed; max "<<worst<<" seconds, "<<max_nodes
			 <<" nodes, "<<max_pivots<<" pivots, "<<max_solves<<" solves, "<<max_cuts<<" cuts\n";
}
