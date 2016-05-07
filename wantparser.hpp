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
#include <fstream>
#include <regex>
#include <string>
#include <vector>

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
	 * @brief Set wantlist input file.
	 * Sets the input file where the wantlist
	 * will be read from.
	 * If not called, std::cin will be used, instead.
	 * @param fn file name
	 * @return *this
	 */
	WantParser & wantlist( const std::string & fn );

	/**
	 * @brief Parse the wantlist.
	 * Parses the wantlist from either
	 * a previously given input file
	 * or from std::cin.
	 */
	void parse();

private:
	/**
	 * @brief File Stream
	 * File stream to read the want list from,
	 * when applicable.
	 */
	std::ifstream _fs;

	/**
	 * @brief Parse official name
	 * @param line line to be parsed
	 */
	WantParser & _parseOfficialName( const std::string & line );

	/**
	 * @brief Split string.
	 */
	static std::vector<std::string> _split( const std::string & input,
			const std::string & regex);
	static std::vector<std::string> _split( const std::string & input,
			const std::regex & regex);

};

#endif /* _WANTPARSER_HPP_ */
