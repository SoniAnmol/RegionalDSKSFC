// Include Libraries
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <algorithm>
#include <iomanip>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <fstream>
#include <string>
#include <string.h> 
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#include <random>
#include <utility> 
#include <stdexcept> 
#include <fenv.h>
#include "newmat10/include.h"
#include "newmat10/newmat.h"
#include "newmat10/newmatio.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "include/CLI11/CLI11.hpp"

// Include External Files
#include "auxiliary/ran1.h"					// Generator of a U[0,1] 
#include "auxiliary/gasdev.h"				// Generator of a N(0,1)
#include "auxiliary/bnldev.h"				// Generator of a Bin(n,p)
#include "auxiliary/betadev.h"				// Generator of a Beta(alfa,beta)
#include "auxiliary/gammln.h"				// Logarithm of the Gamma function

using std::string;
using std::stringstream;

// Include global variables, function, initial values and flags
#include "dsk_sfc_globalvars.h"
#include "dsk_sfc_functions.h"
#include "dsk_sfc_inits.h"
#include "dsk_sfc_flags.h"

// Include modules
#include "modules/module_finance_sfc.h"
#include "modules/module_macro_sfc.h"
#include "modules/module_energy_sfc.h"
#include "modules/module_climate_sfc.h"

