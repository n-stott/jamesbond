#include "bilinearminmax.h"
#include "fmt/core.h"
#include "glpk.h"
#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>



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
};

static SimplexSolver ss;


[[maybe_unused]] static inline void printCostMatrix(const std::array<std::array<double, 3>, 3>& A) {
    fmt::print("{:6} {:6} {:6}\n", A[0][0], A[0][1], A[0][2]);
    fmt::print("{:6} {:6} {:6}\n", A[1][0], A[1][1], A[1][2]);
    fmt::print("{:6} {:6} {:6}\n", A[2][0], A[2][1], A[2][2]);
}

static double evalCandidate(const std::array<std::array<double, 3>, 3>& A, const Point& p) {
    if(p.p[0] != p.p[0] || p.p[0] <= -0.0000001 || p.p[0] >= 1.0000001) return std::numeric_limits<double>::infinity();
    if(p.p[1] != p.p[1] || p.p[1] <= -0.0000001 || p.p[1] >= 1.0000001) return std::numeric_limits<double>::infinity();
    if(p.p[2] != p.p[2] || p.p[2] <= -0.0000001 || p.p[2] >= 1.0000001) return std::numeric_limits<double>::infinity();
    if(p.p[0] + p.p[1] + p.p[2] >= 1.0000001) return std::numeric_limits<double>::infinity();
    if(p.p[0] + p.p[1] + p.p[2] <= 1-0.0000001) return std::numeric_limits<double>::infinity();
    double value = -std::numeric_limits<double>::infinity();
    for(int j = 0; j < 3; ++j) {
        double v = 0;
        for(int i = 0; i < 3; ++i) {
            v += A[i][j] * p.p[i];
        }
        value = std::max(value, v);
    }
    return value;
};

StrategyPoint BilinearMinMax::solveBetter(const std::array<std::array<double, 3>, 3>& A) {
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
    glp_std_basis(ss.lp);
    glp_simplex(ss.lp, &ss.parm);
    double z = glp_get_obj_val(ss.lp);
    double x1 = glp_get_col_prim(ss.lp, 1);
    double x2 = glp_get_col_prim(ss.lp, 2);
    double x3 = glp_get_col_prim(ss.lp, 3);

    Point p{x1, x2, x3};
    return StrategyPoint { z, p };
}

double det(double mat[3][3]) {
    double ans = mat[0][0] * (mat[1][1] * mat[2][2] - mat[2][1] * mat[1][2])
                - mat[0][1] * (mat[1][0] * mat[2][2] - mat[1][2] * mat[2][0])
                + mat[0][2] * (mat[1][0] * mat[2][1] - mat[1][1] * mat[2][0]);
    return ans;
}

std::array<double, 3> solve3x3(const std::array<std::array<double, 3>, 3>& A, const std::array<double, 3>& b) {
    double d[3][3] = {
        { A[0][0], A[0][1], A[0][2] },
        { A[1][0], A[1][1], A[1][2] },
        { A[2][0], A[2][1], A[2][2] },
    };
    double d1[3][3] = {
        { b[0], A[0][1], A[0][2] },
        { b[1], A[1][1], A[1][2] },
        { b[2], A[2][1], A[2][2] },
    };
    double d2[3][3] = {
        { A[0][0], b[0], A[0][2] },
        { A[1][0], b[1], A[1][2] },
        { A[2][0], b[2], A[2][2] },
    };
    double d3[3][3] = {
        { A[0][0], A[0][1], b[0] },
        { A[1][0], A[1][1], b[1] },
        { A[2][0], A[2][1], b[2] },
    };
 
    double D = det(d);
    double D1 = det(d1);
    double D2 = det(d2);
    double D3 = det(d3);
 
    if (D != 0) {
        double x = D1 / D;
        double y = D2 / D;
        double z = D3 / D;
        return std::array<double, 3>{x, y, z};
    } else {
        return std::array<double, 3>{0, 0, 0};
    }
}

StrategyPoint BilinearMinMax::solveFast(const std::array<std::array<double, 3>, 3>& A) {
    std::vector<Point> candidates;

    candidates.push_back(Point{ (A[1][1]-A[1][0])/(A[1][1]-A[1][0]+A[0][0]-A[0][1]), (A[0][0]-A[0][1])/(A[1][1]-A[1][0]+A[0][0]-A[0][1]), 0 });
    candidates.push_back(Point{ (A[1][2]-A[1][0])/(A[1][2]-A[1][0]+A[0][0]-A[0][2]), (A[0][0]-A[0][2])/(A[1][2]-A[1][0]+A[0][0]-A[0][2]), 0 });
    candidates.push_back(Point{ (A[1][1]-A[1][2])/(A[1][1]-A[1][2]+A[0][2]-A[0][1]), (A[0][2]-A[0][1])/(A[1][1]-A[1][2]+A[0][2]-A[0][1]), 0 });

    candidates.push_back(Point{ (A[2][1]-A[2][0])/(A[2][1]-A[2][0]+A[0][0]-A[0][1]), 0, (A[0][0]-A[0][1])/(A[2][1]-A[2][0]+A[0][0]-A[0][1]) });
    candidates.push_back(Point{ (A[2][2]-A[2][0])/(A[2][2]-A[2][0]+A[0][0]-A[0][2]), 0, (A[0][0]-A[0][2])/(A[2][2]-A[2][0]+A[0][0]-A[0][2]) });
    candidates.push_back(Point{ (A[2][1]-A[2][2])/(A[2][1]-A[2][2]+A[0][2]-A[0][1]), 0, (A[0][2]-A[0][1])/(A[2][1]-A[2][2]+A[0][2]-A[0][1]) });

    candidates.push_back(Point{ 0, (A[2][0]-A[2][1])/(A[1][1]-A[1][0]+A[2][0]-A[2][1]), (A[1][1]-A[1][0])/(A[1][1]-A[1][0]+A[2][0]-A[2][1]) });
    candidates.push_back(Point{ 0, (A[2][0]-A[2][2])/(A[1][2]-A[1][0]+A[2][0]-A[2][2]), (A[1][2]-A[1][0])/(A[1][2]-A[1][0]+A[2][0]-A[2][2]) });
    candidates.push_back(Point{ 0, (A[2][2]-A[2][1])/(A[1][1]-A[1][2]+A[2][2]-A[2][1]), (A[1][1]-A[1][2])/(A[1][1]-A[1][2]+A[2][2]-A[2][1]) });

    std::array<std::array<double, 3>, 3> M {{
        { A[0][0] - A[0][1], A[1][0] - A[1][1], A[2][0] - A[2][1] },
        { A[0][1] - A[0][2], A[1][1] - A[1][2], A[2][1] - A[2][2] },
        { 1, 1, 1 }
    }};
    std::array<double, 3> b {{ 0, 0, 1 }};
    auto s = solve3x3(M, b);
    candidates.push_back(Point{s[0], s[1], s[2]});

    Point bestPoint {0, 0, 0};
    double bestValue = std::numeric_limits<double>::infinity();

    for(const auto& p : candidates) {
        double value = evalCandidate(A, p);
        if(value < bestValue) {
            bestValue = value;
            bestPoint = p;
        }
    }
   
    return StrategyPoint { bestValue, bestPoint };
}

StrategyPoint BilinearMinMax::solve(const std::array<std::array<double, 3>, 3>& A) {
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
        auto f = solveFast(A);
        return f;
        // auto b = solveBetter(A);
        // return b;
    }
    Point a {0, 0, 0};
    a.p[std::distance(maxByRow.begin(), maxValueIt)] = 1;
    return StrategyPoint { *minValueIt, a };
}