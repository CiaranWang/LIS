#include <iostream>
#include <filesystem>  // C++17
#include <ctime>
#include <cmath>
#include <fstream>
#include <string>
#include <cstdlib>
#include <optional>     // <<< needed for optional seed
#include <sstream>
#include <omp.h>   // <<< OpenMP
#include <chrono>
#include <random>
#include <algorithm>
#include <cctype>

#include "random.h"
#include "animal.h"
#include "move.h"
#include "population.h"
#include "act.h"
#include "lis.h"

namespace fs = std::filesystem;
using namespace std;

static const std::string PROGRAM_VERSION = "0.3.3";
 
double unit_angle = 2.0 * pi / n_theta; //discrete moving angles
							    //eat    rest    walk

double motivation_change[3][3]{ -5.0/9,	+1.0/15,	+2.0/45,	    //eat
								   0.0,	-5.0/9 , 	+5.0/24,	    //rest
								   0.0,	+5.0/9 ,	-5.0/24};	    //walk

static void print_version() {
    std::cout << "LIS Version: " << PROGRAM_VERSION << std::endl;
}

static void print_help() {
    std::cout <<
        R"(Usage: ./LIS [OPTIONS]

Simulation of animal interactions in a pen.

Options:
  -h, -H, --help          Show this help message and exit
  -u, -U, --update        Automatically update from github and rebuild 
  -v, -V, --version       Print program version and exit
  
  -i [/PATH/TO/INPUT_FILE.txt]    Input phenotype file
  --seed [N]                      Integer random seed (default: time-based)
  -o [/PATH/TO/OUTPUT_FILE.txt]   Output result file
  --step [N]                Number of simulation steps
  --BiteForceSigmaE [SIGMA] Standard deviation of bite force (positive real number)
  --BiteForceDist [TYPE] Distribution to tranform bite force trait value to obervation 
   [TYPE] = Gaussian:  observed bite force is simply drawn from a normal distribution, with 
                       mean = biter's trait value, sd = BiteForceSigmaE value;
   [TYPE] = Poisson+1: observed bite force is drawn from a poisson distribution, with 
                       mean = biter's trait value, then plus one;
   [TYPE] = lognormal: observed bite force is first drawn from a normal distribution, with 
                       mean = biter trait value, sd = BiteForceSigmaE value. 
                       Then natural exponential is taken.
   [TYPE] = uniform:   observed bite force is simply drawn from uniform distribution, with 
                       min = biter trait value - BiteForceSigmaE,
                       max = biter trait value + BiteForceSigmaE;
   To make life easier,this [TYPE] argument is case insensitive, so POiSSoN+1, gaussIAn are ok.


Example:
  ./LIS -i pheno.txt --seed 123456 -o result.txt --step 100 --BiteForceSigmaE 0.37 --BiteForceDist loGNorMaL

Report bugs to: zhuoshi.wang@wur.nl
)";
}

fs::path get_lis_root(char* argv0) {
    fs::path exePath = fs::absolute(argv0);      // /ROOTPATH/build/LIS
    fs::path exeDir = exePath.parent_path();    // /ROOTPATH/build
    fs::path rootDir = exeDir.parent_path();    // /ROOTPATH
    return rootDir;
}

static void run_update(char* argv0) {
    fs::path lis_root = get_lis_root(argv0);

    std::cout << "[UPDATE] Attempting to update LIS in: " << lis_root << std::endl;
    std::cout << "Make sure you have 'git', 'cmake', and 'make' installed." << std::endl;

    // Check if .git folder exists
    if (!fs::exists(lis_root / ".git") || !fs::is_directory(lis_root / ".git")) {
        std::cout << "Warning: LIS root folder is not a git repository.\n";
        std::cout << "Clone the repository and try again:\n";
        std::cout << "  git clone https://github.com/CiaranWang/LIS.git\n";
        return;
    }

    // Pull latest changes
    std::string git_cmd = "cd \"" + lis_root.string() + "\" && git pull origin master";
    if (system(git_cmd.c_str()) != 0) {
        std::cout << "Git pull failed.\n";
        return;
    }

    // Build project
    std::string build_cmd =
        "cd \"" + lis_root.string() + "\" && mkdir -p build && cd build && cmake .. && make -j 8";
    if (system(build_cmd.c_str()) != 0) {
        std::cout << "Build failed.\n";
        return;
    }

    std::cout << "Update and rebuild completed successfully!\n";
    std::cout << "You can now run: ./build/LIS [options]\n";
}

