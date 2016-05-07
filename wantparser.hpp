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
#include <unordered_map>
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

	/**
	 * @brief Print Nodes.
	 * Prints the nodes in a lemon-LGF format.
	 * @param os The output stream (default: std::cout).
	 * @return *this
	 */
	const WantParser & printNodes( std::ostream & os = std::cout ) const ;

private:
	/**
	 * Enum of current status
	 */
	enum Status {
		PARSE_ARCS,
		PARSE_NAMES,
		UNKNOWN,
	};
	Status _status;

	/**
	 * Maps; the key is the item reference name,
	 * e.g., 0042-PUERTO
	 */
	std::unordered_map< std::string , std::string >
		_official_name, /**< Official item name */
		_username;	/**< Owner username */

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
	 * Regex to define fields.
	 * The regular expression defines what the fields **are**,
	 * therefore it's not a field separator.
	 * It's used like FPAT from gawk.
	 */
	static const std::regex _FPAT;

	/**
	 * @brief Split string.
	 * Splits the string based on a regular expression.
	 * @param input The input string.
	 * @param regex Regular expression defining the fields.
	 * @return Vector with matches.
	 */
	static std::vector<std::string> _split( const std::string & input,
			const std::string & regex);
	static std::vector<std::string> _split( const std::string & input,
			const std::regex & regex);

};

#endif /* _WANTPARSER_HPP_ */
