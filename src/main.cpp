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

#include "random.h"
#include "animal.h"
#include "move.h"
#include "population.h"
#include "act.h"
#include "lis.h"

namespace fs = std::filesystem;
using namespace std;

static const std::string PROGRAM_VERSION = "0.2.1";
 
double unit_angle = 2.0 * pi / n_theta; //discrete moving angles
							    //eat    rest    walk

double motivation_change[3][3]{ -5.0/9,	+1.0/15,	+2.0/45,	    //eat
								   0.0,	-5.0/9 , 	+5.0/24,	    //rest
								   0.0,	+5.0/9 ,	-5.0/24};	    //walk

static void print_version() {
    std::cout << "LIS Version: " << PROGRAM_VERSION << std::endl;
}

static void run_update() {
    std::cout << "[UPDATE] Attempting to update from GitHub..." << std::endl;
    std::cout << "Make sure you have 'git', 'cmake', and 'make' installed." << std::endl;

    // Check if .git folder exists
    if (!fs::exists(".git") || !fs::is_directory(".git")) {
        std::cout << "Warning: This folder is not a git repository.\n";
        std::cout << "To enable auto-update, clone the repository from GitHub:\n";
        std::cout << "  git clone https://github.com/CiaranWang/LIS.git\n";
        std::cout << "Then run: ./LIS --update\n";
        return;
    }

    // Step 1: Pull latest changes from GitHub
    int git_result = system("git pull origin master");

    if (git_result != 0) {
        std::cout << "Update failed: git pull did not succeed.\n";
        return;
    }

    std::cout << "Git pull completed successfully.\n";

    // Step 2: Create build folder if it doesn't exist
    int mkdir_result = system("mkdir -p build");
    if (mkdir_result != 0) {
        std::cout << "Warning: Could not create build folder. You may need to create it manually.\n";
    }

    // Step 3: Run CMake
    int cmake_result = system("cd build && cmake ..");
    if (cmake_result != 0) {
        std::cout << "CMake configuration failed. Please run 'cmake ..' manually inside build folder.\n";
        return;
    }

    // Step 4: Build the project
    int make_result = system("cd build && make -j 8");
    if (make_result != 0) {
        std::cout << "Build failed. Please check for compilation errors.\n";
        return;
    }

    std::cout << "Update and rebuild completed successfully!\n";
    std::cout << "You can now run: ./build/LIS [options]\n";
}

int main(int argc, char* argv[])
{
    // ================================================================
    // Early check for --version / --update
    // ================================================================
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "--version" || arg1 == "-v") {
            print_version();
            return 0;
        }
        if (arg1 == "--update" || arg1 == "-u") {
            run_update();
            return 0;
        }
    }
    // ================================================================

    fs::path input_file;
    fs::path output_file;
    optional<int> seed; // <<< optional seed
    int steps = 120; //default steps 2min

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
        run_pen(input_file.string(), pen, per_out, write_header, steps, rng);
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
        bool skip_header = (pen != 1);
        while (getline(per_in, line)) {
            if (skip_header) { skip_header = false; continue; }
            final_out << line << "\n";
        }

        per_in.close();
        fs::remove(perfile);
    }

    final_out.close();

    cout << "Processing complete!\n";

	return 0;
}
