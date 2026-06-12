#pragma once

static const char* LIS_HELP_TEXT = R"(Usage: ./LIS [OPTIONS]

Simulation of animal interactions in a pen.

Options:
  -h, -H, --help          Show this help message and exit
  -u, -U, --update        Automatically update from github and rebuild 
  -v, -V, --version       Print program version and exit
  
  -i, --input [/PATH/TO/INPUT_FILE.txt]
                              Input phenotype file
  -p, --param, --parameter [/PATH/TO/parameter.ini]
                              Parameter file (default: parameter.ini; required at runtime)
  --seed [N]                      Integer random seed (default: time-based)
  -o, --output [/PATH/TO/OUTPUT_FILE.txt]
                              Output result file
  --step, --steps [N]        Number of simulation steps
  --BiteForceSigmaE, --bite-force-sigma-e [SIGMA]
                              Standard deviation of bite force (positive real number)
  --BiteForceDist, --bite-force-dist [TYPE]
                              Distribution to transform bite force trait value to observation 
   [TYPE] = Gaussian:  observed bite force is simply drawn from a normal distribution, with 
                       mean = biter's trait value, sd = BiteForceSigmaE value;
   [TYPE] = normal:    same with Gaussian;
   [TYPE] = Poisson+1: observed bite force is drawn from a poisson distribution, with 
                       mean = biter's trait value, then plus one;
   [TYPE] = lognormal: observed bite force is first drawn from a normal distribution, with 
                       mean = biter trait value, sd = BiteForceSigmaE value. 
                       Then natural exponential is taken.
   [TYPE] = uniform:   observed bite force is simply drawn from uniform distribution, with 
                       min = biter trait value - BiteForceSigmaE,
                       max = biter trait value + BiteForceSigmaE;
   To make life easier, this [TYPE] argument is case insensitive, so POiSSoN+1, gaussIAn are ok.


Example:
  ./LIS --input pheno.txt --param parameter.ini --seed 123456 --output result.txt --steps 100 --bite-force-sigma-e 0.37 --bite-force-dist loGNorMaL

Report bugs to: zhuoshi.wang@wur.nl
)";
