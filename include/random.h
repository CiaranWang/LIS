#ifndef _RANDOM_H_
#define _RANDOM_H_

#include <random>
#include <stdlib.h>
#include <math.h>
#define pi ((double)3.14159265358979323846)

double uniformrand(std::mt19937& rng, double min, double max);

double standard_gaussrand(std::mt19937& rng);

double gaussrand(std::mt19937& rng, double mu, double sigma);

// 2D Gaussian: used for density computation
inline double gaussian2D(double r2, double sigma) {
    const double norm = 1.0 / (2.0 * pi * sigma * sigma);
    return std::exp(-r2 / (2.0 * sigma * sigma)) * norm;
}

// 1D Gaussian: used for radial weighting
inline double gaussian1D(double r, double sigma) {
    const double norm = 1.0 / std::sqrt(2.0 * pi * sigma);
    return std::exp(-r * r / (2.0 * sigma * sigma)) * norm;
}

double poissonrand(std::mt19937& rng, double lambda);

int binaryrand(std::mt19937& rng, double p);

double logistic(double _X_, double _A_, double _B_);

#endif