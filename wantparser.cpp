/* This file is part of MathTrader++, a C++ utility
 * for finding, on a directed graph whose arcs have costs,
 * a set of vertex-disjoint cycles that maximizes the number
 * of covered vertices as a first priority
 * and minimizes the total cost as a second priority.
 *
 * Copyright (C) 2016 George Ioannidis
 *
 * MathTrader++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MathTrader++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MathTrader++.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "wantparser.hpp"

#include <fstream>


/************************************//*
 * 	PUBLIC METHODS - CONSTRUCTORS
 **************************************/

WantParser::WantParser() {
}

WantParser::~WantParser() {
}


/************************************//*
 * 	PUBLIC METHODS - INPUT
 **************************************/
	
WantParser &
WantParser::wantListReader( std::istream & is ) {

	return *this;
}

WantParser &
WantParser::wantListReader( const std::string & fn ) {
	return wantListReader( fn.c_str() );
}

WantParser &
WantParser::wantListReader( const char * fn ) {

	std::filebuf fb;
	fb.open(fn, std::ios::in);
	std::istream is(&fb);
	wantListReader(is);
	fb.close();
	return *this;
}


/************************************//*
 * 	PUBLIC METHODS - OUTPUT
 **************************************/

