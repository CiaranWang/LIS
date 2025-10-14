#ifndef _LIS_H_
#define _LIS_H_
#include <iostream>
#include <filesystem>  // C++17
#include <string>
#include <random>

using namespace std;

#define pi ((double)3.14159265358979323846)  //just pi, dont change pi
#define lx ((double)450)        //length of the pen (in cm)
#define ly ((double)350)        //width of the pen (in cm)
#define n_theta ((int)8)         //nr directions that animals are allowed to move
#define n_animal ((int)14)      //nr of animals
#define n_feeder ((int)1)       //nr of feeders
#define feedersize ((int)40)     //feeder size (in cm)
#define bodysize ((double)40)    //the diamater of the animal
#define stepsize ((double)40)   //the speed of animal (cm per step of simulation)
#define sensingrange ((double)40)  //distance that animal can see/sense,events outside never impact
#define sigma_blur ((double)60)  //sigma of the Gaussian Blurrrr

extern double unit_angle;
extern double motivation_change[3][3];
void run_pen(const string& infile, int _PEN_,
	std::ofstream& asreml_out,
	bool write_header,
	int steps,
	std::mt19937& rng);
#endif

