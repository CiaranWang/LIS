#include "animal.h"
#include <iostream>
#include <cmath>

#include "lis.h"
#include "random.h"
#include "population.h"
#include "move.h"

using namespace std;
// this file define features of turkeys

void animal::set_pos_(double _X_, double _Y_) {
	//position of turkeys
	if (_X_ < 0 || _X_ > lx || _Y_ <0 || _Y_ > ly)
	{
		cout << "ERROR! coordinate beyond boundary!" << endl;
	}

	x = _X_;
	y = _Y_;
}

void animal::set_angle_(double _ALPHA_) {
	//moving angle of turkeys
	if (_ALPHA_ < 0 || _ALPHA_ > 2 * pi)
	{
		cout << "ERROR! angle beyond boundary!" << endl;
	}

	angle = _ALPHA_;
}

void animal::initialize_(std::mt19937& rng) {
	x = uniformrand(rng, 0, lx);
	y = uniformrand(rng, 0, ly);
	new_x = x;
	new_y = y;


	angle = uniformrand(rng, 0.0, 2.0 * pi);

	for (int i = 0; i < n_theta; i++)
	{
		pro_theta[i] = double(1) / n_theta;
	}

	for (int i = 0; i < n_animal; i++)
	{
		n_peck[i] = 0;
		n_interact[i] = 0;
		n_meet[i] = 0;
	}

	//generate random starting status
	motivation[0] = uniformrand(rng, 0, 100); // code 0
	motivation[1] = uniformrand(rng, 0, 100); // code 1
	motivation[2] = uniformrand(rng, 0, 100); // code 2

	threshold[0] = 100; // code 0
	threshold[1] = 100; // code 1
	threshold[2] = 100; // code 2

	behavior_code = 1;

	trait_p = 0;
	trait_r = 0;
	trait_s = 0;
	trait_n = 0;
	
}

void animal::behave_() {
	// let the turkeys make decisions to behave in the way that we defined
	if (motivation[behavior_code] > 0)
	{
	}
	else if (motivation[0] > threshold[0])
	{
		behavior_code = 0;
	}
	else if (motivation[1] > threshold[1])
	{
		behavior_code = 1;
	}
	else if (motivation[2] > threshold[2])
	{
		behavior_code = 2;
	}
	else
	{
		behavior_code = 1;
	}
}

void animal::move_(double _D_, double _THETA_) {
	new_x = x + _D_ * cos(_THETA_);
	new_y = y + _D_ * sin(_THETA_);
	angle = _THETA_;

	if (new_x < 0)
	{
		new_x = -new_x;
	}
	if (new_x > lx)
	{
		new_x = 2 * lx - new_x;
	}
	if (new_y < 0)
	{
		new_y = -new_y;
	}
	if (new_y > ly)
	{
		new_y = 2 * ly - new_y;
	}
}

void animal::update_position_() {
	x = new_x;
	y = new_y;
}

void animal::update_motivation_(int _CODE_) {
	motivation[0] = motivation[0] + motivation_change[0][_CODE_];
	motivation[1] = motivation[1] + motivation_change[1][_CODE_];
	motivation[2] = motivation[2] + motivation_change[2][_CODE_];
}

void animal::alter_prob_uniform() {
	//probability of movement towards a certain direcntion assuming completely random
	for (int i = 0; i < n_theta; i++)
	{
		pro_theta[i] = 1.0 / n_theta;
	}
}

void alter_prob_density(int _I_) 
{
	for (int i = 0; i < n_theta; i++)
	{
		t[_I_].pro_theta[i] = calc_weighed_density_orientation(_I_, i * unit_angle);
	}

	t[_I_].normalize_prob();
}

void alter_prob_density_trait_s(int _I_)
{
	alter_prob_density(_I_);
	double factor = logistic(t[_I_].trait_s, -1, 0) * 2 - 1;
	for (int i = 0; i < n_theta; i++)
	{
		t[_I_].pro_theta[i] = 1.0 / n_theta + (t[_I_].pro_theta[i] - 1.0 / n_theta) * factor;
	}

	t[_I_].normalize_prob();
}

