#include "ip_solver.hpp"
#include <chrono>
#include <iostream>

int main(int argc,char** argv) {
    int h=argc>1?std::stoi(argv[1]):48,w=argc>2?std::stoi(argv[2]):48;
    double limit=argc>3?std::stod(argv[3]):1;
    auto start=std::chrono::steady_clock::now();
    std::vector<std::pair<int,int>> edges;
    for (int i=0;i<h;++i) for (int j=0;j<w;++j) {
        if (i+1<h) edges.push_back({i*w+j,(i+1)*w+j});
        if (j+1<w) edges.push_back({i*w+j,i*w+j+1});
    }
    int n=h*w,m=int(edges.size());
    ip::Solver solver(ip::Vec(m,1));
    for (int v=0;v<n;++v) {
        ip::Vec row(m);
        for (int j=0;j<m;++j) row[j]=edges[j].first==v || edges[j].second==v;
        solver.add_le(std::move(row),1);
    }
    auto model=std::chrono::steady_clock::now();
    ip::Options options; options.cuts=0; options.time_limit=limit;
    auto result=solver.maximize(options);
    auto finish=std::chrono::steady_clock::now();
    std::cout<<"h,w,variables,rows,status,objective,bound,lp_solves,pivots,model_ms,total_ms\n"
             <<h<<','<<w<<','<<m<<','<<n<<','<<int(result.status)<<','<<result.objective<<','
             <<result.bound<<','<<result.nodes<<','<<result.pivots<<','
             <<1000*std::chrono::duration<double>(model-start).count()<<','
             <<1000*std::chrono::duration<double>(finish-start).count()<<'\n';
}
