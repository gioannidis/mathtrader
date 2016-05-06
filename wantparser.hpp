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
#ifndef _WANTPARSER_HPP_
#define _WANTPARSER_HPP_

#include <iostream>

class WantParser {

public:
	/**
	 * @brief Constructor.
	 * Details.
	 */
	WantParser();

	/**
	 * @brief Destructor.
	 * Details.
	 */
	~WantParser();

	/**
	 * @brief Parse want list from istream.
	 * Parses a want list
	 * from the given input stream.
	 * @param is input stream (default: stdin)
	 * @return *this
	 */
	WantParser & wantListReader( std::istream & is = std::cin );

	/**
	 * @brief Parse want list graph from file.
	 * Parses a want list
	 * from the given file.
	 * @param fn file name
	 * @return *this
	 */
	WantParser & wantListReader( const std::string & fn );

	/**
	 * @brief Parse want list graph from file.
	 * Parses a want list
	 * from the given file.
	 * @param fn file name
	 * @return *this
	 */
	WantParser & wantListReader( const char * fn );

	/**
	 */
	void run();

private:

};

#endif /* _WANTPARSER_HPP_ */
