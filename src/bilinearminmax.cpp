#include "bilinearminmax.h"
#include "glpk.h"
#include <algorithm>
#include <limits>



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
    glp_simplex(ss.lp, &ss.parm);
    double z = glp_get_obj_val(ss.lp);
    double x1 = glp_get_col_prim(ss.lp, 1);
    double x2 = glp_get_col_prim(ss.lp, 2);
    double x3 = glp_get_col_prim(ss.lp, 3);
    
    return StrategyPoint { z, Point{x1, x2, x3} };
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
        return solveBetter(A);
    }
    Point a {0, 0, 0};
    a.p[std::distance(maxByRow.begin(), maxValueIt)] = 1;
    return StrategyPoint { *minValueIt, a };
}