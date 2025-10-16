#include <iostream>
#include <cmath>
#include <cstdlib>
#include <vector>

#include "move.h"
#include "lis.h"
#include "animal.h"
#include "population.h"
#include "random.h"

using namespace std;
double feeder[n_feeder][3]{0};

const double norm_factor = 1.0 / 0.477 / std::sqrt(2.0 * pi * sigma_blur);
const double inv_two_sigma2 = 1.0 / (2.0 * sigma_blur * sigma_blur);

void define_feeders() {
	
	if (n_feeder >= 2)
	{
		for (int i = 0; i < n_feeder / 2; i++)
		{
			feeder[i][0] = 0;
			feeder[i][1] = i * ly / (n_feeder / 2.0) + ly / n_feeder;

			feeder[i + n_feeder / 2][0] = lx;
			feeder[i + n_feeder / 2][1] = i * ly / (n_feeder / 2.0) + ly / n_feeder;
		}

		for (int i = 0; i < n_feeder; i++)
		{
			feeder[i][2] = 0;
		}
	}
}

void define_feeders_detti() {
	for (int i = 0; i < n_feeder; i++)
	{
		feeder[i][0] = 0;
		feeder[i][1] = 0;
	}

	for (int i = 0; i < n_feeder; i++)
	{
		feeder[i][2] = 0;
	}
}

int choose_feeder(animal _T_) {
	double x = _T_.x;
	double y = _T_.y;
	int which_feeder = -1;
	double min_d_sqr = lx * lx + ly * ly;
	double d_sqr = 0;
	for (int i = 0; i < n_feeder; i++)
	{
		if (feeder[i][2] == 0)
		{
			d_sqr = (x - feeder[i][0]) * (x - feeder[i][0]) + (y - feeder[i][1]) * (y - feeder[i][1]);
			if (d_sqr < min_d_sqr)
			{
				min_d_sqr = d_sqr;
				which_feeder = i;
			}
		}
	}
	return which_feeder;
}

int select_direction(animal& _T_, std::mt19937& rng) {

	double accumulate_lower = -_T_.pro_theta[0];
	double accumulate_upper = 0;

	// Use thread-safe RNG
	double random = uniformrand(rng, 0.0, 1.0);

	for (int i = 0; i < n_theta; i++)
	{
		accumulate_lower = accumulate_upper;
		accumulate_upper = accumulate_upper + _T_.pro_theta[i];

		if (random >= accumulate_lower && random < accumulate_upper)
		{
			return i;
		}
	}

	return static_cast<int>(round(_T_.angle / unit_angle));
}

void report_pro_theta(animal _T_) {
	for (int j = 0; j < n_theta; j++)
	{
		cout << _T_.pro_theta[j] << endl;
	}
}

int select_rnd_direction(animal _T_) {
	int random = (rand() % (n_theta));
	return random;
}

double calc_heatmap_XY(animal t[n_animal], int _I_, double _X_, double _Y_)
{
	double density = 0;
	double r_square = 0;
	double PDF_j = 0;

	for (int j = 0; j < n_animal; j++)
	{
		if (j == _I_)
		{ 
			continue; 
		}

		r_square = (_X_ - t[_I_].x) * (_X_ - t[_I_].x) + (_Y_ - t[_I_].y) * (_Y_ - t[_I_].y);
		PDF_j = 1 / (2.0 * pi * sigma_blur * sigma_blur) * exp(- r_square / (2.0 * sigma_blur * sigma_blur));

		density = density + PDF_j;
	}

	return density;
}

double calc_heatmap_RHOTHETA(animal t[n_animal], int _I_, double _RHO_, double _THETA_)
{
	double density = 0.0;

	double _X_ = t[_I_].x + _RHO_ * cos(_THETA_);
	double _Y_ = t[_I_].y + _RHO_ * sin(_THETA_);

	for (int j = 0; j < n_animal; j++)
	{
		if (j == _I_) continue;

		double dx = _X_ - t[j].x;
		double dy = _Y_ - t[j].y;
		density += gaussian2D(dx * dx + dy * dy, sigma_blur);
	}

	return density;
}

double calc_weighed_density_orientation(animal t[n_animal], int _I_, double _THETA_)
{
	double integral = 0.0;
	const int iteration = 500;
	const double delta_r = 2.0 * sigma_blur / iteration;

	for (int k = 0; k < iteration; k++)
	{
		double r = k * delta_r;
		double delta_p = calc_heatmap_RHOTHETA(t, _I_, r, _THETA_) * delta_r;

		double weighed_factor = norm_factor * std::exp(-r * r * inv_two_sigma2);

		integral += weighed_factor * delta_p;
	}
	return integral;
}