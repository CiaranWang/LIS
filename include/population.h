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

extern animal t[n_animal];

void read_pheno(const std::string& _INFILE_, int _PEN_);

#endif