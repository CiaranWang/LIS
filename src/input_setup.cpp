#include "input_setup.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "helptext.h"
#include "lis.h"

namespace fs = std::filesystem;
using namespace std;

static const std::string PROGRAM_VERSION = "0.7.r";

double lx = 450.0;
double ly = 350.0;
int n_theta = 8;
int n_feeder = 1;
double bodysize = 40.0;
double stepsize = 40.0;
double sensingrange = 40.0;
double sigma_blur = 60.0;
std::vector<std::array<double, 2>> feeder_coordinates;
double unit_angle = 2.0 * pi / n_theta;

double motivation_change[3][3]{ -5.0/9, +1.0/15, +2.0/45,
                                  0.0,   -5.0/9,  +5.0/24,
                                  0.0,   +5.0/9,  -5.0/24 };

static std::string trim(std::string value)
{
    const char* whitespace = " \t\r\n";
    const size_t first = value.find_first_not_of(whitespace);
    if (first == std::string::npos) {
        return "";
    }
    const size_t last = value.find_last_not_of(whitespace);
    return value.substr(first, last - first + 1);
}

static std::string normalize_key(std::string key)
{
    key = trim(key);
    std::transform(key.begin(), key.end(), key.begin(),
        [](unsigned char c) {
            if (c == ' ' || c == '-' || c == '.') {
                return '_';
            }
            return static_cast<char>(std::tolower(c));
        });
    return key;
}

static double parse_number_or_fraction(const std::string& token)
{
    const size_t slash_pos = token.find('/');
    if (slash_pos == std::string::npos) {
        return std::stod(token);
    }

    const double numerator = std::stod(token.substr(0, slash_pos));
    const double denominator = std::stod(token.substr(slash_pos + 1));
    if (denominator == 0.0) {
        throw std::runtime_error("Division by zero in parameter value: " + token);
    }
    return numerator / denominator;
}

static std::vector<double> parse_number_list(std::string value)
{
    for (char& c : value) {
        if (c == ',' || c == ';') {
            c = ' ';
        }
    }

    std::vector<double> values;
    std::istringstream iss(value);
    std::string token;
    while (iss >> token) {
        values.push_back(parse_number_or_fraction(token));
    }
    return values;
}

static int parse_positive_int_parameter(const std::string& key, const std::string& value)
{
    const double parsed = parse_number_or_fraction(value);
    const int as_int = static_cast<int>(parsed);
    if (parsed != as_int || as_int <= 0) {
        throw std::runtime_error(key + " must be a positive integer");
    }
    return as_int;
}

static double parse_positive_double_parameter(const std::string& key, const std::string& value)
{
    const double parsed = parse_number_or_fraction(value);
    if (parsed <= 0.0) {
        throw std::runtime_error(key + " must be positive");
    }
    return parsed;
}

static std::vector<std::array<double, 2>> parse_feeder_coordinates(const std::string& value)
{
    const std::vector<double> values = parse_number_list(value);
    if (values.empty() || values.size() % 2 != 0) {
        throw std::runtime_error("feeder_coordinates must contain x,y pairs");
    }

    std::vector<std::array<double, 2>> coordinates;
    coordinates.reserve(values.size() / 2);
    for (size_t i = 0; i < values.size(); i += 2) {
        coordinates.push_back({ values[i], values[i + 1] });
    }
    return coordinates;
}

static int get_bfdc(std::string bfdt)
{
    std::transform(bfdt.begin(), bfdt.end(), bfdt.begin(),
        [](unsigned char c) { return std::tolower(c); });

    if (bfdt == "gaussian" || bfdt == "normal") {
        return 0;
    }
    if (bfdt == "poisson+1") {
        return 1;
    }
    if (bfdt == "lognormal") {
        return 2;
    }
    if (bfdt == "uniform") {
        return 3;
    }

    throw std::invalid_argument("Unknown bite_force_dist: " + bfdt);
}

static const char* bite_force_dist_name(int bfdc)
{
    if (bfdc == 0) return "Gaussian/normal";
    if (bfdc == 1) return "Poisson+1";
    if (bfdc == 2) return "lognormal";
    if (bfdc == 3) return "uniform";
    return "unknown";
}

static void validate_feeder_coordinates()
{
    if (static_cast<int>(feeder_coordinates.size()) != n_feeder) {
        throw std::runtime_error(
            "feeder_coordinates must contain exactly " +
            std::to_string(n_feeder) + " x,y pairs");
    }

    for (int i = 0; i < static_cast<int>(feeder_coordinates.size()); i++) {
        const double x = feeder_coordinates[i][0];
        const double y = feeder_coordinates[i][1];
        if (!(x >= 0.0 && x <= lx && y >= 0.0 && y <= ly)) {
            throw std::runtime_error(
                "Feeder " + std::to_string(i + 1) +
                " is not in the pen area: x=" + std::to_string(x) +
                ", y=" + std::to_string(y));
        }
    }
}

