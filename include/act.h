#ifndef _ACT_H_
#define _ACT_H_

#include <iostream>
#include <cmath>
#include <cstdlib>

#include "random.h"
#include "animal.h"
#include "move.h"
#include "lis.h"

double prob_peck_distance_factor(double _D_);

double prob_peck_pheno_factor(double _PHENO11_, double _PHENO22_);

double prob_peck(animal _T1_, animal _T2_);

#endif