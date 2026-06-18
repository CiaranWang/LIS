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
|-- LICENSE.txt             MIT license
|-- README.md               Project documentation
|-- input_example.txt       Example phenotype input file
|-- parameter.ini           Default model parameter file
|-- lis_test.sh             Example SLURM job script
|-- include/                Header files
`-- src/                    C++ source files
```

## Requirements

- CMake 3.12 or newer
- A C++20 compiler
- OpenMP

The code has been used with CMake/Ninja on Windows and includes presets for Windows, Linux, and macOS. On Linux clusters, the example `lis_test.sh` script shows a SLURM-style execution pattern.

LIS uses OpenMP to run pens in parallel. By default, it will use as many CPU threads as the OpenMP runtime makes available. On shared systems or clusters, limit this externally, for example with scheduler CPU settings or `OMP_NUM_THREADS`.

## Installation

Clone the repository and build LIS from source:

```bash
git clone https://github.com/CiaranWang/LIS.git
cd LIS
mkdir -p build
cd build
cmake ../
make -j 8
```

The executable will be created in the `build` directory:

```bash
./LIS --help
```

## Usage

```bash
./LIS --input input_example.txt --param parameter.ini --seed 123456 --output result.txt --steps 100
```

Options:

```text
-h, -H, --help                    Show help
-v, -V, --version                 Print version
-u, -U, --update                  Update from GitHub and rebuild

-i PATH, --input PATH             Input phenotype file
-p PATH, --param PATH,
--parameter PATH                  Parameter file, default: parameter.ini
-o PATH, --output PATH            Output result file

--seed N                          Integer random seed
--step N, --steps N               Number of simulation steps
```

The parameter file is required at runtime. If `-p`, `--param`, or `--parameter` is not provided, LIS looks for `parameter.ini` in the current working directory. If that file is missing, the program stops with an error before running simulations.

If `-o` or `--output` is not provided, LIS writes to `output_<input filename>` in the same directory as the input file. For reproducible workflows, especially batch runs, it is recommended to provide an explicit output path with `--output`.

Supported `bite_force_dist` values:

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

## Motivation And Behaviour

Each animal has three internal motivation values: eating, resting, and walking. In the current model, the threshold is fixed at 100 for each behaviour. When a motivation reaches or exceeds 100, the corresponding behaviour can be triggered.

At each simulation step, LIS uses the following decision tree:

1. If the animal's current behaviour still has positive motivation, the animal continues that behaviour.
2. Otherwise, LIS checks eating motivation first. If eating motivation is at or above the threshold, the animal eats.
3. If eating is not triggered, LIS checks resting motivation. If resting motivation is at or above the threshold, the animal rests.
4. If resting is not triggered, LIS checks walking motivation. If walking motivation is at or above the threshold, the animal walks.
5. If none of the motivations reaches the threshold, the animal rests by default.

This means that when multiple motivations are above threshold at the same time, eating has priority over resting, and resting has priority over walking.

After an animal performs a behaviour, the motivation-rate matrix updates all three motivations. Rows are the motivations being updated, and columns are the behaviour performed during the current step:

```text
                  performed behaviour
                  eat      rest     walk
updated eat       m00      m01      m02
updated rest      m10      m11      m12
updated walk      m20      m21      m22
```

For example, if an animal walks, LIS uses the `walk` column of the matrix to update eating, resting, and walking motivations. Users can tune the entries in `motivation_rate` in `parameter.ini` to create different behaviour-cycle dynamics.

## Social Traits

The input file provides four traits for each animal:

- `Trait_1`: performer effect
- `Trait_2`: recipient effect
- `Trait_3`: social tendency
- `Trait_4`: bite/peck force or interaction intensity trait

`Trait_3` affects movement during walking. Animals with different social tendency values differ in how strongly their movement direction is pulled toward or away from local animal density.

When two animals are within the sensing range, LIS calculates a directed interaction probability from the performer animal's `Trait_1` and the receipient animal's `Trait_2`:

```text
p(performer -> receipient) = logistic(performer Trait_1 + receipient Trait_2)
```

The current logistic function is:

```text
logistic(x) = 1 / (1 + exp(-x))
```

LIS then draws a 0/1 interaction outcome from this probability. If the outcome is 1, an interaction is recorded and an observed interaction intensity is generated from the performer animal's `Trait_4`. The uncertainty or residual variation of this observed intensity is controlled by `bite_force_sigma_e` in `parameter.ini`, and the observation distribution is selected with `bite_force_dist`.

## Parameter File

By default, LIS reads model parameters from `parameter.ini`. A different file can be provided with `-p`, `--param`, or `--parameter`. The parameter file is required.

The motivation-rate matrix is read from the `motivation_rate` key:

```ini
motivation_rate =
    -5/9,  1/15,  2/45
       0,  -5/9,  5/24
       0,   5/9, -5/24
```

The matrix contains 9 values. Rows are the motivations being updated: eat, rest, and walk. Columns are the behaviour performed in the current step: eat, rest, and walk. Values may be decimals or simple fractions.

The same file also contains the main pen, feeder, movement, sensing, and interaction parameters:

```ini
lx = 450
ly = 350
n_theta = 8
n_feeder = 1
feeder_coordinates =
    225, 175
body_size = 40
step_size = 40
sensing_range = 40
sigma_blur = 60
bite_force_sigma_e = 1
bite_force_dist = lognormal
```

`n_theta` must be at least 4 and divisible by 4 because the movement-direction logic uses quarter-turn calculations.

`feeder_coordinates` must contain one `x,y` pair for each feeder. For multiple feeders, write one pair per continuation line:

```ini
n_feeder = 3
feeder_coordinates =
    50, 50
    225, 175
    400, 300
```

Before running the simulation, LIS checks that every feeder is inside the pen area: `x >= 0`, `x <= lx`, `y >= 0`, and `y <= ly`. If a feeder is outside the pen, the program prints an error naming the feeder and its coordinates.

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

Several core model settings are read from `parameter.ini`, including pen dimensions, number of movement directions, number of feeders, feeder coordinates, body size, movement step size, sensing range, density blur, bite-force observation settings, and the motivation-rate matrix.

## Example SLURM Run

The `lis_test.sh` file gives an example cluster job:

```bash
rm -rf lis_test/
mkdir -p lis_test

./build/LIS --input ./input_example.txt --param ./parameter.ini --seed 123456 --steps 100 --output lis_test/test_output.txt
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

## Acknowledgements

This work was financially supported by the Dutch Research Council (NWO-TTW) and Hendrix Genetics BV (Boxmeer, the Netherlands) through the SmartTurkeys project (project number 17238), under the NWO-TTW Open Technology Program.

## License

This project is released under the MIT License. See `LICENSE.txt` for details.
