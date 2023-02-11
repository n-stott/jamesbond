#include <array>
#include <limits>
#include <cassert>
#include <cstddef>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <vector>
#include "fmt/core.h"
#include "glpk.h"

struct Point { double p[3]; };

struct StrategyPoint {
    double value = 0.0;
    Point a {};
    Point b {};
};

struct BilinearMinMax {

    static constexpr double RESOLUTION = 1.0/6.0;

    static StrategyPoint solveBad(const std::array<std::array<double, 3>, 3>& A) {
        Point x;
        Point y;
        double bestXValue = +std::numeric_limits<double>::infinity();
        Point bestXPoint {0, 0, 0};
        Point bestXYPoint {0, 0, 0};
        for(x.p[0] = 0; x.p[0] <= 1; x.p[0] += RESOLUTION) {
            for(x.p[1] = 0; x.p[1] <= 1 - x.p[0]; x.p[1] += RESOLUTION) {
                x.p[2] = 1 - x.p[0] - x.p[1];
                double bestYValue = -std::numeric_limits<double>::infinity();
                Point bestYPoint {0, 0, 0};
                auto eval = [&](const Point& y) {
                    double val = 0.0;
                    for(int i = 0; i < 3; ++i) {
                        if(x.p[i] == 0) continue;
                        for(int j = 0; j < 3; ++j) {
                            if(y.p[j] == 0) continue;
                            assert(A[i][j] == A[i][j]);
                            val += A[i][j] * x.p[i] * y.p[j];
                        }
                    }
                    assert(val == val);
                    return val;
                };
                for(y.p[0] = 0; y.p[0] <= 1; y.p[0] += RESOLUTION) {
                    for(y.p[1] = 0; y.p[1] <= 1 - y.p[0]; y.p[1] += RESOLUTION) {
                        y.p[2] = 1 - y.p[0] - y.p[1];
                        double val = eval(y);
                        if(val > bestYValue) {
                            bestYValue = val;
                            bestYPoint = y;
                        }
                    }
                }
                if(bestYValue < bestXValue) {
                    bestXValue = bestYValue;
                    bestXPoint = x;
                    bestXYPoint = bestYPoint;
                }
            }
        }
        return StrategyPoint { bestXValue, bestXPoint, bestXYPoint };
    }

    static StrategyPoint solve(const std::array<std::array<double, 3>, 3>& A, size_t* pureSolves) {
        const double inf = std::numeric_limits<double>::infinity();
        std::array<double, 3> maxByRow {{ -inf, -inf, -inf }};
        std::array<double, 3> minByCol {{ +inf, +inf, +inf }};
        for(int i = 0; i < 3; ++i) {
            for(int j = 0; j < 3; ++j) {
                maxByRow[i] = std::max(maxByRow[i], A[i][j]);
                minByCol[j] = std::min(minByCol[j], A[i][j]);
            }
        }
        auto minValueIt = std::max_element(minByCol.begin(), minByCol.end());
        auto maxValueIt = std::min_element(maxByRow.begin(), maxByRow.end());
        if(*maxValueIt != *minValueIt) {
            return solveBad(A);
        }
        if(pureSolves) ++(*pureSolves);
        Point a {0, 0, 0};
        Point b {0, 0, 0};
        a.p[std::distance(maxByRow.begin(), maxValueIt)] = 1;
        b.p[std::distance(minByCol.begin(), minValueIt)] = 1;
        return StrategyPoint { *minValueIt, a, b };
    }

};

[[maybe_unused]] static inline void printCostMatrix(const std::array<std::array<double, 3>, 3>& A) {
    fmt::print("{:6} {:6} {:6}\n", A[0][0], A[0][1], A[0][2]);
    fmt::print("{:6} {:6} {:6}\n", A[1][0], A[1][1], A[1][2]);
    fmt::print("{:6} {:6} {:6}\n", A[2][0], A[2][1], A[2][2]);
}

struct SimplexSolver {

