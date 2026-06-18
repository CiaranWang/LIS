#ifndef _INPUT_SETUP_H_
#define _INPUT_SETUP_H_

#include <filesystem>
#include <optional>
#include <vector>

#include "population.h"

struct SimulationConfig {
    std::filesystem::path input_file;
    std::filesystem::path output_file;
    std::filesystem::path parameter_file = "parameter.ini";
    std::optional<int> seed;
    int steps = 120;
    double trait4_sigmaE = 1.0;
    int bfdc = 2;
    std::vector<PenPopulation> pens;
};

enum class StartupAction {
    Run,
    ExitSuccess,
    ExitFailure
};

StartupAction handle_immediate_command(int argc, char* argv[]);
bool prepare_simulation(int argc, char* argv[], SimulationConfig& config);
void print_startup_summary(const SimulationConfig& config);

#endif
