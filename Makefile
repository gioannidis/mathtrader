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

#===============================#
#	Directories		#
#===============================#

INCDIR   = ./include
SRCDIR   = ./src
OBJDIR   = ./build
CFGDIR   = ./cfg
DOXYDIR  = ./doc


#===============================#
#	Files			#
#===============================#

SOURCES    = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS    = $(addprefix $(OBJDIR)/,$(notdir $(SOURCES:.cpp=.o)))
EXECUTABLE = mathtrader++

DOXYBIN	 = doxygen
DOXYFILE = $(CFGDIR)/doxyfile.cfg


#===============================#
#	Compiler/Linker Flags	#
#===============================#

CXX      = g++
DBGFLAGS = -g
OPTIM    = -O3
INCLUDE  = -I $(INCDIR)
CXXFLAGS = $(DEBUG) $(OPTIM) $(INCLUDE) -Wall -Wextra -std=c++11
LDFLAGS  = -lemon


#===============================#
#	Command aliases		#
#===============================#

ECHO   = echo
RM     = rm
MKDIR  = mkdir -p


#===============================#
#	Main Receipes		#
#===============================#

###
# Default receipe
###

all: directories $(EXECUTABLE)

###
# Debug receipe
###

debug: CXXFLAGS := $(filter-out $(OPTIM), $(CXXFLAGS))
debug: CXXFLAGS += -O0
debug: CXXFLAGS += $(DBGFLAGS)
debug: all


#===============================#
#	Directories Receipes	#
#===============================#

.PHONY: directories
directories: $(OBJDIR)

$(OBJDIR):
	$(MKDIR) $(OBJDIR)


#===============================#
#	Executable Receipe	#
#===============================#

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $^ $(LDFLAGS) -o $@


#===============================#
#	Dependencies Receipies	#
#===============================#

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@


#===============================#
#	    Documentation	#
#===============================#

.PHONY: html
html:
	$(DOXYBIN) $(DOXYFILE)


#===============================#
#	    Cleaning		#
#===============================#

.PHONY: clean
clean:
	-$(RM) -f $(OBJECTS) $(EXECUTABLE)

.PHONY: purge
purge: clean
	-$(RM) -rf $(DOXYDIR) $(OBJDIR)
