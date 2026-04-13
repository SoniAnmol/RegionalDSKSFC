# Copyright (C) 2025 Anmol Soni, Tania Teribech, Tina Comes, Giuli Piccllio, Francesco Lamperti, Andrea Roventini 
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

########################################################################################

Regional DSK-SFC Model
======================

This repository contains the regionalised version of the DSK-SFC model (R-DSK). It extends the national-level DSK-SFC model from Reissl et al. (2025) with multiple regions, regional government fiscal policy (social protection and fiscal blocks), and region-specific climate shock parameters.


#################
Model Structure
#################

- dsk_sfc_main.cpp is the main model script containing the main simulation loop and a majority of model functions
- The "modules" folder contains domain-specific functions, each with a .cpp and .h file:
  - module_climate_sfc  : C-Roads climate box, cumulative emissions, climate policy (CO2 tax), climate shocks (one-off and repeated)
  - module_energy_sfc   : Energy demand, production, investment, R&D, emissions by industry
  - module_macro_sfc    : Labour allocation, macro aggregates, wage dynamics, government budget, Taylor rule, regional government fiscal block (social protection & fiscal)
  - module_finance_sfc  : Credit determination, loan rates, banking profits, bailouts, interbank settlement
- dsk_sfc_functions.h declares the model functions
- dsk_sfc_parameters.h declares the model parameters
- dsk_sfc_inits.h declares the model initial values
- dsk_sfc_flags.h declares the model flags (indicator variables for simulation settings)
- dsk_sfc_globalvars.h declares global variables including regional matrices
- dsk_sfc_include.h declares all libraries and other files needed
- The "auxiliary" folder contains functions for quasi-random number generation (betadev, bnldev, gasdev, gammln, ran1)
- The "newmat10" folder contains the Newmat C++ matrix library
- The "rapidjson" folder contains the RapidJSON header-only library for parsing JSON input files
- The "include" folder contains the CLI11 header-only library for command-line argument parsing


################
Input Files
################

All input files are JSON-formatted and supply parameters, flags, initial values, and regional configuration.

Structure of an input JSON file (top-level keys):
  params, climparams, climshockparams, flags, inits, climinits, regions, shocks_kfirms, shocks_cfirms, shock_scalar

The "inputs" folder contains:

  dsk_sfc_inputs.json                          : Default single-region configuration
  regions_input.json                           : Example regional configuration (NR=3, heterogeneous regions)

  homogenous_regions_homogenous_shocks/        : Verification scenarios (NR=4, identical regions, identical shocks)
    scenario1_inputs.json                        : Baseline (no shocks): flag_shockexperiment=0
    scenario2_inputs.json                        : Capital stock shocks: flag_shockexperiment=1, flag_capshocks=1
    scenario3_inputs.json                        : Productivity shocks:  flag_shockexperiment=1, flag_prodshocks1=6

  heterogenous_regions_homogenous_shocks/      : Heterogeneous regions with homogeneous shocks (5 scenarios)
  heterogenous_regions_largemoreexposed/       : Large region more exposed to climate shocks (5 scenarios)
  heterogenous_regions_smallmoreexposed/       : Small region more exposed to climate shocks (5 scenarios)


#########################
Compilation instructions
#########################

The model requires a C++17 compatible compiler and CMake 3.10+.

#####################
Compilation in Linux:

- Open the console and execute:
'sudo apt-get update'
'sudo apt-get install build-essential gdb cmake'
- cd into the source directory, then execute:
'cmake .'
'make'

#####################
Compilation in macOS:

- Install Homebrew; open the terminal and execute:
'/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"'
- Install cmake:
'brew install cmake'
- cd into the source directory, then execute:
'cmake .'
'make'

#####################################################################
Compilation on Windows using WSL and VS Code:

- Make sure that WSL is enabled in Windows features
- Install Ubuntu from the Microsoft store
- Open the Ubuntu console and execute:
'sudo apt-get update'
'sudo apt-get install build-essential gdb cmake'
- Install VS Code
- In VS Code, install the Remote-WSL extension
- Install C/C++, CMake, and CMake Tools extensions in the remote
- Open the folder containing the model in the remote VS Code window
- Update the path in CMakeSettings.json "wslPath" (line 25) to match your machine
- Compile:
'cmake .'
'make'

