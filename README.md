# Gravity-Sim

A powerful GPU implementation of the direct-sum and Barnes-Hut N-body simulation algorithms, using the Vulkan graphics API.

![A screenshot of the application](/images/Simulation.png)

## Features

* Implemented graphics and computation modes
* Direct sum and Barnes-Hut algorithm implementations
* Benchmark mode
* Four simulation generation presets, with the option for custom particle inputs

## Performance

Tested on an NVIDIA GeForce RTX 3070 and a Ryzen 5 5600X, the average time for force calculations is:

* 3.602ms for 50000 particles using the direct sum algorithm
* 2.853ms for 500000 particles using the Barnes-Hut algorithm

## Requirements

* A C++ compiler with support for C++20
* CMake version 3.5 or higher
* Vulkan SDK version 1.3 or higher

## Options

The same list can be queried by using the `--help` command line argument.

### Available parameters

* `--log-file`: The file to output all logs to. If unspecified, logs will not be outputted to a file
* `--particles-in`: The input file containing the starting variables of the particles. If not specified, the program will use the given generation parameters
* `--particles-out`: The optional output file in which the variables of the particles will be written once the simulation is finished
* `--particle-count`: The number of particles to generate. All generation parameters are ignored if an input file is specified
* `--generate-type`: The variant to use for the particle system generation. One of the following options:
    * `random`: Randomly distribures particles within the generation confines
    * `galaxy`: Simulates the approximate structure of a spiral galaxy spanning the generation confines
    * `galaxy-collision`: Simulates the collision of two sipral galaxies, each spanning a third of the generation confines
    * `symmetrical-galaxy-collision`: Simulates the collision of two symmetrical spiral galaxies, each spanning a third of the generation confines
* `--generate-size`: The radius of the resulting generation's size
* `--min-mass`: The minimum mass of the generated particles
* `--max-mass`: The maximum mass of the generated particles
* `--gravitational-const`: The gravitational constant used for the simulation. Defaulted to 1
* `--simulation-time`: The time interval length, in seconds, simulated in one instance. Defaulted to 1e-3
* `--simulation-speed`: The speed factor at which the simulation is run. Defaulted to 1
* `--softening-len`: The softening length used to soften the extreme forces that would usually result from close interactions. Defaulted to 0.2
* `--accuracy-parameter`: The accuracy parameter used to calibrate force approximation. Only used for Barnes-Hut simulations. Defaulted to 1
* `--simulation-algorithm`: The simulation algorithm used to calculate the gravitational forces. One of the following options
    * `direct-sum`: The direct-sum method, calculating every interaction between particles
    * `barnes-hut`: The Barnes-Hut algorithm, organizing all particles in a quadtree
* `--simulation-count`: The number of simulations to run before closing the program. No limit will be used if this parameter isn't specified

### Available options:

* `--help`: Displays all parameters and options and exits the program
* `--log-detailed`: Outputs non-crucial logs that might be useful for debugging or additional information
* `--no-graphics`: Doesn't display the live positions of all particles, instead running the simulations in the background
* `--benchmark`: Benchmarks the required runtime for all simulations. Ignored if `--no-graphics` isn't specified.