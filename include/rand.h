#ifndef RAND_H
#define RAND_H

#include <numeric>
#include <array>
#include <cmath>
#include <cassert>

class Rand {
public:
    explicit Rand(int seed) {
        x += (unsigned long)seed;
    }

    int pick(int n) {
        assert(n > 0);
        unsigned long r = xorshf96();
        if(n == 1) return 0;
        if(n == 2) return r % 2;
        if(n == 3) return r % 3;
        return r % n;
    }

    int pickWithBias(double p0, double p1, double p2) {
        std::array<double, 3> cumulative {{ p0, p0+p1, p0+p1+p2 }};
        double total = p0+p1+p2;
        int r = pick(std::numeric_limits<int>::max());
        double p = total * (double)r / std::numeric_limits<int>::max();
        for(int i = 0; i < 3; ++i) {
            if(cumulative[i] > p) return i;
        }
        assert(false);
        return 0;
    }

private:
    unsigned long x = 123456789;
    unsigned long y = 362436069;
    unsigned long z = 521288629;

    unsigned long xorshf96() { //period 2^96-1
        unsigned long t;
        x ^= x << 16;
        x ^= x >> 5;
        x ^= x << 1;

        t = x;
        x = y;
        y = z;
        z = t ^ x ^ y;

        return z;
    }
};

#endif