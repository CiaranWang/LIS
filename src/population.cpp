#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <map>
#include <stdexcept>
#include <utility>

#include "random.h"
#include "animal.h"
#include "move.h"
#include "act.h"
#include "lis.h"
#include "population.h"

using namespace std;

std::vector<PenPopulation> read_population(const std::string& infile)
{
	std::ifstream input(infile);
	if (!input) {
		throw std::runtime_error("Could not open input file: " + infile);
	}

	std::string line;
	std::getline(input, line); // skip header

	std::map<int, std::vector<animal>> by_pen;
	int line_number = 1;

	while (std::getline(input, line)) {
		line_number++;
		if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
			continue;
		}

		std::istringstream iss(line);
		std::string id;
		int pen;
		double trait1, trait2, trait3, trait4;

		if (!(iss >> id >> pen >> trait1 >> trait2 >> trait3 >> trait4)) {
			throw std::runtime_error("Invalid input row at line " + std::to_string(line_number));
		}

		animal current;
		current.set_id(id);
		current.set_pen(pen);
		current.set_trait_p(trait1);
		current.set_trait_r(trait2);
		current.set_trait_s(trait3);
		current.set_trait_n(trait4);

		by_pen[pen].push_back(current);
	}

	std::vector<PenPopulation> pens;
	pens.reserve(by_pen.size());
	for (auto& item : by_pen) {
		PenPopulation pen_population;
		pen_population.pen = item.first;
		pen_population.animals = std::move(item.second);
		pens.push_back(std::move(pen_population));
	}

	if (pens.empty()) {
		throw std::runtime_error("Input file contains no animal records: " + infile);
	}

	return pens;
}
