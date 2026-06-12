#ifndef _LIS_H_
#define _LIS_H_
#include <iostream>
#include <filesystem>  // C++17
#include <fstream>
#include <string>
#include <random>
#include <vector>
#include <array>

using namespace std;

#define pi ((double)3.14159265358979323846)  //just pi, dont change pi

extern double unit_angle;
extern double motivation_change[3][3];
extern double lx;           // length of the pen (in cm)
extern double ly;           // width of the pen (in cm)
extern int n_theta;         // number of directions animals are allowed to move
extern int n_feeder;        // number of feeders
extern double bodysize;     // diameter of the animal
extern double stepsize;     // movement distance per simulation step (cm)
extern double sensingrange; // distance animals can see/sense
extern double sigma_blur;   // sigma of the Gaussian blur
extern std::vector<std::array<double, 2>> feeder_coordinates;

class animal;

void define_feeders(std::vector<std::array<double, 3>>& feeder);
void define_feeders_detti(std::vector<std::array<double, 3>>& feeder);
void run_pen(const std::vector<animal>& pen_animals,
	int _PEN_,
	std::ofstream& asreml_out,
	bool write_header,
	int steps,
	double trait4_sigmaE,
	int bfdc,
	std::mt19937& rng);
#endif

