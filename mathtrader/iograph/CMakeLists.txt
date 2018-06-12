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

project(SolverProject LANGUAGES CXX)

# The library name
set(LIBNAME iograph)
set(LIB_CONFIG_FILENAME IoGraphLibConfig)

# Get the library sources.
set(SOURCES
	src/baseparser.cpp
	src/resultparser.cpp
	src/wantparser.cpp
)

###
### OTHER VARIABLE DEFINITIONS
### Do not edit below this point!
###

# Define the new library and add the sources.
add_library(${LIBNAME}
	STATIC
	${SOURCES}
)

# Define headers for this library. PUBLIC headers are used for
# compiling the library, and will be added to consumers' build
# paths.
target_include_directories(${LIBNAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
	PRIVATE src
)

# 'make install' to the correct locations (provided by GNUInstallDirs).
install(TARGETS ${LIBNAME} EXPORT ${LIB_CONFIG_FILENAME}
	ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR} # Windows
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Make the project importable from the install directory.
# Put config file in per-project dir (name MUST match).
# Alternatively, the config file may also just go into 'cmake'.
install(EXPORT ${LIB_CONFIG_FILENAME} DESTINATION share/${PROJECT_NAME}/cmake)

# This makes the project importable from the build directory.
export(TARGETS ${LIBNAME} FILE ${LIB_CONFIG_FILENAME}.cmake)


##############################
#	TESTING
##############################
