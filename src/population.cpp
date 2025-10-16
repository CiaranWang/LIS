#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include "random.h"
#include "animal.h"
#include "move.h"
#include "act.h"
#include "lis.h"
#include "population.h"

using namespace std;

void read_pheno(animal t[n_animal], const std::string& _INFILE_, int _PEN_) // pen starts from 1 not 0
{
	std::ifstream infile(_INFILE_);
	if (!infile) {
		std::cerr << "Error: could not open file.\n";
       // return 0; // return 0 if file failed
	}

	std::string line;
	std::getline(infile, line); // skip header

    // data line numbers (counting from 2, since line 1 was header)
	int start_line = 14 * (_PEN_ - 1) + 2;
	int end_line = start_line + 13;

	int current_line = 2; ; // we are now at line 2
	int idx = 0;

    while (std::getline(infile, line) && current_line <= end_line) {
        if (current_line >= start_line && idx < 14) {
            std::istringstream iss(line);
            std::string ID;
            int Pen;
            double trait1, trait2, trait3, trait4;

            if (iss >> ID >> Pen >> trait1 >> trait2 >> trait3 >> trait4) {
                t[idx].set_id(ID);
                t[idx].set_pen(Pen);
                t[idx].set_trait_p(trait1);
                t[idx].set_trait_r(trait2);
                t[idx].set_trait_s(trait3);
                t[idx].set_trait_n(trait4);
                idx++;
            }
        }
        current_line++;
    }

    /*for (int i = 0; i < idx; i++) {
        t[i].print_traits();
    }
    cout << idx << endl;*/
	//return idx; // number of animals actually read
}
