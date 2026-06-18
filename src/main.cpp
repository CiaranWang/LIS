#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

#include <omp.h>

#include "input_setup.h"
#include "lis.h"

namespace fs = std::filesystem;
using namespace std;

int main(int argc, char* argv[])
{
    const StartupAction startup_action = handle_immediate_command(argc, argv);
    if (startup_action == StartupAction::ExitSuccess) {
        return 0;
    }
    if (startup_action == StartupAction::ExitFailure) {
        return 1;
    }

    SimulationConfig config;
    if (!prepare_simulation(argc, argv, config)) {
        return 1;
    }

    print_startup_summary(config);

    #pragma omp parallel for schedule(dynamic)
    for (int pen_index = 0; pen_index < static_cast<int>(config.pens.size()); ++pen_index)
    {
        const PenPopulation& pen_data = config.pens[pen_index];
        const int pen = pen_data.pen;
        using namespace std::chrono;
        auto start = high_resolution_clock::now();

        unsigned int thread_seed = config.seed.value_or(static_cast<int>(time(nullptr))) + pen;
        std::mt19937 rng(thread_seed);

        fs::path perfile = config.output_file;
        std::ostringstream oss;
        oss << config.output_file.stem().string() << "_pen" << pen
            << config.output_file.extension().string();
        perfile.replace_filename(oss.str());

        ofstream per_out(perfile, ios::out);
        if (!per_out) {
            #pragma omp critical
            { cerr << "Failed to open per-pen file: " << perfile << "\n"; }
            continue;
        }

        bool write_header = (pen_index == 0);
        run_pen(
            pen_data.animals,
            pen,
            per_out,
            write_header,
            config.steps,
            config.trait4_sigmaE,
            config.bfdc,
            rng);
        per_out.close();

        auto end = high_resolution_clock::now();
        auto ms = duration_cast<milliseconds>(end - start).count();

        #pragma omp critical
        {
            cout << "Pen " << pen << " finished in " << ms / 1000.0 << " seconds\n";
        }
    }

    ofstream final_out(config.output_file, ios::out);
    if (!final_out) {
        cerr << "Failed to open final output file: " << config.output_file << "\n";
        return 1;
    }

    for (const PenPopulation& pen_data : config.pens)
    {
        const int pen = pen_data.pen;
        fs::path perfile = config.output_file;
        std::ostringstream oss;
        oss << config.output_file.stem().string() << "_pen" << pen
            << config.output_file.extension().string();
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