    SimplexSolver() {
        lp = glp_create_prob();
        glp_set_obj_dir(lp, GLP_MIN);
        glp_add_rows(lp, 4);
        glp_set_row_bnds(lp, 1, GLP_UP, 0.0, 0.0);
        glp_set_row_bnds(lp, 2, GLP_UP, 0.0, 0.0);
        glp_set_row_bnds(lp, 3, GLP_UP, 0.0, 0.0);
        glp_set_row_bnds(lp, 4, GLP_FX, 1.0, 1.0);
        glp_add_cols(lp, 4);
        glp_set_col_bnds(lp, 1, GLP_LO, 0.0, 1.0);
        glp_set_obj_coef(lp, 1, 0.0);
        glp_set_col_bnds(lp, 2, GLP_LO, 0.0, 1.0);
        glp_set_obj_coef(lp, 2, 0.0);
        glp_set_col_bnds(lp, 3, GLP_LO, 0.0, 1.0);
        glp_set_obj_coef(lp, 3, 0.0);
        glp_set_col_bnds(lp, 4, GLP_FR, 0.0, 0.0);
        glp_set_obj_coef(lp, 4, 1.0);

        glp_init_smcp(&parm);
        parm.meth = GLP_DUAL;
        parm.presolve = GLP_OFF;
        parm.r_test = GLP_RT_STD;
    }

    ~SimplexSolver() {
        glp_delete_prob(lp);
    }

    glp_prob* lp;
    glp_smcp parm;

    struct Sol {
        std::array<int, 4> rowstat;
        std::array<int, 4> colstat;
    };
};

static SimplexSolver ss;

static StrategyPoint solveSimplex(const std::array<std::array<double, 3>, 3>& A, SimplexSolver::Sol* sol, bool resolve = false) {
    int ia[1+15], ja[1+15];
    double ar[1+15];
    ia[1] = 1, ja[1] = 1, ar[1] = A[0][0];
    ia[2] = 1, ja[2] = 2, ar[2] = A[1][0];
    ia[3] = 1, ja[3] = 3, ar[3] = A[2][0];
    ia[4] = 1, ja[4] = 4, ar[4] = -1.0;
    ia[5] = 2, ja[5] = 1, ar[5] = A[0][1];
    ia[6] = 2, ja[6] = 2, ar[6] = A[1][1];
    ia[7] = 2, ja[7] = 3, ar[7] = A[2][1];
    ia[8] = 2, ja[8] = 4, ar[8] = -1.0;
    ia[9] = 3, ja[9] = 1, ar[9] = A[0][2];
    ia[10] = 3, ja[10] = 2, ar[10] = A[1][2];
    ia[11] = 3, ja[11] = 3, ar[11] = A[2][2];
    ia[12] = 3, ja[12] = 4, ar[12] = -1.0;
    ia[13] = 4, ja[13] = 1, ar[13] = 1.0;
    ia[14] = 4, ja[14] = 2, ar[14] = 1.0;
    ia[15] = 4, ja[15] = 3, ar[15] = 1.0;
    glp_load_matrix(ss.lp, 15, ia, ja, ar);
    glp_term_out(GLP_OFF);

    if(resolve && !!sol) {
        glp_set_row_stat(ss.lp, 1, sol->rowstat[0]);
        glp_set_row_stat(ss.lp, 2, sol->rowstat[1]);
        glp_set_row_stat(ss.lp, 3, sol->rowstat[2]);
        glp_set_row_stat(ss.lp, 4, sol->rowstat[3]);
        glp_set_col_stat(ss.lp, 1, sol->colstat[0]);
        glp_set_col_stat(ss.lp, 2, sol->colstat[1]);
        glp_set_col_stat(ss.lp, 3, sol->colstat[2]);
        glp_set_col_stat(ss.lp, 4, sol->colstat[3]);
    }

    glp_simplex(ss.lp, &ss.parm);
    double z = glp_get_obj_val(ss.lp);
    double x1 = glp_get_col_prim(ss.lp, 1);
    double x2 = glp_get_col_prim(ss.lp, 2);
    double x3 = glp_get_col_prim(ss.lp, 3);

    if(!!sol) {
        sol->rowstat[0] = glp_get_row_stat(ss.lp, 1);
        sol->rowstat[1] = glp_get_row_stat(ss.lp, 2);
        sol->rowstat[2] = glp_get_row_stat(ss.lp, 3);
        sol->rowstat[3] = glp_get_row_stat(ss.lp, 4);
        sol->colstat[0] = glp_get_col_stat(ss.lp, 1);
        sol->colstat[1] = glp_get_col_stat(ss.lp, 2);
        sol->colstat[2] = glp_get_col_stat(ss.lp, 3);
        sol->colstat[3] = glp_get_col_stat(ss.lp, 4);
    }
    
    return StrategyPoint { z, Point{x1, x2, x3}, Point{0, 0, 0} };
}