void animal::alter_prob_towards(double _TARGET_X_, double _TARGET_Y_) {
	//create un-uniform probabilities of different direction
	double slope = (y - _TARGET_Y_) / (x - _TARGET_X_);
	double alpha = 0;

	if (_TARGET_X_ > x && slope > 0)
	{
		alpha = atan(slope);
	}
	else if (_TARGET_X_ > x && slope < 0)
	{
		alpha = atan(slope) + 2 * pi;
	}
	else if (_TARGET_X_ < x)
	{
		alpha = atan(slope) + pi;
	}
	int direction_code = static_cast<int>(std::round(alpha / unit_angle));
	for (int i = 0; i < n_theta / 4; i++)
	{
		//cout << cos(i * unit_angle) << endl;
		pro_theta[(direction_code + i) % n_theta] = 0.5 * cos(i * unit_angle);
		pro_theta[(direction_code - i + n_theta) % n_theta] = 0.5 * cos(i * unit_angle);

		pro_theta[(direction_code + 2 * i)% n_theta] = 0;
		pro_theta[(direction_code - 2 * i + n_theta) % n_theta] = 0;
	}
}

void animal::alter_prob_avoid_obstacles(double _TARGET_X_, double _TARGET_Y_) {
	//let the turkeys avoid obstacles. in this version I sent you, I keep the simple way. Late we shall discuss what obstacles are there.
	double slope = (y - _TARGET_Y_) / (x - _TARGET_X_);
	double alpha = 0;
	
	if (_TARGET_X_ > x && slope > 0)
	{
		alpha = atan(slope);
	}
	else if (_TARGET_X_ > x && slope < 0 )
	{
		alpha = atan(slope) + 2 * pi;
	}
	else if (_TARGET_X_ < x)
	{
		alpha = atan(slope) + pi;
	}

	int direction_code = static_cast<int>(std::round(alpha / unit_angle));

	for (int i = 0; i < n_theta; i++)
	{
		pro_theta[(direction_code + i) % n_theta] = 0;
		pro_theta[(direction_code - i + n_theta) % n_theta] = 0;
	} //?? 16?
}

void animal::normalize_prob() {
	//making the probabilities sum up to one
	double sum = 0;
	for (int i = 0; i < n_theta; i++)
	{
		sum = sum + pro_theta[i];
	}
	if (sum > 0)
	{
		for (int i = 0; i < n_theta; i++)
		{
			pro_theta[i] = pro_theta[i] / sum;
		}
	}
	else
	{
		for (int i = 0; i < n_theta; i++)
		{
			pro_theta[i] = 1.0 / n_theta;
		}
	}

}

void animal::set_id(string _ID_)
{
	id = _ID_;
}
void animal::set_pen(int _PEN_)
{
	pen = _PEN_;
}
void animal::set_trait_p(double _PHENOTYPE_)
{
	trait_p = _PHENOTYPE_;
}
void animal::set_trait_r(double _PHENOTYPE_)
{
	trait_r = _PHENOTYPE_;
}
void animal::set_trait_s(double _PHENOTYPE_)
{
	trait_s = _PHENOTYPE_;
}
void animal::set_trait_n(double _PHENOTYPE_)
{
	trait_n = _PHENOTYPE_;
}
void animal::print_traits() const
{
	std::cout << "ID: " << id
		<< "Pen: " << pen
		<< " | Traits: " << trait_p << ", "
		<< trait_r << ", "
		<< trait_s << ", "
		<< trait_n << "\n";
}

double d_sqr(animal _T1_, animal _T2_)
{
	double d_squared = (_T1_.x - _T2_.x) * (_T1_.x - _T2_.x)
					 + (_T1_.y - _T2_.y) * (_T1_.y - _T2_.y);
	return d_squared;
}

double distance(animal _T1_, animal _T2_)
{
	double distance = sqrt(d_sqr(_T1_, _T2_));
	return distance;
}