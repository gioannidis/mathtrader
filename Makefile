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

SRCSUB = $(shell find $(SRCDIR) -type d)


#===============================#
#	Files			#
#===============================#

COMMONSOURCES = $(wildcard $(SRCDIR)/common/*.cpp)

###
# MathTrader++ related files
###
MATHSOURCES = $(COMMONSOURCES) $(wildcard $(SRCDIR)/mathtrader/*.cpp)
MATHOBJECTS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(MATHSOURCES))

###
# RouteCheck related files
###
ROUTESOURCES = $(COMMONSOURCES) $(wildcard $(SRCDIR)/routecheck/*.cpp)
ROUTEOBJECTS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(ROUTESOURCES))

###
# Executables
###
OBJECTS	= $(MATHOBJECTS) $(ROUTEOBJECTS)
EXECUTABLES = mathtrader++ routechecker

###
# Various
###
DOXYBIN	 = doxygen
DOXYFILE = $(CFGDIR)/doxyfile.cfg


#===============================#
#	Compiler/Linker Flags	#
#===============================#

CXX      = g++
DBGFLAGS = -g
OPTIM    = -O3
INCLUDE  = -I $(INCDIR)
CXXFLAGS = $(OPTIM) $(INCLUDE) -Wall -Wextra -std=c++11 -Wno-strict-aliasing
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

all: buildrepo $(EXECUTABLES)

###
# Debug receipe
###

debug: CXXFLAGS := $(filter-out $(OPTIM), $(CXXFLAGS))
debug: CXXFLAGS += -O0
debug: CXXFLAGS += $(DBGFLAGS)
debug: all


#===============================#
#	Executable Receipes	#
#===============================#

mathtrader++: $(MATHOBJECTS)
	$(CXX) $^ $(LDFLAGS) -o $@

routechecker: $(ROUTEOBJECTS)
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
	-$(RM) -f $(OBJECTS) $(EXECUTABLES)

.PHONY: purge
purge: clean
	-$(RM) -rf $(DOXYDIR) $(OBJDIR)


#=======================================#
#          Object Repository            #
#=======================================#

# Replicate source directories under $(BLDIR)

.PHONY: buildrepo
buildrepo:
	@$(call make-repo)

define make-repo
        for folder in $(SRCSUB); \
        do \
                $(MKDIR) $(OBJDIR)/$${folder#$(SRCDIR)}; \
        done
endef
