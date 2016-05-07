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

	/**
	 * @brief Print Arcs.
	 * Prints the arcs in a lemon-LGF format.
	 * @param os The output stream (default: std::cout).
	 * @return *this
	 */
	const WantParser & printArcs( std::ostream & os = std::cout ) const ;

	/**
	 * @brief Clear the parser.
	 * Clears all data and resets variables.
	 */
	WantParser & clear();

private:
	/**
	 * Enum of current status
	 */
	enum Status {
		PARSE_WANTS,
		PARSE_NAMES,
		UNKNOWN,
	};
	Status _status;

	/**
	 * @brief Node struct
	 * Structure to hold node information
	 */
	typedef struct _Node_s {

		std::string item;		/**< Item name; 0001-PUERTO */
		std::string official_name;	/**< Official name; "Puerto Rico" */
		std::string username;		/**< Username; "Aldie" */
		bool dummy;			/**< Dummy node */

		inline _Node_s ( const std::string & _item,
				const std::string & _official,
				const std::string & _user,
				bool _dummy = false
				) :
			item( _item ),
			official_name( _official ),
			username( _user ),
			dummy( _dummy ) {}

	} _Node_t;

	/**
	 * @brief Arc struct
	 * Structure to hold arc information
	 */
	typedef struct _Arc_s {

		std::string item_s;	/**< Item name; source */
		std::string item_t;	/**< Item name; target */
		int rank;		/**< Rank of arc */

		inline _Arc_s ( const std::string & _source,
				const std::string & _target,
				int _rank ) :
			item_s( _source ),
			item_t( _target ),
			rank( _rank ) {}

	} _Arc_t;

	/**
	 * Node & Arc maps; the key is the item reference name,
	 * e.g., 0042-PUERTO
	 * The Arc Map maps to a vector of arcs.
	 */
	std::unordered_map< std::string , _Node_t > _node_map;
	std::unordered_map< std::string ,
		std::vector< _Arc_t > >  _arc_map;

	/**
	 * @brief File Stream
	 * File stream to read the want list from,
	 * when applicable.
	 */
	std::ifstream _fs;

	/**
	 * @brief Parse official name
	 * Parses lines giving the official names of nodes.
	 * @param line line to be parsed
	 */
	WantParser & _parseOfficialName( const std::string & line );

	/**
	 * @brief Parse want list
	 * Parses lines giving the want lists.
	 */
	WantParser & _parseWantList( const std::string & line );

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
			const std::regex & regex = _FPAT );

};

#endif /* _WANTPARSER_HPP_ */
