# Copyright (C) 2025 Francesco Lamperti, Severin Reissl, Luca Fierro, Andrea Roventini 
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

The folder contains all files necessary to compile the model and simulate the scenarios shown in the paper.

The model is structured as follows:
- dsk_sfc_main.cpp is the main model script which contains the main simulation loop as well as a majority of model functions
- Some functions pertaining to climate, the energy sector, finance, and macroeconomic aspects are contained in separate scripts contained in the "modules" folder. Each .cpp file comes with an associated header file
- dsk_sfc_functions.h declares the model functions
- dsk_sfc_parameters.h declares the model parameters
- dsk_sfc_inits.h declares the model initial values
- dsk_sfc_flags.h declares the model flags (indicator variables used to specify different simulation settings)
- dsk_sfc_include.h declares all libraries and other files needed
- The folder "auxiliary" contains a range of functions used in the code, especially related to the generation of quasi-random numbers
- dsk_sfc_inputs.json is the input file which supplies parameters, flags and initial values
 

#########################
Compilation instructions:
#########################

As highlighted in the paper, results of individual simulation runs may differ across machines and operating systems due to use of different compilers, compiler versions, compiler options, as well as differences in processor architecture. To precisely reproduce the results reported in the paper, we recommend simulating the Linux executable supplied with the model code. The calibration and simulation runs shown in the paper were produced on the High Performance Cluster of the Euro-Mediterranean Center on Climate change (CMCC), running Linux CentOS 7.6 x86\_64 on compute nodes with Intel Xeon Gold 6154 CPUs. Results were subsequently reproduced on the corresponding author's computer running Ubuntu 20.04.4 LTS in WSL2 on an Intel Core i7-1165G7 CPU. The executable was compiled on the corresponding author's computer using GNU GCC 9.4.0.

#####################################################################
Compilation and debug runs on Windows machines using WSL and VS Code:

- Make sure that WSL is enabled in Windows features
- Install Ubuntu from the Microsoft store
- Open the Ubuntu console and execute the following commands:
'sudo apt-get update'
'sudo apt-get install build-essential gdb cmake'
- Install VS code
- In VS code, install the Remote-WSL extension
- Having done so, install the following VS Code extensions in the remote:
	- C/C++
	- CMake
	- CMake Tools
- Inside the remote VS Code window, open the folder containing the DSK-SFC model
- Change the path given in the CMakeSettings.json file, "wslPath" (line 25), to the appropriate path on your machine
- In the file CMakeLists.txt, ensure that the option CMAKE_BUILD_TYPE is set as set(CMAKE_BUILD_TYPE Debug)
- Compile the model by executing:
'cmake .'
'make'
- Using the launch configuration contained in the .vscode folder, you should be able to perform debug runs in VSCode by pressing F5 once the model has been compiled.

#####################
Compilation in Linux:

- Open the console and type and execute:
'sudo apt-get update'
'sudo apt-get install build-essential gdb cmake'
- cd into the DSK-SFC source directory, then type and execute:
'cmake .'
'make'

#####################
Compliation in macOS:

- Install Homebrew; open the terminal and execute:
'/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"'
- Install cmake; execute:
'brew install cmake'
- cd into the DSK-SFC source directory, then type and execute:
'cmake .'
'make'

###################################
Compilation in Windows using MinGW:

- Install Chocolatey (see https://chocolatey.org/install) using an Admin PowerShell (e.g. open via Win + X), then type and execute:
'choco install mingw cmake'
- Make sure the Path environmental variable includes C:\ProgramData\chocolatey\bin and C:\Program Files\cmake\bin (e.g. you should be able to invoke gcc and cmake in a command prompt)
- Open a command prompt and cd into the DSK-SFC source directory, then type and execute:
'cmake -G "MinGW Makefiles" .'
'cmake --build .'



#################
Running the model
#################

The compiled executable dsk_SFC takes several arguments to be entered when the executable is called from the console:

1. The path to a json file containing parameter values, initial values and flags
2. -r, A name for the run (without spaces), which will be appended, along with the seed, to the name of every output file generated (default="test")
3. -s, The seed, which should be a positive integer (default=1)
4. -f, A dummy, indicating whether full output (1) or reduced output (0) should be written to files (default=0)
5. -c, A dummy indicating whether error messages should be printed to the console (1) or only saved to the error log file (0) (default=0)
6. -v, A dummy indicating whether simulation progress updates should be printed to the console (1=yes, 0=no) (default=0)

Arguments 2-6 are optional; default values will be used if they are not supplied. To see instructions execute:

'./dsk_SFC --help'
 
in Linux/macOS or

'.\dsk_SFC --help'

in Windows.

Example Linux and macOS:

'./dsk_SFC dsk_sfc_inputs.json -r test -s 1 -f 0 -c 0 -v 0'

Example Windows:

'.\dsk_SFC dsk_sfc_inputs.json -r test -s 1 -f 0 -c 0 -v 0'

All simulated data will be saved in a folder named "output" located in the same directory as the executable dsk_SFC; this folder will be created if it does not exist yet. All error log files will be saved in a folder named "output/errors" located in the same directory as the executable dsk_SFC; this folder will be created if it does not exist yet.


#########
Scenarios
#########

To simulate the scenarios shown in the paper, the relevant flags in the input file dsk_sfc_inputs.json must be set to the values given below. All instructions are provided starting from the unchanged .json file found in the folder.

1. Validation run:
set flag_validation=1
set flag_shockexperiment=0

2. Baseline run:
No changes needed w.r.t. configuration provided

3. Carbon tax experiment:
set flag_desc=1

4. Climate shocks:
	Capital stock shocks:
	set flag_exogenousshocks=0
	set flag_capshocks=1

	Productivity shocks:
	set flag_exogenousshocks=0
	set flag_prodshocks1=6

5. Unemployment benefit scenarios:
set flag_desc=2 (small increase in benefit ratio)
set flag_desc=3 (large increase in benefit ratio) 


6. Additional experiments (appendix):

6.1 High carbon tax + endogenous maximum expansion rate of green capacity:
set flag_desc=1
set flag_energy_exp=4

6.2 Lower carbon tax + green energy investment subsidy + endogenous maximum expansion rate of green capacity:
set flag_desc=4
set flag_energy_exp=4

6.3 Green energy investment subsidy + public R&D + endogenous maximum expansion rate of green capacity:
set flag_desc=5
set flag_energy_exp=4


All scenarios shown in the paper are simulated for 600 periods, of which the first 200 are discarded as a transient during analysis of output data.
The model is simulated 108 times with seeds 1-108 for each scenario.
We include example R scripts which can be used to perform parallel runs of the model and to automatically modify values in the dsk_sfc_inputs.json file.
