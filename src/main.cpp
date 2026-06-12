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
#include <vector>
#include <stdexcept>
#include <array>

#include "random.h"
#include "animal.h"
#include "move.h"
#include "population.h"
#include "act.h"
#include "lis.h"
#include "helptext.h"

namespace fs = std::filesystem;
using namespace std;

static const std::string PROGRAM_VERSION = "0.6.r";
 
double lx = 450.0;
double ly = 350.0;
int n_theta = 8;
int n_feeder = 1;
double bodysize = 40.0;
double stepsize = 40.0;
double sensingrange = 40.0;
double sigma_blur = 60.0;
std::vector<std::array<double, 2>> feeder_coordinates;
double unit_angle = 2.0 * pi / n_theta; //discrete moving angles
							    //eat    rest    walk

double motivation_change[3][3]{ -5.0/9,	+1.0/15,	+2.0/45,	    //eat
								   0.0,	-5.0/9 , 	+5.0/24,	    //rest
								   0.0,	+5.0/9 ,	-5.0/24};	    //walk

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

static void load_parameters(const fs::path& parameter_file)
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
        else if (key == "bodysize") {
            bodysize = parse_positive_double_parameter(key, value);
        }
        else if (key == "stepsize") {
            stepsize = parse_positive_double_parameter(key, value);
        }
        else if (key == "sensingrange") {
            sensingrange = parse_positive_double_parameter(key, value);
        }
        else if (key == "sigma_blur") {
            sigma_blur = parse_positive_double_parameter(key, value);
        }
        else if (key == "feeder_coordinates") {
            feeder_coordinates = parse_feeder_coordinates(value);
            found_feeder_coordinates = true;
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

    validate_feeder_coordinates();

    unit_angle = 2.0 * pi / n_theta;
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

static const char* bite_force_dist_name(int bfdc)
{
    if (bfdc == 0) return "Gaussian/normal";
    if (bfdc == 1) return "Poisson+1";
    if (bfdc == 2) return "lognormal";
    if (bfdc == 3) return "uniform";
    return "unknown";
}

static void print_parameter_summary(int steps, double trait4_sigmaE, int bfdc)
{
    cout << "Loaded simulation parameters:\n";
    cout << "  lx = " << lx << "\n";
    cout << "  ly = " << ly << "\n";
    cout << "  n_theta = " << n_theta << "\n";
    cout << "  n_feeder = " << n_feeder << "\n";
    cout << "  bodysize = " << bodysize << "\n";
    cout << "  stepsize = " << stepsize << "\n";
    cout << "  sensingrange = " << sensingrange << "\n";
    cout << "  sigma_blur = " << sigma_blur << "\n";
    cout << "  unit_angle = " << unit_angle << "\n";
    cout << "  simulation steps = " << steps << "\n";
    cout << "  BiteForceSigmaE = " << trait4_sigmaE << "\n";
    cout << "  BiteForceDist = " << bite_force_dist_name(bfdc) << "\n";

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

static void print_version() {
    std::cout << "LIS Version: " << PROGRAM_VERSION << std::endl;
}

static void print_help() {
    std::cout << LIS_HELP_TEXT << std::endl;
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

    if (bfdt == "gaussian" || bfdt == "normal") {
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
    fs::path parameter_file = "parameter.ini";
    optional<int> seed; // <<< optional seed
    int steps = 120; //default steps 2min
    double trait4_sigmaE = 1.0;
    int bfdc = 2;
    // ----- Parse command line arguments -----
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            input_file = argv[++i];
        }
        else if (arg == "--seed" && i + 1 < argc) {
            seed = std::stoi(argv[++i]);  // <<< store in optional
        }
        else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            output_file = argv[++i];
        }
        else if ((arg == "-p" || arg == "--param" || arg == "--parameter") && i + 1 < argc) {
            parameter_file = argv[++i];
        }
        else if ((arg == "--step" || arg == "--steps") && i + 1 < argc) {
            steps = std::stoi(argv[++i]);
        }
        else if ((arg == "--BiteForceSigmaE" || arg == "--bite-force-sigma-e") && i + 1 < argc) {
            trait4_sigmaE = std::stod(argv[++i]);
        }
        else if ((arg == "--BiteForceDist" || arg == "--bite-force-dist") && i + 1 < argc) {
            bfdc = get_bfdc(argv[++i]);
        }
        else {
            cerr << "Unknown or incomplete argument: " << arg << "\n";
            return 1;
        }
    }

    if (input_file.empty()) {
        cerr << "Error: -i/--input input_file is required\n";
        return 1;
    }

    try {
        load_parameters(parameter_file);
    }
    catch (const std::exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::vector<PenPopulation> pens;
    try {
        pens = read_population(input_file.string());
    }
    catch (const std::exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    // ----- Set default output path if not provided -----
    if (output_file.empty()) {
        output_file = input_file.parent_path() / ("output_" + input_file.filename().string());
    }

    cout << "Input file:  " << fs::absolute(input_file) << "\n";
    cout << "Parameter file: " << fs::absolute(parameter_file) << "\n";
    cout << "Output file: " << fs::absolute(output_file) << "\n";
    print_parameter_summary(steps, trait4_sigmaE, bfdc);

    if (seed.has_value()) {
        cout << "Seed:        " << seed.value() << " (using deterministic per-thread RNG)\n";
    }
    else {
        cout << "Seed:        (not provided, using time-based RNG per thread)\n";
    }

    print_population_summary(pens);

    // ---- Run pens in parallel with OpenMP ----
    #pragma omp parallel for schedule(dynamic)
    for (int pen_index = 0; pen_index < static_cast<int>(pens.size()); ++pen_index)
    {
        const PenPopulation& pen_data = pens[pen_index];
        const int pen = pen_data.pen;
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

        bool write_header = (pen_index == 0);
        run_pen(pen_data.animals, pen, per_out, write_header, steps, trait4_sigmaE, bfdc, rng);
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

    for (const PenPopulation& pen_data : pens)
    {
        const int pen = pen_data.pen;
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