StrategyPoint solveBetter(const std::array<std::array<double, 3>, 3>& A, size_t* nbPure, SimplexSolver::Sol* sol, bool resolve = false) {
    const double inf = std::numeric_limits<double>::infinity();
    std::array<double, 3> maxByRow {{ -inf, -inf, -inf }};
    std::array<double, 3> minByCol {{ +inf, +inf, +inf }};
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
            maxByRow[i] = std::max(maxByRow[i], A[i][j]);
            minByCol[j] = std::min(minByCol[j], A[i][j]);
        }
    }
    auto minValueIt = std::max_element(minByCol.begin(), minByCol.end());
    auto maxValueIt = std::min_element(maxByRow.begin(), maxByRow.end());
    if(*maxValueIt == *minValueIt) {
        if(nbPure) ++(*nbPure);
        Point a {0, 0, 0};
        Point b {0, 0, 0};
        a.p[std::distance(maxByRow.begin(), maxValueIt)] = 1;
        b.p[std::distance(minByCol.begin(), minValueIt)] = 1;
        return StrategyPoint { *minValueIt, a, b };
    }

    return solveSimplex(A, sol, resolve);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {

    // size_t limit = (argc < 2 ? 1000000 : std::atoi(argv[1]));
    std::ifstream myfile;
    myfile.open("matrices");
    size_t totalTime = 0;
    size_t totalTime2 = 0;
    size_t totalTime3 = 0;
    size_t iterations = 0;
    size_t pureSolves = 0;
    size_t pureSolves2 = 0;
    while(!myfile.eof()) {
        std::array<std::array<double, 3>, 3> A;
        for(int i = 0; i < 3; ++i) {
            for(int j = 0; j < 3; ++j) {
                double val;
                myfile >> val;
                A[i][j] = val;
            }
        }
        std::chrono::steady_clock::time_point a = std::chrono::steady_clock::now();
        BilinearMinMax::solve(A, &pureSolves);
        std::chrono::steady_clock::time_point b = std::chrono::steady_clock::now();
        SimplexSolver::Sol sol;
        solveBetter(A, &pureSolves2, &sol, false);
        std::chrono::steady_clock::time_point c = std::chrono::steady_clock::now();
        solveBetter(A, &pureSolves2, &sol, true);
        std::chrono::steady_clock::time_point d = std::chrono::steady_clock::now();
        // if((pureSolves != pureSolves2) || (s.value != t)) {
        //     iterations += 1;
        //     fmt::print("{} {}\n", s.value, t);
        //     fmt::print("approx strat={} {} {}\n", s.a.p[0], s.a.p[1], s.a.p[2]);
        //     printCostMatrix(A);
        // }
        totalTime += (b-a).count();
        totalTime2 += (c-b).count();
        totalTime3 += (d-c).count();
        iterations += 1;
        // if(iterations >= limit) break;
        if(iterations % 1000 == 0) {
            fmt::print("{} : {} ns (~{} us per)\n", iterations, totalTime, totalTime/iterations/1000.0);
            fmt::print("{} : {} ns (~{} us per)\n", iterations, totalTime2, totalTime2/iterations/1000.0);
            fmt::print("{} : {} ns (~{} us per)\n", iterations, totalTime3, totalTime3/iterations/1000.0);
            // fmt::print("{} {}\n", pureSolves, pureSolves2);
        }
    }
}