static int get_bfdc(std::string bfdt)
{
    // Convert to lowercase
    std::transform(bfdt.begin(), bfdt.end(), bfdt.begin(),
        [](unsigned char c) { return std::tolower(c); });

    if (bfdt == "gaussian") {
        return 0;
    }
    else if (bfdt == "poisson+1") {
        return 1;
    }
    else if (bfdt == "lognormal") {
        return 2;
    }
    else if (bfdt == "uniform") {
        return 3;
    }

    throw std::invalid_argument("Unknown bfdt: " + bfdt);
}

int main(int argc, char* argv[])
{
    // ================================================================
    // Early check for --version / --update / --help
    // ================================================================
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "--version" || arg1 == "-v" || arg1 == "-V") {
            print_version();
            return 0;
        }
        if (arg1 == "--update" || arg1 == "-u" || arg1 == "-U") {
            run_update(argv[0]);
            return 0;
        }
        if (arg1 == "--help" || arg1 == "-h" || arg1 == "-H") {
            print_help();
            return 0;
        }
    }
    // ================================================================

    fs::path input_file;
    fs::path output_file;
    optional<int> seed; // <<< optional seed
    int steps = 120; //default steps 2min
    double trait4_sigmaE = 1.0;
    int bfdc = 2;
    // ----- Parse command line arguments -----
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-i" && i + 1 < argc) {
            input_file = argv[++i];
        }
        else if (arg == "--seed" && i + 1 < argc) {
            seed = std::stoi(argv[++i]);  // <<< store in optional
        }
        else if (arg == "-o" && i + 1 < argc) {
            output_file = argv[++i];
        }
        else if (arg == "--step" && i + 1 < argc) {
            steps = std::stoi(argv[++i]);
        }
        else if (arg == "--BiteForceSigmaE" && i + 1 < argc) {
            trait4_sigmaE = std::stod(argv[++i]);
        }
        else if (arg == "--BiteForceDist" && i + 1 < argc) {
            bfdc = get_bfdc(argv[++i]);
        }
        else {
            cerr << "Unknown or incomplete argument: " << arg << "\n";
            return 1;
        }
    }

    if (input_file.empty()) {
        cerr << "Error: -i input_file is required\n";
        return 1;
    }

    // ----- Set default output path if not provided -----
    if (output_file.empty()) {
        output_file = input_file.parent_path() / ("output_" + input_file.filename().string());
    }

    cout << "Input file:  " << input_file << "\n";
    cout << "Output file: " << output_file << "\n";
    cout << "Steps:       " << steps << "\n";

    if (seed.has_value()) {
        cout << "Seed:        " << seed.value() << " (using deterministic per-thread RNG)\n";
    }
    else {
        cout << "Seed:        (not provided, using time-based RNG per thread)\n";
    }

    int nPens = 400;
    
    // ---- Run pens in parallel with OpenMP ----
    #pragma omp parallel for schedule(dynamic)
    for (int pen = 1; pen <= nPens; ++pen) 
    {
        using namespace std::chrono;
        auto start = high_resolution_clock::now();

        // --- Thread-safe per-pen RNG ---
        unsigned int thread_seed = seed.value_or(static_cast<int>(time(nullptr))) + pen;
        std::mt19937 rng(thread_seed);  // Mersenne Twister RNG for this pen

        // make per-pen output file
        fs::path perfile = output_file;
        std::ostringstream oss;
        oss << output_file.stem().string() << "_pen" << pen << output_file.extension().string();
        perfile.replace_filename(oss.str());

        ofstream per_out(perfile, ios::out);
        if (!per_out) {
            #pragma omp critical
            { cerr << "Failed to open per-pen file: " << perfile << "\n"; }
            continue;
        }

        bool write_header = (pen == 1);
        run_pen(input_file.string(), pen, per_out, write_header, steps, trait4_sigmaE, bfdc, rng);
        per_out.close();

        auto end = high_resolution_clock::now();
        auto ms = duration_cast<milliseconds>(end - start).count();

        #pragma omp critical
        {
            cout << "Pen " << pen << " finished in " << ms / 1000.0 << " seconds\n";
        }
    }

    // ---- Merge results into final file ----
    ofstream final_out(output_file, ios::out);
    if (!final_out) {
        cerr << "Failed to open final output file: " << output_file << "\n";
        return 1;
    }

    for (int pen = 1; pen <= nPens; ++pen) 
    {
        fs::path perfile = output_file;
        std::ostringstream oss;
        oss << output_file.stem().string() << "_pen" << pen << output_file.extension().string();
        perfile.replace_filename(oss.str());

        ifstream per_in(perfile);
        if (!per_in) continue;

        string line;
        while (getline(per_in, line)) {
            final_out << line << "\n";
        }

        per_in.close();
        fs::remove(perfile);
    }

    final_out.close();

    cout << "Processing complete!\n";

	return 0;
}
