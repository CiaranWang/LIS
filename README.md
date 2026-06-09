# LIS

LIS is a C++ agent-based simulation for studying animal behaviour and directed social interactions in group-housed animals. It was developed around pen-level simulations of turkey behaviour, but the model structure is useful for academic work on movement, motivation-driven behaviour, encounter rates, and interaction phenotypes.

The program simulates animals moving in a rectangular pen, choosing between feeding, resting, and walking, and producing directed interaction summaries between animal pairs.

## What the Model Simulates

Each simulated animal has:

- an identifier and pen assignment
- a two-dimensional position in the pen
- motivation values for feeding, resting, and walking
- four phenotype/trait values:
  - `Trait_1`: performer effect
  - `Trait_2`: recipient effect
  - `Trait_3`: social tendency
  - `Trait_4`: bite/peck force or interaction intensity trait

At each simulation step, animals update behaviour, move through the pen, respond to nearby animals, and may interact when they are within the sensing range. The final output is a directed dyad table with counts of meetings, interactions, and bites/pecks.

## Repository Layout

```text
.
|-- CMakeLists.txt          CMake build configuration
|-- CMakePresets.json       Visual Studio/Ninja build presets
|-- LICENSE.txt             MIT license
|-- README.md               Project documentation
|-- input_example.txt       Example phenotype input file
|-- parameter.ini           Default model parameter file
|-- lis_test.sh             Example SLURM job script
|-- include/                Header files
|-- src/                    C++ source files
`-- var_check/              Validation/checking files and R script
```

## Requirements

- CMake 3.12 or newer
- A C++20 compiler
- OpenMP

The code has been used with CMake/Ninja on Windows and includes presets for Windows, Linux, and macOS. On Linux clusters, the example `lis_test.sh` script shows a SLURM-style execution pattern.

## Build

From the project root:

```bash
cmake -S . -B build
cmake --build build
```

If using CMake presets, for example on Windows:

```bash
cmake --preset x64-release
cmake --build out/build/x64-release
```

The executable is named `LIS`.

## Usage

```bash
./LIS -i input_example.txt -p parameter.ini --seed 123456 -o result.txt --step 100 --BiteForceSigmaE 0.37 --BiteForceDist lognormal
```

Options:

```text
-h, -H, --help                    Show help
-v, -V, --version                 Print version
-u, -U, --update                  Update from GitHub and rebuild

-i PATH                           Input phenotype file
-p PATH, --param PATH             Parameter file, default: parameter.ini
-o PATH                           Output result file
--seed N                          Integer random seed
--step N                          Number of simulation steps
--BiteForceSigmaE SIGMA           Standard deviation for observed bite force
--BiteForceDist TYPE              Bite force observation distribution
```

Supported `--BiteForceDist` values:

- `Gaussian` or `normal`
- `Poisson+1`
- `lognormal`
- `uniform`

Distribution names are case-insensitive.

## Input Format

The input file is a whitespace-delimited phenotype table with a header:

```text
ID Pen Trait_1 Trait_2 Trait_3 Trait_4
F1578_4 1 -0.688262526564818 1.57855113181651 0.903732845244942 -2.32434185156005
```

The program scans this file before running the simulation and automatically determines:

- the total number of animals
- the number of pens
- the animals belonging to each pen

Pens may contain different numbers of animals. The input does not need to contain exactly 14 animals per pen, and the simulation no longer assumes exactly 400 pens.

## Parameter File

By default, LIS reads model parameters from `parameter.ini`. A different file can be provided with `-p`, `--param`, or `--parameter`.

The motivation-rate matrix is read from the `motivation_rate` key:

```ini
motivation_rate = -5/9, 1/15, 2/45; 0, -5/9, 5/24; 0, 5/9, -5/24
```

The matrix contains 9 values. Rows are the motivations being updated: eat, rest, and walk. Columns are the behaviour performed in the current step: eat, rest, and walk. Values may be decimals or simple fractions.

## Output Format

The output is a tab-delimited directed dyad table:

```text
performer receiver pen nr_meet nr_interact nr_bites
```

Columns:

- `performer`: animal initiating the directed interaction
- `receiver`: animal receiving the directed interaction
- `pen`: pen number
- `nr_meet`: number of times the pair was close enough to meet
- `nr_interact`: number of realised interactions
- `nr_bites`: accumulated bite/peck count or intensity

The program runs pens in parallel with OpenMP, writes temporary per-pen output files, merges them into the requested output file, and removes the temporary files.

## Reproducibility

Use `--seed` for deterministic random number generation. The current implementation derives a per-pen random seed as:

```text
thread_seed = seed + pen
```

This makes repeated runs reproducible for the same executable, input file, model constants, and command-line options.

## Model Constants

Several core model settings are currently defined in `include/lis.h`, including:

- pen dimensions
- number of feeders
- feeder size
- animal body size
- movement step size
- sensing range
- movement direction discretisation
- density blur parameter

The motivation-rate matrix is already read from `parameter.ini`. For broader academic use, the remaining compiled constants should eventually move into the same parameter file or command-line options.

## Example SLURM Run

The `lis_test.sh` file gives an example cluster job:

```bash
srun -c 16 ~/LIS/build/LIS -i ./input_example.txt -p ./parameter.ini --seed 123456 --step 1000 -o test_output.txt
```

Adjust paths, CPU count, memory, and wall time for your local cluster environment.

## Public-Release Notes

This project is close to being usable as public academic software, but the following items should be addressed before a formal public release:

- replace the placeholder copyright fields in `LICENSE.txt`
- document the biological interpretation and units of all model parameters
- add automated tests for input parsing, random-number reproducibility, and key probability functions
- add a small input file for quick smoke tests
- document expected output for the example input
- decide whether generated files such as build outputs, `.vs/`, `.Rhistory`, and large result files should remain outside version control
- align the compiled binary version with the source version before release

## License

This project is released under the MIT License. See `LICENSE.txt` for details.
