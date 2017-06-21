#include "Gauss.h"

namespace Gauss {
// should use a better function than this (box muller method - stolen from the internet)
double rand_normal(double mean, double sigma, bool can_be_negative) {
    static double n2 = 0.0;
    static int n2_cached = 0;
    if (!n2_cached) {
        double x, y, r;
        do {
            x = 2.0 * rand() / RAND_MAX - 1;
            y = 2.0 * rand() / RAND_MAX - 1;

            r = x * x + y * y;
        } while (r == 0.0 || r > 1.0);
        double d = sqrt(-2.0 * log(r) / r);
        double n1 = x * d;
        n2 = y * d;
        double result = n1 * sigma + mean;
        n2_cached = 1;
        if (!can_be_negative && result < 0) {
            rand_normal(mean, sigma, can_be_negative);
        }
        return result;
    } else {
        n2_cached = 0;
        if (!can_be_negative && n2 * sigma + mean < 0) {
            rand_normal(mean, sigma, can_be_negative);
        }
        return n2 * sigma + mean;
    }
}
}