###################################
Compilation in Windows using MinGW:

- Install Chocolatey (see https://chocolatey.org/install) using an Admin PowerShell, then execute:
'choco install mingw cmake'
- Ensure the Path includes C:\ProgramData\chocolatey\bin and C:\Program Files\cmake\bin
- Open a command prompt and cd into the source directory, then execute:
'cmake -G "MinGW Makefiles" .'
'cmake --build .'


#################
Running the model
#################

The compiled executable dsk_SFC accepts the following arguments:

1. inputfile (required) : Path to a JSON file containing parameters, flags, and initial values
2. -r, --run            : Run name (no spaces), appended to output filenames (default="test")
3. -s                   : Seed, a positive integer (default=1)
4. -f, --fulloutput     : Full output (1) or reduced output (0) (default=0)
5. -c, --cerr           : Print errors to console (1) or only to log file (0) (default=0)
6. -v, --verbose        : Print progress updates to console (1=yes, 0=no) (default=0)

To see usage instructions:
'./dsk_SFC --help'

Example (Linux/macOS):
'./dsk_SFC inputs/dsk_sfc_inputs.json -r test -s 1 -f 0 -c 0 -v 0'

Example (Windows):
'.\dsk_SFC inputs\dsk_sfc_inputs.json -r test -s 1 -f 0 -c 0 -v 0'


################
Output Files
################

All output files are saved under the "output" folder (created automatically).
Error logs are saved under "output/errors".

For each run, the model produces:
  ymc_<runname>_<seed>.txt                : Macro-level time series
  ymc_<region>_<runname>_<seed>.txt       : Regional time series (one per region, if NR > 0)

When flag_shockexperiment=1, additionally:
  resultsexp_<runname>_<seed>.txt         : Shock experiment results (national)
  resultsexp_reg<N>_<runname>_<seed>.txt  : Shock experiment results per region (if NR > 0)
  shockpars_<runname>_<seed>.txt          : Shock parameter draws

When -f 1 (full output), firm-level matrices are also saved.


##############################
Batch runs with run_scenarios.py
##############################

The script run_scenarios.py automates batch execution across multiple scenarios with Monte Carlo replications and parallel processing.

Arguments:
  -d, --scenarios-dir   : Directory containing scenario input files (default=./inputs/experiment)
  -o, --output          : Base output directory (default=./output/experiment)
  -e, --executable      : Path to model executable (default=./dsk_SFC)
  -n, --replications    : Monte Carlo replications per scenario (default=1)
  -s, --seed-base       : Base seed value (default=1)
  -f, --full-output     : Enable full firm-level output
  -q, --quiet           : Suppress verbose output
  -w, --workers         : Parallel workers: 0=auto-detect CPUs, 1=sequential (default=1)

Example:
'python3 run_scenarios.py -d inputs/homogenous_regions_homogenous_shocks -o output/verification -n 50 -w 0'

Output is organised hierarchically: <output>/<scenario>/<rep_NNN>/


#####################
Verification Scenarios
#####################

The homogenous_regions_homogenous_shocks input set recreates three scenarios from Reissl et al. (2025) on the regionalised model with NR=4 identical regions:

  Scenario 1 (Baseline)           : flag_shockexperiment=0
  Scenario 2 (Capital shocks)     : flag_shockexperiment=1, flag_capshocks=1
  Scenario 3 (Productivity shocks): flag_shockexperiment=1, flag_prodshocks1=6

These scenarios are simulated for 600 periods (first 200 discarded as transient).

Key flags for shock behaviour:
  flag_exogenousshocks  : 1 = single exogenous shock (SINGLESHOCK), 0 = repeated endogenous shocks (SHOCKS)
  flag_uniformshocks    : 1 = single draw broadcast to all firms per region, 0 = independent per-firm draws


#################
Utility Scripts
#################

scripts/plot_resultsexp.py   : Visualise national-level shock experiment results
scripts/plot_ymc.py          : Plot macroeconomic time series
scripts/verification.py      : Compare R-DSK vs DSK baseline outputs
verify_uniformshock0.py      : Verification script: KS-test comparison of R-DSK vs DSK across all scenarios

fnModifyInputs.R             : R helper for modifying JSON input files
runPar.R                     : R script for parallel model execution
