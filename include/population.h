#ifndef _POPULATION_H_
#define _POPULATION_H_

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>

#include "animal.h"
#include "lis.h"

using namespace std;

struct PenPopulation {
	int pen = 0;
	std::vector<animal> animals;
};

std::vector<PenPopulation> read_population(const std::string& infile);

#endif
