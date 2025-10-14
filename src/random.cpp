#include <stdlib.h>
#include <math.h>
#include <cmath>
#include <vector>

#include "random.h"
//#include <bits/stdc++.h>

const int MAX = 100;
const int n = 2;

double uniformrand(std::mt19937& rng, double min, double max)
{
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng);
}

double standard_gaussrand(std::mt19937& rng)
{
    // Use std normal distribution (Box-Muller internal by lib)
    static thread_local std::normal_distribution<double> norm(0.0, 1.0);
    return norm(rng);
}

double gaussrand(std::mt19937& rng, double mu, double sigma)
{
    std::normal_distribution<double> dist(mu, sigma);
    return dist(rng);
}

double poissonrand(std::mt19937& rng, double lambda)
{
    std::poisson_distribution<int> dist(static_cast<double>(lambda));
    return static_cast<double>(dist(rng));
}

int binaryrand(std::mt19937& rng, double p)
{
    std::bernoulli_distribution d(p);
    return d(rng) ? 1 : 0;
}

double logistic(double _X_, double _A_, double _B_)
{
    return 1.0 / (1.0 + std::exp(_A_ * _X_ + _B_));
}