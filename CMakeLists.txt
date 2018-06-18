#  This file is part of MathTrader++.
#
#  Copyright (C) 2018 George Ioannidis
#
#  MathTrader++ is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  MathTrader++ is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with MathTrader++.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.0.2)


########################################
### GENERAL PARAMETERS
########################################

# Name of the project.
project(MathTraderProject LANGUAGES CXX)

# The version number.
# Copyright year
set(MathTraderProject_VERSION_MAJOR 1)
set(MathTraderProject_VERSION_MINOR 3beta)
set(MathTraderProject_YEAR 2018)


########################################
### COMPILE & LINK FLAGS
########################################

# Project-Wide compile flags.
add_compile_options(-std=c++14 -Wall -Wextra -Wpedantic -pedantic -O3 -g)
set(CMAKE_CXX_STANDARD 14)	# since CMake 3.1

# Project-Wide link flags.
#set(GCC_COVERAGE_LINK_FLAGS "")
#set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")


########################################
### REQUIRED PACKAGES
########################################

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

# Add cmake extensions
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake_extensions)

# Look for the Lemon package.
find_package(Lemon REQUIRED)

# Include external headers as system includes.
# Adding them as "SYSTEM" includes suppresses their warnings.
# NOTE: this might not be required to build the project and has risen compilation errors in the past.
include_directories(SYSTEM ${LEMON_INCLUDE_DIR})

# Add the lemon libraries during the link stage in all targets.
link_libraries(${LEMON_LIBRARIES})


########################################
### SUB-DIRECTORIES
########################################

add_subdirectory(app)
add_subdirectory(lib)

########################################
### DOCUMENTATION
########################################

# Indicate the documentation build as an option.
# Set it to ON by default.
option(BUILD_DOC "Build documentation" ON)

# check if Doxygen is installed
find_package(Doxygen)
if (DOXYGEN_FOUND)

	# Set input and output files.
	set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
	set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

	# Request to configure the input file.
	configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
	message("-- Doxygen build started")

	# note the option ALL which allows to build the docs together with the application
	# If wishing to build the documentation together with the application
	# add the option 'ALL'.
	# Example:
	#	add_custom_target( doc ALL ... )
	add_custom_target(doc
		COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen"
		VERBATIM
	)
else (DOXYGEN_FOUND)
	message("-- Doxygen needed to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)
