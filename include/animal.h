#ifndef _ANIMAL_H_
#define _ANIMAL_H_

#include "lis.h"

class animal {
public:
	string id;
	void set_id(string _ID_);

	int pen;
	void set_pen(int _PEN_);
	double x;
	double y;
	double angle;
	void set_pos_(double _X_, double _Y_);
	void set_angle_(double _ALPHA_);

	double new_x;
	double new_y;

	double motivation[3];
	double threshold[3];

	int behavior_code; // 0 = eat, 1 = rest, 2 = walk

	double pro_theta[n_theta];

	//phenotype of traits:
	double trait_p; //ppppperformer effect
	double trait_r; //rrrrreceipient effect
	double trait_s; //overall sssssocial tendency
	double trait_n; //nnnnnr of bites/pecks per interction

	void print_traits() const;

	int n_peck[n_animal];
	int n_interact[n_animal];
	int n_meet[n_animal];

	void initialize_(std::mt19937& rng);
	void behave_();
	void move_(double _D_, double _THETA_);
	void update_motivation_(int _CODE_);
	void update_position_();

	void alter_prob_uniform();
	void alter_prob_towards(double _TARGET_X_, double _TARGET_Y_);
	void alter_prob_avoid_obstacles(double _TARGET_X_, double _TARGET_Y_);
	void normalize_prob();

	void set_trait_p(double _PHENOTYPE_);
	void set_trait_r(double _PHENOTYPE_);
	void set_trait_s(double _PHENOTYPE_);
	void set_trait_n(double _PHENOTYPE_);
};

double d_sqr(animal _T1_, animal _T2_);
double distance(animal _T1_, animal _T2_);
void alter_prob_density(animal t[n_animal], int _I_);
void alter_prob_density_trait_s(animal t[n_animal], int _I_);

#endif