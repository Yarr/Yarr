#include "scurvegauss.h"
#include <cmath>

void scurvegauss(double* par, const int n_points, const double* x, const double *y){
    // Offset (par[3]) is the first y-value
    par[3] = y[0];

    // Loop over the x values, calculating y-differences between
    // neighbouring points (derivative, which maps out a Gaussian)
    // and the corresponding moments

    double mean  = 0.;
    double var   = 0.;
    double norm  = 0.;

    for (int i = 0; i < n_points - 1; i++){
        const double diff = y[i + 1] - y[i];
        mean  += (x[i] + 0.5) * diff;
        var   += (x[i] + 0.5) * (x[i] + 0.5) * diff;
        norm  += diff;
    }

    // normalize mean and var
    mean /= norm;
    var  /= norm;

    // assign the values to the par array
    par[0] = mean;
    par[1] = std::sqrt(var - mean*mean);
    par[2] = y[0] + norm;
}