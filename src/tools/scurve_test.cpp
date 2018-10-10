// Program to test closure of s-curve fitting

#include <iostream>
#include <array>
#include <cmath>
#include "lmcurve.h"
#include "Gauss.h"

// Errorfunction
// par[0] = Mean
// par[1] = Sigma
// par[2] = Normlization
#define SQRT2 1.414213562
double scurve(double x, const double *par) {
    return 0.5*(2-erfc((x-par[0])/(par[1]*SQRT2)))*par[2];
}

int main(int argc, char* argv[]) {

    // Generate samples
    const double step_size = 10;
    const unsigned steps = 300;
    const unsigned n_samples = 1000;
    std::array<std::array<double, steps>, n_samples> sample;
    for (unsigned i=0; i<n_samples; i++) {
        for (unsigned j=0; j<steps; j++) {
            sample[i][j] = 0;
        }
    }
    
    
    std::array<double, steps> x;
    for (unsigned i=0; i<steps; i++) {
        x[i] = i*step_size;
    }

    std::cout << "Settings:" << std::endl;
    unsigned n_injections = 100;
    double threshold = 1500;
    double noise = 50;
    std::cout << "- Number of injections: " << n_injections << std::endl;
    std::cout << "- Target threshold: " << threshold << std::endl;
    std::cout << "- Target ENC: " << noise << std::endl;
    std::cout << "- Vcal bins: " << steps << std::endl;
    std::cout << "- Vcal step size: " << step_size << std::endl;
    std::cout << "- Number of samples: " << n_samples << std::endl;

    std::cout << "Generate samples ..." << std::endl;
    for (unsigned i=0; i<n_samples; i++) {
        for (unsigned j=0; j<steps; j++) {
            for (unsigned n=0; n<n_injections; n++) {
                if (j*step_size > (threshold + Gauss::rand_normal(0, noise, true))) {
                    sample[i][j]++;
                }
            }
        }
    }

    // Loop over samples and fit distribution
    double sum_thr = 0;
    double sum_noise = 0;
    double sum_inj = 0;
    for (unsigned i=0; i<n_samples; i++) {
        // Prepare fitting
        lm_status_struct status;
        lm_control_struct control;
        control = lm_control_float;
        control.verbosity = 0;
        double par[3] = {100, 5, 50};
        // Do fit
        lmcurve(3, par, steps, &x[0], &sample[i][0], scurve, &control, &status);
        //std::cout << par[0] << " " << par[1] << " " << par[2] << std::endl;
        sum_thr += par[0];
        sum_noise += par[1];
        sum_inj += par[2];
        if (i%10==0 && i>0) {
            std::cout << "Mean afer [" << i+1 << "] samples: \t" << sum_thr/(double)(i+1) << "\t" << sum_noise/(double)(i+1) << "\t" << sum_inj/(double)(i+1) << std::endl;
        }
    }
    std::cout << "Final mean afer [" << n_samples << "] samples: \t" << sum_thr/(double)n_samples << "\t" << sum_noise/(double)n_samples << "\t" << sum_inj/(double)n_samples << std::endl;
    return 0;
}