static void load_parameters(const fs::path& parameter_file, double& trait4_sigmaE, int& bfdc)
{
    std::ifstream input(parameter_file);
    if (!input) {
        throw std::runtime_error("Could not open parameter file: " + parameter_file.string());
    }

    std::string line;
    std::string pending_key;
    std::string pending_value;
    int pending_line_number = 0;
    int line_number = 0;
    bool found_motivation_rate = false;
    bool found_feeder_coordinates = false;
    bool found_bite_force_sigma_e = false;
    bool found_bite_force_dist = false;

    auto process_parameter = [&](const std::string& key, const std::string& value, int key_line_number) {
        if (key == "motivation_rate") {
            const std::vector<double> values = parse_number_list(value);
            if (values.size() != 9) {
                throw std::runtime_error(
                    "motivation_rate must contain exactly 9 values at " +
                    parameter_file.string() + ":" + std::to_string(key_line_number));
            }

            int index = 0;
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 3; col++) {
                    motivation_change[row][col] = values[index++];
                }
            }
            found_motivation_rate = true;
        }
        else if (key == "lx") {
            lx = parse_positive_double_parameter(key, value);
        }
        else if (key == "ly") {
            ly = parse_positive_double_parameter(key, value);
        }
        else if (key == "n_theta") {
            n_theta = parse_positive_int_parameter(key, value);
        }
        else if (key == "n_feeder") {
            n_feeder = parse_positive_int_parameter(key, value);
        }
        else if (key == "body_size") {
            bodysize = parse_positive_double_parameter(key, value);
        }
        else if (key == "step_size") {
            stepsize = parse_positive_double_parameter(key, value);
        }
        else if (key == "sensing_range") {
            sensingrange = parse_positive_double_parameter(key, value);
        }
        else if (key == "sigma_blur") {
            sigma_blur = parse_positive_double_parameter(key, value);
        }
        else if (key == "bite_force_sigma_e") {
            trait4_sigmaE = parse_positive_double_parameter(key, value);
            found_bite_force_sigma_e = true;
        }
        else if (key == "bite_force_dist") {
            bfdc = get_bfdc(trim(value));
            found_bite_force_dist = true;
        }
        else if (key == "feeder_coordinates") {
            feeder_coordinates = parse_feeder_coordinates(value);
            found_feeder_coordinates = true;
        }
        else {
            throw std::runtime_error(
                "Unknown parameter key '" + key + "' at " +
                parameter_file.string() + ":" + std::to_string(key_line_number));
        }
    };

    auto flush_pending = [&]() {
        if (!pending_key.empty()) {
            process_parameter(pending_key, pending_value, pending_line_number);
            pending_key.clear();
            pending_value.clear();
            pending_line_number = 0;
        }
    };

    while (std::getline(input, line)) {
        line_number++;

        const size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        line = trim(line);
        if (line.empty() || (line.front() == '[' && line.back() == ']')) {
            continue;
        }

        const size_t equals_pos = line.find('=');
        if (equals_pos == std::string::npos) {
            if (!pending_key.empty()) {
                pending_value += "; " + line;
            }
            continue;
        }

        flush_pending();
        pending_key = normalize_key(line.substr(0, equals_pos));
        pending_value = trim(line.substr(equals_pos + 1));
        pending_line_number = line_number;
    }

    flush_pending();

    if (!found_motivation_rate) {
        throw std::runtime_error("Missing motivation_rate key in parameter file: " + parameter_file.string());
    }
    if (n_theta < 4 || n_theta % 4 != 0) {
        throw std::runtime_error("n_theta must be at least 4 and divisible by 4");
    }
    if (!found_feeder_coordinates) {
        throw std::runtime_error("Missing feeder_coordinates key in parameter file: " + parameter_file.string());
    }
    if (!found_bite_force_sigma_e) {
        throw std::runtime_error("Missing bite_force_sigma_e key in parameter file: " + parameter_file.string());
    }
    if (!found_bite_force_dist) {
        throw std::runtime_error("Missing bite_force_dist key in parameter file: " + parameter_file.string());
    }

    validate_feeder_coordinates();
    unit_angle = 2.0 * pi / n_theta;
}

static fs::path get_lis_root(char* argv0)
{
    fs::path exe_path = fs::absolute(argv0);
    return exe_path.parent_path().parent_path();
}

static void print_version()
{
    std::cout << "LIS Version: " << PROGRAM_VERSION << std::endl;
}

static void print_help()
{
    std::cout << LIS_HELP_TEXT << std::endl;
}

static void run_update(char* argv0)
{
    fs::path lis_root = get_lis_root(argv0);

    std::cout << "[UPDATE] Attempting to update LIS in: " << lis_root << std::endl;
    std::cout << "Make sure you have 'git', 'cmake', and 'make' installed." << std::endl;

    if (!fs::exists(lis_root / ".git") || !fs::is_directory(lis_root / ".git")) {
        std::cout << "Warning: LIS root folder is not a git repository.\n";
        std::cout << "Clone the repository and try again:\n";
        std::cout << "  git clone https://github.com/CiaranWang/LIS.git\n";
        return;
    }

    std::string git_cmd = "cd \"" + lis_root.string() + "\" && git pull origin master";
    if (system(git_cmd.c_str()) != 0) {
        std::cout << "Git pull failed.\n";
        return;
    }

    std::string build_cmd =
        "cd \"" + lis_root.string() + "\" && mkdir -p build && cd build && cmake .. && make -j 8";
    if (system(build_cmd.c_str()) != 0) {
        std::cout << "Build failed.\n";
        return;
    }

    std::cout << "Update and rebuild completed successfully!\n";
    std::cout << "You can now run: ./build/LIS [options]\n";
}

