#ifndef _MOVE_H_
#define _MOVE_H_

#include <random>
#include "lis.h"
#include "animal.h"

int choose_feeder(double(&feeder)[n_feeder][3], animal _T_);

int select_direction(animal& _T_, std::mt19937& rng);

int select_rnd_direction(animal& _T_, std::mt19937& rng);

void report_pro_theta(animal _T_);

double calc_heatmap_XY(animal t[n_animal], int _I_, double _X_, double _Y_);

double calc_heatmap_RHOTHETA(animal t[n_animal], int _I_, double _RHO_, double _THETA_);

double calc_weighed_density_orientation(animal t[n_animal], int _I_, double _THETA_);

#endif