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

project(AppProject LANGUAGES CXX)

# Define the executable(s).
add_executable(mathtrader++
	mathtrader.cpp
)
add_executable(routechecker
	routechecker.cpp
)

# Define the libraries the executables depend upon.
target_link_libraries(mathtrader++
	iograph
	solver
)
target_link_libraries(routechecker
	iograph
	solver
)

# Configure header files to pass some of the CMake settings
# to the source code.
set(BINARY_INCLUDE_CONFIG_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
configure_file (
	"${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in"
	"${BINARY_INCLUDE_CONFIG_DIR}/config.hpp"
)

# Add subdirectory to the search path for include files
# so that we will find the configured header files.
target_include_directories(mathtrader++
	PRIVATE
	${BINARY_INCLUDE_CONFIG_DIR}
)
