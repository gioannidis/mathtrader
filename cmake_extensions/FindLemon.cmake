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

# This module tries to find an installed Lemon library.
#
# It sets the following variables:
#  LEMON_FOUND		- 'true' if Lemon library is found; 'false' or 'undefined' otherwise.
#  LEMON_INCLUDE_DIR	- The Lemon include directories.
#  LEMON_LIBRARIES	- The libraries needed to use Lemon.
#  LEMON_DEFINITIONS	- Compiler switches required for using Lemon.

# Find the Lemon include path.
FIND_PATH(
	LEMON_INCLUDE_DIR lemon/core.h
	PATHS
		/usr/include
		/usr/local/include
		CPLUS_INCLUDE_PATH
		ENV
		${CMAKE_INCLUDE_PATH}
		${CMAKE_PREFIX_PATH}/include
		$ENV{LEMON_ROOT}/include
		"C:/Program Files/LEMON/include"
	DOC "The Lemon include directory"
)

# Find the Lemon library.
FIND_LIBRARY(
	LEMON_LIBRARIES
	NAMES emon lemon
	PATHS
		LD_LIBRARY_PATH
		LIBRARY_PATH
		ENV
		$ENV{LEMON_ROOT}/lib
		$ENV{LEMON_ROOT}/src/impex
		"C:/Program Files/LEMON/lib"
	DOC "The Lemon library"
)

# Isolate the full path of the lemon library,
# without the file name.
GET_FILENAME_COMPONENT(LEMON_LIBRARY_PATH ${LEMON_LIBRARIES} DIRECTORY)

# Put the library directory in a variable which is put in the CACHE.
# Type: Directory chooser dialog ('PATH')
SET(LEMON_LIBRARY_DIR ${LEMON_LIBRARY_PATH} CACHE PATH "Path to lemon library.")

# Handle the QUIETLY and REQUIRED arguments and set LOGGING_FOUND to TRUE
# if all listed variables are TRUE.
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LEMON DEFAULT_MSG LEMON_LIBRARIES LEMON_INCLUDE_DIR)

# Tell cmake GUIs to ignore the "local" variables.
MARK_AS_ADVANCED(LEMON_INCLUDE_DIR LEMON_LIBRARIES LEMON_LIBRARY_DIR)
