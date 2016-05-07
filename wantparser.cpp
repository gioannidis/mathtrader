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

#include <regex>
#include <stdexcept>


/************************************//*
 * 	PUBLIC METHODS - CONSTRUCTORS
 **************************************/

WantParser::WantParser()
{
}

WantParser::~WantParser() {

	/**
	 * Close file stream, if applicable.
	 */
	if ( _fs.is_open() ) {
		_fs.close();
	}
}


/************************************//*
 * 	PUBLIC METHODS - INPUT
 **************************************/
	
WantParser &
WantParser::setInputFile( const std::string & fn ) {

	/**
	 * Open file for reading
	 */
	_fs.open(fn, std::ios_base::in);
	if ( _fs.fail() ) {
		throw std::runtime_error("Could not open file "
				+ fn + " for reading");
	}
	return *this;
}


/************************************//*
 * 	PUBLIC METHODS - OUTPUT
 **************************************/

void
WantParser::parse() {

	/**
	 * Read buffer
	 */
	const size_t BUFSIZE = (1<<10);
	std::string buffer;
	buffer.reserve(BUFSIZE);

	/**
	 * Input stream:
	 * Given file, if applicable.
	 * Else, std::cin.
	 */
	std::istream & is = (_fs.is_open()) ? _fs : std::cin;

	//std::regex e("([^[:blank:]]+)|(\"[^\"]+\")|(\\([^\\)]+\\))");
	std::regex e("(\\s+)");

	while (std::getline( is, buffer )) {

		/**
		 * Ignore comments
		 */
		if ( buffer.compare(0, 2, "#!") == 0 ) {

			/**
			 * Option line
			 */

			continue ;

		} else if ( buffer.compare(0, 1, "#") == 0 ) {

			/**
			 * Comment line
			 */
			continue ;

		} else if ( buffer.compare(0, 1, "!") == 0 ) {

			if ( buffer.compare(0, 21, "!BEGIN-OFFICIAL-NAMES") == 0 ) {

				continue ;

			} else if ( buffer.compare(0, 19, "!END-OFFICIAL-NAMES") == 0 ) {\

				break ;

			} else {
				throw std::runtime_error("Unrecognized directive: "
						+ buffer);
			}
			continue ;
		}

		std::cout << buffer << std::endl;

#if 1
		auto match = _split( buffer, e );
		for ( auto const & x : match ) {
			std::cout << x << std::endl;
		}
#endif

	}
}


std::vector<std::string>
WantParser::_split( const std::string & input, const std::string & regex ) {

	// passing -1 as the submatch index parameter performs splitting
	std::regex re(regex);
	return _split( input, re );
}

std::vector<std::string>
WantParser::_split( const std::string & input, const std::regex & regex ) {

	std::sregex_token_iterator
		first{input.begin(), input.end(), regex, -1},
		last;

	return {first, last};
}
