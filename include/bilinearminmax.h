#ifndef BILINEARMINMAX_H
#define BILINEARMINMAX_H

#include <array>

struct Point { double p[3]; };

struct StrategyPoint {
    double value = 0.0;
    Point p {};
};

struct CombinedStrategyPoint {
    double value = 0.0;
    Point p {};
};


class BilinearMinMax {
private:
    static StrategyPoint solveBetter(const std::array<std::array<double, 3>, 3>& A);
public:
    static StrategyPoint solve(const std::array<std::array<double, 3>, 3>& A);
};


#endif