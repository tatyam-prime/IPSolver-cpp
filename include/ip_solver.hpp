#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

// Optimize c*x, subject to linear constraints and variable bounds. C++17.
namespace ip {
using Vec = std::vector<double>;
inline constexpr double INF = std::numeric_limits<double>::infinity();
enum class Status { Optimal, Infeasible, UnboundedRelaxation, Limit, NumericalError };
struct Options {
    double eps = 1e-9, integer_eps = 1e-7, time_limit = INF;
    uint64_t node_limit = UINT64_MAX, pivot_limit = 1000000;
    int strong_branching = 3, cuts = 8; // Root GMI cuts; 0 disables either feature.
    Vec initial_solution; // Optional feasible solution in original coordinates.
};
struct Result {
    Status status = Status::Infeasible;
    double objective = -INF, bound = INF;
    Vec x;
    uint64_t nodes = 0, pivots = 0;
    bool has_solution() const { return std::isfinite(objective); }
};

class Solver {
    int n;
    Vec c, b, lo, hi;
    std::vector<Vec> a;
    std::vector<bool> integer;
    static void require(bool ok) {
        if (!ok) throw std::invalid_argument("ip::Solver: invalid input");
    }
    struct Work {
        const Options& o;
        Result& result;
        uint64_t dual_limit = UINT64_MAX;
        bool retry = false;
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        bool timeout() const {
            return std::isfinite(o.time_limit) &&
                std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count() >= o.time_limit;
        }
        bool node() {
            if (result.nodes >= o.node_limit || timeout()) return false;
            ++result.nodes;
            return true;
        }
    };
    // Dictionary: x_B = rhs - D*x_N, z = rhs - D_obj*x_N.
    struct LP {
        int n, m, next;
        std::vector<int> B, N;
        std::vector<std::pair<int,double>> path;
        std::vector<Vec> d;
        LP(const std::vector<Vec>& a, const Vec& b, const Vec& c)
            : n(int(c.size())), m(int(b.size())), next(n+m), B(m), N(n+1),
              d(m+2, Vec(n+2)) {
            std::iota(B.begin(), B.end(), n);
            std::iota(N.begin(), N.end(), 0);
            N[n] = -1;
            for (int i=0; i<m; ++i) {
                std::copy(a[i].begin(), a[i].end(), d[i].begin());
                d[i][n] = -1;
                d[i][n+1] = b[i];
            }
            for (int j=0; j<n; ++j) d[m][j] = -c[j];
            d[m+1][n] = 1;
        }
        bool pivot(int r, int s, Work& w) {
            if (w.result.pivots >= w.o.pivot_limit || w.timeout()) return false;
            ++w.result.pivots;
            double inv = 1/d[r][s];
            for (int i=0; i<m+2; ++i) if (i!=r) {
                double k = d[i][s]*inv;
                if (k) for (int j=0; j<n+2; ++j) if (j!=s) d[i][j] -= d[r][j]*k;
                d[i][s] = -k;
            }
            for (int j=0; j<n+2; ++j) if (j!=s) d[r][j] *= inv;
            d[r][s] = inv;
            std::swap(B[r], N[s]);
            return true;
        }
        Status primal(int phase, Work& w) {
            int z = m+phase, stalled = 0;
            double eps = w.o.eps;
            // Give pricing a tableau-sized run before falling back to Bland.
            for (;;) {
                int s=-1, r=-1;
                for (int j=0; j<=n; ++j) if ((phase || N[j]!=-1) && d[z][j]<-eps)
                    if (s<0 || (stalled>=std::max(20,n+m) ? N[j]<N[s] :
                        d[z][j]<d[z][s]-eps || (std::abs(d[z][j]-d[z][s])<=eps && N[j]<N[s]))) s=j;
                if (s<0) {
                    for (int i=0; i<m; ++i) if (!std::isfinite(d[i][n+1]) || d[i][n+1]<-w.o.integer_eps)
                        return Status::NumericalError;
                    return std::isfinite(d[z][n+1]) ? Status::Optimal : Status::NumericalError;
                }
                // Harris' two-pass ratio test: prefer a large pivot within feasibility tolerance.
                double step=INF;
                for (int i=0; i<m; ++i) if (d[i][s]>eps)
                    step=std::min(step,(std::max(0.0,d[i][n+1])+eps)/d[i][s]);
                for (int i=0; i<m; ++i) if (d[i][s]>eps) {
                    double v=d[i][n+1]/d[i][s];
                    double old=r<0 ? INF : d[r][n+1]/d[r][s];
                    if (stalled>=std::max(20,n+m) ? r<0 || v<old || (v==old && B[i]<B[r]) :
                        v<=step && (r<0 || d[i][s]>d[r][s] || (d[i][s]==d[r][s] && B[i]<B[r]))) r=i;
                }
                if (r<0) {
                    for (int i=0; i<m; ++i) if (d[i][s]>0)
                        return Status::NumericalError;
                    return Status::UnboundedRelaxation;
                }
                double old=d[z][n+1];
                if (!pivot(r,s,w)) return Status::Limit;
                stalled = d[z][n+1]>old+eps ? 0 : stalled+1;
            }
        }
        Status root(Work& w) {
            if (!w.node()) return Status::Limit;
            int r=-1;
            for (int i=0; i<m; ++i) if (r<0 || d[i][n+1]<d[r][n+1]) r=i;
            if (r>=0 && d[r][n+1]<-w.o.eps) {
                if (!pivot(r,n,w)) return Status::Limit;
                Status s=primal(1,w);
                if (s==Status::Limit || s==Status::NumericalError) return s;
                if (s!=Status::Optimal) return Status::NumericalError;
                if (d[m+1][n+1]<-w.o.eps) return Status::Infeasible;
                if (std::abs(d[m+1][n+1])>w.o.eps) return Status::NumericalError;
                for (int i=0; i<m; ++i) if (B[i]==-1) {
                    int j=-1;
                    for (int k=0; k<=n; ++k) if (std::abs(d[i][k])>w.o.eps &&
                        (j<0 || std::abs(d[i][k])>std::abs(d[i][j]))) j=k;
                    if (j>=0 && !pivot(i,j,w)) return Status::Limit;
                }
            }
            return primal(0,w);
        }
        // Add sign*x_k <= sign*t in the current basis, retaining dual feasibility.
        void branch(int k, double t, int sign) {
            path.push_back({sign>0 ? k : -k-1,t});
            Vec row(n+2);
            row[n+1]=sign*t;
            for (int j=0; j<=n; ++j) if (N[j]==k) row[j]=sign;
            for (int i=0; i<m; ++i) if (B[i]==k) {
                for (int j=0; j<=n; ++j) row[j]=-sign*d[i][j];
                row[n+1]-=sign*d[i][n+1];
            }
            d.insert(d.begin()+m, std::move(row));
            B.push_back(next++);
            ++m;
        }
        Status dual(Work& w) {
            if (!w.node()) return Status::Limit;
            uint64_t start=w.result.pivots;
            int stalled=0;
            double eps=w.o.eps;
            for (;;) {
                int r=-1, s=-1;
                for (int i=0; i<m; ++i) if (d[i][n+1]<-eps &&
                    (r<0 || (stalled>=20 ? B[i]<B[r] : d[i][n+1]<d[r][n+1]))) r=i;
                if (r<0) return std::isfinite(value()) ? Status::Optimal : Status::NumericalError;
                if (w.result.pivots-start>=w.dual_limit) { w.retry=true; return Status::Limit; }
                double step=INF;
                for (int j=0; j<=n; ++j) if (N[j]!=-1 && d[r][j]<-eps)
                    step=std::min(step,(std::max(0.0,d[m][j])+eps)/(-d[r][j]));
                for (int j=0; j<=n; ++j) if (N[j]!=-1 && d[r][j]<-eps) {
                    double v=d[m][j]/(-d[r][j]);
                    double old=s<0 ? INF : d[m][s]/(-d[r][s]);
                    if (stalled>=20 ? s<0 || v<old || (v==old && N[j]<N[s]) :
                        v<=step && (s<0 || d[r][j]<d[r][s] || (d[r][j]==d[r][s] && N[j]<N[s]))) s=j;
                }
                if (s<0) {
                    double noise=0;
                    for (int j=0; j<=n; ++j) noise=std::max(noise,std::abs(d[r][j]));
                    noise=64*std::numeric_limits<double>::epsilon()*std::max(1.0,noise);
                    for (int j=0; j<=n; ++j) if (N[j]!=-1 && d[r][j]<-noise)
                        return Status::NumericalError;
                    return Status::Infeasible;
                }
                double old=value();
                if (!pivot(r,s,w)) return Status::Limit;
                stalled = value()<old-eps ? 0 : stalled+1;
            }
        }
        bool cut(const std::vector<bool>& integer) {
            int r=-1;
            double best=0.01;
            for (int i=0; i<m; ++i) if (B[i]>=0 && B[i]<n && integer[B[i]]) {
                double f=d[i][n+1]-std::floor(d[i][n+1]);
                double score=std::min(f,1-f);
                if (score>best && std::all_of(d[i].begin(),d[i].begin()+n+1,
                    [](double v){ return std::abs(v)<1e6; })) { best=score; r=i; }
            }
            if (r<0) return false;
            double f=d[r][n+1]-std::floor(d[r][n+1]);
            Vec row(n+2);
            for (int j=0; j<=n; ++j) if (N[j]!=-1) {
                double v=d[r][j], g;
                if (N[j]<n && integer[N[j]]) {
                    double t=v-std::floor(v);
                    g=std::min(t/f,(1-t)/(1-f));
                } else g=v>=0 ? v/f : -v/(1-f);
                row[j]=-g;
            }
            row[n+1]=-1;
            d.insert(d.begin()+m,std::move(row));
            B.push_back(next++); ++m;
            return true;
        }
        double value() const { return d[m][n+1]; }
        Vec solution() const {
            Vec x(n);
            for (int i=0; i<m; ++i) if (B[i]>=0 && B[i]<n) x[B[i]]=d[i][n+1];
            return x;
        }
    };
public:
    explicit Solver(Vec objective) : n(int(objective.size())), c(std::move(objective)),
        lo(n), hi(n,INF), integer(n,true) {
        for (double v:c) require(std::isfinite(v));
    }
    void add_le(Vec row, double rhs) {
        require(int(row.size())==n && std::isfinite(rhs));
        for (double v:row) require(std::isfinite(v));
        a.push_back(std::move(row)); b.push_back(rhs);
    }
    void add_ge(Vec row, double rhs) {
        for (double& v:row) v=-v;
        add_le(std::move(row),-rhs);
    }
    void add_eq(Vec row, double rhs) { add_le(row,rhs); add_ge(std::move(row),rhs); }
    void bounds(int i, double lower, double upper=INF) {
        require(i>=0 && i<n && std::isfinite(lower) && (std::isfinite(upper) || upper==INF));
        lo[i]=lower; hi[i]=upper;
    }
    void continuous(int i) { require(i>=0 && i<n); integer[i]=false; }
    Result maximize(const Options& o=Options()) const { return run(c,o); }
    Result minimize(const Options& o=Options()) const {
        Vec objective=c;
        for (double& v:objective) v=-v;
        Result r=run(objective,o);
        r.objective=0-r.objective; r.bound=0-r.bound;
        return r;
    }
private:
    Result run(const Vec& c, const Options& o) const {
        require(o.eps>0 && std::isfinite(o.eps) && o.integer_eps>=o.eps &&
            std::isfinite(o.integer_eps) && o.integer_eps<0.5 && o.time_limit>=0 && o.strong_branching>=0 && o.cuts>=0);
        Result ans;
        Work w{o,ans};
        Vec lower=lo, upper=hi;
        bool integral_objective=true;
        for (int j=0; j<n; ++j) {
            if (integer[j]) { lower[j]=std::ceil(lo[j]); upper[j]=std::floor(hi[j]); }
            if (lower[j]>upper[j]) { ans.bound=-INF; return ans; }
            if ((integer[j] && c[j]!=std::round(c[j])) || (!integer[j] && c[j]!=0)) integral_objective=false;
        }
        auto feasible=[&](const Vec& x) {
            for (int j=0; j<n; ++j)
                if (!std::isfinite(x[j]) || x[j]<lo[j]-o.integer_eps || x[j]>hi[j]+o.integer_eps) return false;
            for (int i=0; i<int(a.size()); ++i) {
                long double v=0, scale=1+std::abs(b[i]);
                for (int j=0; j<n; ++j) { v+=(long double)a[i][j]*x[j]; scale+=std::abs((long double)a[i][j]*x[j]); }
                if (v>b[i]+o.eps+8*std::numeric_limits<double>::epsilon()*scale) return false;
            }
            return true;
        };
        auto accept=[&](Vec x, bool shifted=true) {
            for (int j=0; j<n; ++j) {
                if (shifted) x[j]+=lower[j];
                if (integer[j]) x[j]=std::round(x[j]);
            }
            if (!feasible(x)) return false;
            double v=std::inner_product(c.begin(),c.end(),x.begin(),0.0);
            if (!std::isfinite(v)) return false;
            if (!ans.has_solution() || v>ans.objective) { ans.objective=v; ans.x=std::move(x); }
            return true;
        };
        if (!o.initial_solution.empty()) {
            require(int(o.initial_solution.size())==n);
            for (int j=0; j<n; ++j) if (integer[j])
                require(std::abs(o.initial_solution[j]-std::round(o.initial_solution[j]))<=o.integer_eps);
            require(accept(o.initial_solution,false));
        }
        if (!o.node_limit || w.timeout()) { ans.status=Status::Limit; return ans; }
        auto A=a;
        Vec rhs=b;
        Vec implied(n,INF);
        for (int i=0; i<int(A.size()); ++i) {
            int64_t g=0;
            bool lattice=true;
            for (int j=0; j<n; ++j) if (A[i][j]!=0) {
                double v=std::abs(A[i][j]);
                if (!integer[j] || v!=std::floor(v) || v>1e12) { lattice=false; break; }
                g=std::gcd(g,int64_t(v));
            }
            if (lattice && g) {
                for (double& v:A[i]) v/=g;
                rhs[i]=std::floor(rhs[i]/g);
            }
            double scale=0;
            for (int j=0; j<n; ++j) { rhs[i]-=A[i][j]*lower[j]; scale=std::max(scale,std::abs(A[i][j])); }
            if (scale) {
                double small=INF;
                for (double v:A[i]) if (v) small=std::min(small,std::abs(v));
                scale=std::min(scale,small);
                for (double& v:A[i]) v/=scale;
                rhs[i]/=scale;
            }
            // Only original rows imply these bounds, so removing bounds cannot be circular.
            if (std::all_of(A[i].begin(),A[i].end(),[](double v){ return v>=0; }))
                for (int j=0; j<n; ++j) if (A[i][j]>0)
                    implied[j]=std::min(implied[j],rhs[i]/A[i][j]);
        }
        for (int j=0; j<n; ++j) if (std::isfinite(upper[j]) && upper[j]-lower[j]<implied[j]) {
            Vec row(n); row[j]=1;
            A.push_back(std::move(row)); rhs.push_back(upper[j]-lower[j]);
        }
        double offset=std::inner_product(c.begin(),c.end(),lower.begin(),0.0);
        double objective_scale=1;
        for (int j=0; j<n; ++j) objective_scale+=std::abs(c[j])*
            (1+std::abs(lower[j])+(std::isfinite(upper[j]) ? std::abs(upper[j]) : 0));
        auto checked=[&](const LP& p, Status status) {
            if (status!=Status::Optimal) return status;
            for (int i=0; i<p.m; ++i) if (!std::isfinite(p.d[i][p.n+1]) || p.d[i][p.n+1]<-o.integer_eps)
                return Status::NumericalError;
            double tolerance=o.eps+64*std::numeric_limits<double>::epsilon()*objective_scale;
            for (int j=0; j<=n; ++j) if (p.N[j]!=-1 &&
                (!std::isfinite(p.d[p.m][j]) || p.d[p.m][j]<-tolerance)) return Status::NumericalError;
            Vec x=p.solution();
            for (auto [k,t]:p.path) if ((k>=0 ? x[k]-t : t-x[-k-1])>o.integer_eps)
                return Status::NumericalError;
            long double value=0, scale=objective_scale+std::abs(p.value());
            for (int j=0; j<n; ++j) { value+=(long double)c[j]*x[j]; scale+=std::abs((long double)c[j]*x[j]); x[j]+=lower[j]; }
            if (!std::isfinite(p.value()) || std::abs(value-p.value())>o.eps+64*std::numeric_limits<double>::epsilon()*scale)
                return Status::NumericalError;
            return feasible(x) ? status : Status::NumericalError;
        };
        LP root(A,rhs,c);
        ans.status=checked(root,root.root(w));
        if (ans.status!=Status::Optimal) {
            if (ans.status==Status::Infeasible && ans.has_solution()) ans.status=Status::NumericalError;
            if (ans.status==Status::Infeasible) ans.bound=-INF;
            return ans;
        }
        auto bound=[&](const LP& p) {
            double v=p.value()+offset;
            v+=o.eps+64*std::numeric_limits<double>::epsilon()*(objective_scale+std::abs(p.value())+std::abs(offset));
            return integral_objective ? std::floor(v+o.eps) : v;
        };
        ans.bound=bound(root);
        if (!std::isfinite(ans.bound)) { ans.status=Status::NumericalError; return ans; }
        auto search=[&](LP root, int cuts) {
            uint64_t first_node=ans.nodes-1;
            ans.status=Status::Optimal;
            ans.bound=bound(root);
            for (int k=0; k<cuts && ans.bound>ans.objective+o.eps && root.cut(integer); ++k) {
                w.dual_limit=std::max<uint64_t>(1000,10*uint64_t(n+root.m));
                ans.status=checked(root,root.dual(w));
                if (ans.status!=Status::Optimal) {
                    if (ans.status==Status::Infeasible && ans.has_solution()) ans.status=Status::NumericalError;
                    if (ans.status==Status::Infeasible) ans.bound=-INF;
                    return;
                }
                ans.bound=bound(root);
            }
            Vec pc[2]={Vec(n,1),Vec(n,1)};
            std::vector<int> count[2]={std::vector<int>(n),std::vector<int>(n)};
            std::vector<LP> stack;
            stack.push_back(std::move(root));
            while (!stack.empty()) {
                LP p=std::move(stack.back()); stack.pop_back();
                if (bound(p)<=ans.objective+o.eps) continue;
                if (w.timeout()) { ans.status=Status::Limit; return; }
                Vec x=p.solution();
                std::vector<std::pair<double,int>> candidates;
                for (int j=0; j<n; ++j) if (integer[j]) {
                    double f=x[j]-std::floor(x[j]);
                    if (std::min(f,1-f)>o.integer_eps) {
                        double d=pc[0][j]*f, u=pc[1][j]*(1-f);
                        candidates.push_back({std::min(d,u)+0.1*std::max(d,u),j});
                    }
                }
                if (candidates.empty()) {
                    if (!accept(x)) { ans.status=Status::NumericalError; return; }
                    continue;
                }
                accept(x);
                for (int dir=0; dir<2; ++dir) {
                    Vec y=x;
                    for (int j=0; j<n; ++j) if (integer[j]) y[j]=dir ? std::ceil(x[j]) : std::floor(x[j]);
                    accept(std::move(y));
                }
                if (bound(p)<=ans.objective+o.eps) continue;
                std::sort(candidates.rbegin(),candidates.rend());
                int probes=std::min(int(candidates.size()), o.strong_branching && ans.nodes-first_node<32 ? o.strong_branching : 1);
                double best=-1;
                std::vector<LP> children;
                for (int t=0; t<probes; ++t) {
                    int j=candidates[t].second;
                    double f=x[j]-std::floor(x[j]), gain[2];
                    std::vector<LP> trial;
                    for (int dir=0; dir<2; ++dir) {
                        LP q=p;
                        q.branch(j, dir ? std::ceil(x[j]) : std::floor(x[j]), dir ? -1 : 1);
                        Status s=checked(q,q.dual(w));
                        if (s==Status::NumericalError) {
                            // Rebuild this branch once without cuts; Work retains every global budget.
                            auto rows=A; Vec bounds=rhs;
                            for (auto [k,t]:q.path) {
                                Vec row(n); int sign=k>=0 ? 1 : -1;
                                row[k>=0 ? k : -k-1]=sign;
                                rows.push_back(std::move(row)); bounds.push_back(sign*t);
                            }
                            LP fresh(rows,bounds,c);
                            fresh.path=std::move(q.path);
                            s=checked(fresh,fresh.root(w));
                            if (s==Status::UnboundedRelaxation) s=Status::NumericalError; // The parent LP was bounded.
                            q=std::move(fresh);
                        }
                        if (s==Status::Limit || s==Status::NumericalError) { ans.status=s; return; }
                        gain[dir]=s==Status::Infeasible ? 1e100 : std::max(0.0,p.value()-q.value());
                        if (s==Status::Optimal) {
                            double cost=gain[dir]/(dir ? 1-f : f);
                            pc[dir][j]=(pc[dir][j]*count[dir][j]+cost)/(count[dir][j]+1);
                            ++count[dir][j];
                            if (bound(q)>ans.objective+o.eps) trial.push_back(std::move(q));
                        }
                    }
                    double score=std::min(gain[0],gain[1])+0.1*std::max(gain[0],gain[1]);
                    if (score>best || trial.empty()) { best=score; children=std::move(trial); }
                    if (children.empty()) break;
                }
                std::sort(children.begin(),children.end(),[&](const LP& u,const LP& v){ return bound(u)<bound(v); });
                for (auto& q:children) stack.push_back(std::move(q));
            }
            ans.status=ans.has_solution() ? Status::Optimal : Status::Infeasible;
            ans.bound=ans.objective;
        };
        if (!o.cuts || objective_scale>=1e10) search(std::move(root),0);
        else {
            search(root,o.cuts);
            // Discard all cuts and descendants, keeping the incumbent and total budgets.
            if ((w.retry || ans.status==Status::NumericalError) && !w.timeout() &&
                ans.nodes<o.node_limit && ans.pivots<o.pivot_limit) {
                w.dual_limit=UINT64_MAX;
                search(std::move(root),0);
            }
        }
        return ans;
    }
};
} // namespace ip
