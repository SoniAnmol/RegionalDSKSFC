cmake_minimum_required(VERSION 3.10)

project(dsk_SFC LANGUAGES C CXX)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Source files (CPP only)
set(SOURCE_FILES
    newmat10/bandmat.cpp
    newmat10/cholesky.cpp
    newmat10/evalue.cpp
    newmat10/fft.cpp
    newmat10/hholder.cpp
    newmat10/jacobi.cpp
    newmat10/myexcept.cpp
    newmat10/newfft.cpp
    newmat10/newmat1.cpp
    newmat10/newmat2.cpp
    newmat10/newmat3.cpp
    newmat10/newmat4.cpp
    newmat10/newmat5.cpp
    newmat10/newmat6.cpp
    newmat10/newmat7.cpp
    newmat10/newmat8.cpp
    newmat10/newmat9.cpp
    newmat10/newmatex.cpp
    newmat10/newmatnl.cpp
    newmat10/newmatrm.cpp
    newmat10/solution.cpp
    newmat10/sort.cpp
    newmat10/submat.cpp
    newmat10/svd.cpp

    auxiliary/betadev.cpp
    auxiliary/bnldev.cpp
    auxiliary/gammln.cpp
    auxiliary/gasdev.cpp
    auxiliary/ran1.cpp

    modules/module_energy_sfc.cpp
    modules/module_climate_sfc.cpp
    modules/module_macro_sfc.cpp
    modules/module_finance_sfc.cpp

    dsk_sfc_main.cpp
)

# Executable
add_executable(dsk_SFC ${SOURCE_FILES})

# Include directories
target_include_directories(dsk_SFC PRIVATE
    include
    newmat10
    auxiliary
    modules
    rapidjson
)

# Platform-specific options
if (NOT APPLE)
    target_link_libraries(dsk_SFC -static)
endif()


# cmake_minimum_required(VERSION 3.6)

# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)
# message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
# set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fno-math-errno -msse")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-math-errno -msse")

# project(dsk_SFC)

# set(SOURCE_FILES
#         include/CLI11/CLI11.hpp
#         newmat10/bandmat.cpp
#         newmat10/cholesky.cpp
#         newmat10/controlw.h
#         newmat10/evalue.cpp
#         newmat10/fft.cpp
#         newmat10/hholder.cpp
#         newmat10/jacobi.cpp
#         newmat10/myexcept.cpp
#         newmat10/newfft.cpp
#         newmat10/newmat1.cpp
#         newmat10/newmat2.cpp
#         newmat10/newmat3.cpp
#         newmat10/newmat4.cpp
#         newmat10/newmat5.cpp
#         newmat10/newmat6.cpp
#         newmat10/newmat7.cpp
#         newmat10/newmat8.cpp
#         newmat10/newmat9.cpp
#         newmat10/newmatap.h
#         newmat10/newmatex.cpp
#         newmat10/newmatnl.cpp
#         newmat10/newmatnl.h
#         newmat10/newmatrc.h
#         newmat10/newmatrm.cpp
#         newmat10/newmatrm.h
#         newmat10/precisio.h
#         newmat10/solution.cpp
#         newmat10/solution.h
#         newmat10/sort.cpp
#         newmat10/submat.cpp
#         newmat10/svd.cpp
#         newmat10/newmatio.h
#         newmat10/newmat.h
#         newmat10/include.h
		
#         auxiliary/betadev.cpp
#         auxiliary/betadev.h
#         auxiliary/bnldev.cpp
#         auxiliary/bnldev.h
#         auxiliary/gammln.cpp
#         auxiliary/gammln.h
#         auxiliary/gasdev.cpp
#         auxiliary/gasdev.h
#         auxiliary/boolean.h
#         auxiliary/myexcept.h
#         auxiliary/ran1.cpp
#         auxiliary/ran1.h

#         dsk_sfc_main.cpp
#         dsk_sfc_functions.h
#         dsk_sfc_globalvars.h
# 	dsk_sfc_flags.h
#         dsk_sfc_include.h
# 	dsk_sfc_inits.h
# 	dsk_sfc_parameters.h
#         modules/module_energy_sfc.cpp
#         modules/module_energy_sfc.h
#         modules/module_climate_sfc.cpp
#         modules/module_climate_sfc.h
#         modules/module_macro_sfc.cpp
#         modules/module_macro_sfc.h
# 	modules/module_finance_sfc.cpp
# 	modules/module_finance_sfc.h
#     )

# add_executable(dsk_SFC ${SOURCE_FILES})
# if (NOT APPLE)
# 	target_link_libraries(dsk_SFC -static)
# endif(NOT APPLE)