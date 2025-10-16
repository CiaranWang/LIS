#include <iostream>
#include <filesystem>  // C++17
#include <ctime>
#include <cmath>
#include <fstream>
#include <string>
#include <cstdlib>
#include <random>

#include "lis.h"
#include "random.h"
#include "animal.h"
#include "move.h"
#include "population.h"
#include "act.h"

namespace fs = std::filesystem;
using namespace std;

void run_pen(const string& infile, int _PEN_,
	std::ofstream& asreml_out,
	bool write_header,
	int steps,
	std::mt19937& rng)
{
	
	if (write_header)
	{
		//motion_out << "pen\tstep\tID\tx\ty\tangle\tME\tMW\tMR\tstatus\t" << endl;
		//interaction_out << "pen\tstep\tperformer\treceiver\tp_X\tp_Y\tr_X\tr_Y\tnumber\ttotal_number\ttotal_meets" << endl;
		asreml_out << "performer\treceiver\tpen\tnr_meet\tnr_interact\tnr_bites" << endl;
	}
	
	/*string video_name = "";
	video_name = outpath + "\\Turkeymotion_Video.mp4";
	VideoWriter video(video_name, VideoWriter::fourcc('P', 'I', 'M', '1'), 15.0, Size(1000, 1000), 1);*/

	define_feeders_detti(); // call define feeders

	animal t[n_animal];

	for (int i = 0; i < n_animal; i++)
	{
		t[i].initialize_(rng); // initialize the status of the turkey
		//report_pro_theta(t[i]);
	}

	read_pheno(infile, _PEN_); // created in R
	//std::cout << "Pen " << _PEN_ << ": feeder set up, motivation initialized, phenotypes read\n";
	
	int step = 0;
	//int total_meet = 0;

	while (step < steps) //12 hours
	{
		// behave eat rest walk
		for (int i = 0; i < n_animal; i++)
		{
			t[i].behave_(); // choose behaviour using current motivations

			if (t[i].behavior_code == 0) // feed
			{
				int feeder_code = choose_feeder(t[i]); // choose nearest unoccupied

				if (feeder_code == -1) // all feeders taken
				{
					t[i].alter_prob_uniform(); // no feeder then walk around, all angles have equal probability
					//report_pro_theta(t[i]);
					for (int j = 0; j < n_animal; j++)
					{
						if (j != i)
						{// distance btw turkeys
							double d_tt_sqr = d_sqr(t[i], t[j]);
							if (d_tt_sqr < 4 * bodysize * bodysize) // close enough
							{
								t[i].alter_prob_avoid_obstacles(t[j].x, t[j].y); // avoid the turkey thats close   
							}
						}
					}
					t[i].normalize_prob(); // only 7 directions with probability > 0, so normaize that to add up to 1
					int theta_code = select_direction(t[i], rng); // select from 8 directions given probabilities
					//cout << theta_code << endl;
					t[i].move_(stepsize, theta_code * unit_angle);
					t[i].update_motivation_(2); // given that we walked

				}
				else
				{// now we approach a feeder, distance turkey to feeder
					double d_ft_sqr = (t[i].x - feeder[feeder_code][0]) * (t[i].x - feeder[feeder_code][0]) +
						(t[i].y - feeder[feeder_code][1]) * (t[i].y - feeder[feeder_code][1]);

					if (d_ft_sqr < stepsize * stepsize) // because dist is squared above as well
					{
						feeder[feeder_code][2] = 1;
						t[i].update_motivation_(0);
						//if (t[i].motivation[0] <= 0)
						//{feeder[feeder_code][2] = 0;}

					}
					else // go to the feeder, when distance more than a step to the feeder
					{
						t[i].alter_prob_towards(feeder[feeder_code][0], feeder[feeder_code][1]);
						//t[i].alter_prob_uniform();
						//report_pro_theta(t[i]);

						for (int j = 0; j < n_animal; j++)
						{
							if (j != i)
							{
								double d_tt_sqr = d_sqr(t[i], t[j]);
								if (d_tt_sqr < 4 * bodysize * bodysize)
								{
									t[i].alter_prob_avoid_obstacles(t[j].x, t[j].y);
								}
							}
						}
						t[i].normalize_prob();
						int theta_code = select_direction(t[i], rng);
						//cout << theta_code << endl;
						t[i].move_(stepsize, theta_code * unit_angle);
						t[i].update_motivation_(2); // update walking motivation
					}
				}
			}
			else if (t[i].behavior_code == 1) //rest
			{
				t[i].update_motivation_(1);
			}
			else if (t[i].behavior_code == 2) //random walk
			{
				alter_prob_density_trait_s(t, i);
				//report_pro_theta(t[i]);
				for (int j = 0; j < n_animal; j++)
				{
					if (j != i)
					{
						double d_tt_sqr = d_sqr(t[i], t[j]);
						if (d_tt_sqr < 4 * bodysize * bodysize)
						{
							t[i].alter_prob_avoid_obstacles(t[j].x, t[j].y);
						}
					}
				}
				t[i].normalize_prob();
				int theta_code = select_direction(t[i], rng);
				//cout << theta_code << endl;
				t[i].move_(stepsize, theta_code * unit_angle);
				t[i].update_motivation_(2);
			}

			/*motion_out << _PEN_ << "\t"
				<< step << "\t"
				<< t[i].id << "\t"
				<< t[i].x << "\t"
				<< t[i].y << "\t"
				<< t[i].angle << "\t"
				<< t[i].motivation[0] << "\t" //new motivation of animal 
				<< t[i].motivation[1] << "\t"
				<< t[i].motivation[2] << "\t"
				<< t[i].behavior_code << endl; // curren behaviour
				*/
		}

		for (int i = 0; i < n_animal; i++)
		{
			t[i].update_position_();
		}
		
		//behave interaction
		for (int i = 0; i < n_animal; i++)
		{
			for (int j = 0; j < i; j++)
			{
				double pij = prob_peck(t[i], t[j]); // turkey 1 (i) pecks turkey 2 (j)
				double pji = prob_peck(t[j], t[i]);
				if (pij > 0) // if 2 turkeys are close p>0
				{
					t[i].n_meet[j]++; // ++ mean plus one
					t[j].n_meet[i]++; // ++ mean plus one

					int interact_or_not_ij = binaryrand(rng, pij);
					int interact_or_not_ji = binaryrand(rng, pji);

					if (interact_or_not_ij == 1)
					{
						t[i].n_interact[j]++;
						double nr_act_i2j = poissonrand(rng, t[i].trait_n) + 1;
						t[i].n_peck[j] = t[i].n_peck[j] + int(nr_act_i2j);
					}

					if (interact_or_not_ji == 1)
					{
						t[j].n_interact[i]++;
						double nr_act_j2i = poissonrand(rng, t[j].trait_n) + 1;
						t[j].n_peck[i] = t[j].n_peck[i] + int(nr_act_j2i);
					}

					/*
					interaction_out
						<< _PEN_ << "\t"
						<< step << "\t"
						<< t[i].id << "\t"
						<< t[j].id << "\t"
						<< t[i].x << "\t"
						<< t[i].y << "\t"
						<< t[j].x << "\t"
						<< t[j].y << "\t"
						<< nr_act_i2j << "\t"
						<< t[i].n_peck[j] << "\t"
						<< t[i].n_meet[j] << "\t"
						<< endl;

					interaction_out
						<< _PEN_ << "\t"
						<< step << "\t"
						<< t[j].id << "\t"
						<< t[i].id << "\t"
						<< t[j].x << "\t"
						<< t[j].y << "\t"
						<< t[i].x << "\t"
						<< t[i].y << "\t"
						<< nr_act_j2i << "\t"
						<< t[j].n_peck[i] << "\t"
						<< t[j].n_meet[i] << "\t"
						<< endl;*/
				}
			}	
		}

		step++;

		//total_meet = 0; // alternatively one could stop at X pecks
		//compute total numbers of meets
		/*for (int i = 0; i < n_animal; i++)
		{
			for (int j = 0; j < i; j++)
			{
				total_meet = total_meet + t[i].n_meet[j];
			}
		}*/
	}

	for (int i = 0; i < n_animal; i++)
	{
		for (int j = 0; j < n_animal; j++)
		{
			if (j == i) continue; //skip self

			asreml_out
				<< t[i].id << "\t"
				<< t[j].id << "\t"
				<< _PEN_ << "\t"
				<< t[i].n_meet[j] << "\t"
				<< t[i].n_interact[j] << "\t"
				<< t[i].n_peck[j] << endl;
		}
	}

	/*for (int i = 0; i < n_turkey; i++)
	{
		for (int j = 0; j < n_turkey; j++)
		{
			if (i != j)
			{
				peck_file << step << "\t"
					<< i << "\t"
					<< j << "\t"
					<< t[i].xPos << "\t"
					<< t[i].yPos << "\t"
					<< t[j].xPos << "\t"
					<< t[j].yPos << "\t"
					<< 0 << "\t"
					<< t[i].n_meet[j] << "\t"
					<< endl;
			}
		}
	}*/

	//cout<< "the end of pen: " << _PEN_ << endl;
}