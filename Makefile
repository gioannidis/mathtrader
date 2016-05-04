# This file is part of MathTrader++, a C++ utility
# for finding, on a directed graph whose arcs have costs,
# a set of vertex-disjoint cycles that maximizes the number
# of covered vertices as a first priority
# and minimizes the total cost as a second priority.
#
# Copyright (C) 2016 George Ioannidis
#
# MathTrader++ is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# MathTrader++ is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with MathTrader++.  If not, see <http://www.gnu.org/licenses/>.

CXX	 = g++
DEBUG    = #-g
OPTIM	 = -O3
INCLUDE  =
CXXFLAGS = $(DEBUG) $(OPTIM) $(INCLUDE) -Wall -Wextra -std=c++11
LDFLAGS  = -lemon

ECHO	= echo
RM	= rm

SOURCES=main.cpp mathtrader.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=mathtrader++

DOXYGEN	 = doxygen
DOXYFILE = doxyfile.cfg
DOXYDIR  = doc

all: $(EXECUTABLE)


#===============================#
#	Build Executable	#
#===============================#

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $^ $(LDFLAGS) -o $@


#===============================#
#	Build Dependencies	#
#===============================#

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@


#===============================#
#	    Documentation	#
#===============================#

html:
	$(DOXYGEN) $(DOXYFILE)

#===============================#
#	    Cleaning		#
#===============================#

.PHONY: clean
clean:
	-$(RM) -f $(OBJECTS) $(EXECUTABLE)

.PHONY: purge
purge: clean
	-$(RM) -rf $(DOXYDIR)
