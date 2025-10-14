#include <iostream>
#include <cmath>
#include <cstdlib>

#include "random.h"
#include "animal.h"
#include "move.h"
#include "act.h"
#include "lis.h"

using namespace std;


double prob_peck_distance_factor(double _D_) 
{
	//useless function
	double left_part = 0.5;

	if (_D_ <= sensingrange * left_part)
	{
		return 1;
	}
	else if (_D_ > sensingrange * left_part && _D_ < sensingrange)
	{
		return 1;
		//return (sensingrange - _D_) / (sensingrange  - sensingrange * left_part);
	}

	return 0;
}

double prob_peck_pheno_factor(double _PHENO11_, double _PHENO22_)
{
	//probability of a pecking event, when two individuals are close enough
	double a = -1;
	double b = 0;
	return logistic(_PHENO11_ + _PHENO22_, a, b);
}

double prob_peck(animal _T1_, animal _T2_)
{
	//probability of a pecking event, telling if two individuals are close enough
	double d = sqrt((_T1_.x - _T2_.x) * (_T1_.x - _T2_.x)
				  + (_T1_.y - _T2_.y) * (_T1_.y - _T2_.y));

	if (d < sensingrange)
	{
		return prob_peck_distance_factor(d) * prob_peck_pheno_factor(_T1_.trait_p, _T2_.trait_r);
	}
	else
	{
		return 0;
	}
}