static int print_population_summary(const std::vector<PenPopulation>& pens)
{
    int total_animals = 0;
    for (const PenPopulation& pen_data : pens) {
        total_animals += static_cast<int>(pen_data.animals.size());
    }

    cout << "Parsing grouping information from input file...\n";
    cout << "Found " << total_animals << " animals in total\n";
    cout << "Found " << pens.size() << " pens\n";
    for (const PenPopulation& pen_data : pens) {
        cout << "Pen " << pen_data.pen << " has "
             << pen_data.animals.size() << " animals\n";
    }

    return total_animals;
}

static void print_parameter_summary(int steps, double trait4_sigmaE, int bfdc)
{
    cout << "Loaded simulation parameters:\n";
    cout << "  lx = " << lx << "\n";
    cout << "  ly = " << ly << "\n";
    cout << "  n_theta = " << n_theta << "\n";
    cout << "  n_feeder = " << n_feeder << "\n";
    cout << "  body_size = " << bodysize << "\n";
    cout << "  step_size = " << stepsize << "\n";
    cout << "  sensing_range = " << sensingrange << "\n";
    cout << "  sigma_blur = " << sigma_blur << "\n";
    cout << "  unit_angle = " << unit_angle << "\n";
    cout << "  simulation steps = " << steps << "\n";
    cout << "  bite_force_sigma_e = " << trait4_sigmaE << "\n";
    cout << "  bite_force_dist = " << bite_force_dist_name(bfdc) << "\n";

    cout << "  motivation_rate:\n";
    for (int row = 0; row < 3; row++) {
        cout << "    " << motivation_change[row][0] << ", "
             << motivation_change[row][1] << ", "
             << motivation_change[row][2] << "\n";
    }

    cout << "  feeder_coordinates:\n";
    for (int i = 0; i < static_cast<int>(feeder_coordinates.size()); i++) {
        cout << "    Feeder " << i + 1 << ": x=" << feeder_coordinates[i][0]
             << ", y=" << feeder_coordinates[i][1] << "\n";
    }
}

static bool parse_command_line(int argc, char* argv[], SimulationConfig& config)
{
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            config.input_file = argv[++i];
        }
        else if (arg == "--seed" && i + 1 < argc) {
            config.seed = std::stoi(argv[++i]);
        }
        else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            config.output_file = argv[++i];
        }
        else if ((arg == "-p" || arg == "--param" || arg == "--parameter") && i + 1 < argc) {
            config.parameter_file = argv[++i];
        }
        else if ((arg == "--step" || arg == "--steps") && i + 1 < argc) {
            config.steps = std::stoi(argv[++i]);
        }
        else {
            cerr << "Unknown or incomplete argument: " << arg << "\n";
            return false;
        }
    }

    if (config.input_file.empty()) {
        cerr << "Error: -i/--input input_file is required\n";
        return false;
    }

    return true;
}

StartupAction handle_immediate_command(int argc, char* argv[])
{
    if (argc <= 1) {
        return StartupAction::Run;
    }

    std::string arg1 = argv[1];
    if (arg1 == "--version" || arg1 == "-v" || arg1 == "-V") {
        print_version();
        return StartupAction::ExitSuccess;
    }
    if (arg1 == "--update" || arg1 == "-u" || arg1 == "-U") {
        run_update(argv[0]);
        return StartupAction::ExitSuccess;
    }
    if (arg1 == "--help" || arg1 == "-h" || arg1 == "-H") {
        print_help();
        return StartupAction::ExitSuccess;
    }

    return StartupAction::Run;
}

bool prepare_simulation(int argc, char* argv[], SimulationConfig& config)
{
    try {
        if (!parse_command_line(argc, argv, config)) {
            return false;
        }

        load_parameters(config.parameter_file, config.trait4_sigmaE, config.bfdc);
        config.pens = read_population(config.input_file.string());
    }
    catch (const std::exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return false;
    }

    if (config.output_file.empty()) {
        config.output_file = config.input_file.parent_path() /
            ("output_" + config.input_file.filename().string());
    }

    return true;
}

void print_startup_summary(const SimulationConfig& config)
{
    cout << "Input file:  " << fs::absolute(config.input_file) << "\n";
    cout << "Parameter file: " << fs::absolute(config.parameter_file) << "\n";
    cout << "Output file: " << fs::absolute(config.output_file) << "\n";
    print_parameter_summary(config.steps, config.trait4_sigmaE, config.bfdc);

    if (config.seed.has_value()) {
        cout << "Seed:        " << config.seed.value() << " (using deterministic per-thread RNG)\n";
    }
    else {
        cout << "Seed:        (not provided, using time-based RNG per thread)\n";
    }

    print_population_summary(config.pens);
}
