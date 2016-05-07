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
#include <map>
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

	/**
	 * @brief Print LGF file.
	 * Prints the nodes & arcs in a lemon-LGF format.
	 * @param os The output stream (default: std::cout).
	 * @return *this
	 */
	const WantParser & print( std::ostream & os = std::cout ) const ;

	/**
	 * @brief Print LGF file.
	 * Prints the nodes & arcs in a lemon-LGF format.
	 * @param fn The file output name.
	 * @return *this
	 */
	const WantParser & print( const std::string & fn ) const ;

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
		bool unknown;		/**< Unknown item; node is missing */

		inline _Arc_s ( const std::string & _source,
				const std::string & _target,
				int _rank ) :
			item_s( _source ),
			item_t( _target ),
			rank( _rank ),
			unknown( false ){}

	} _Arc_t;

	/**
	 * Node & Arc maps; the key is the item reference name,
	 * e.g., 0042-PUERTO
	 * The Arc Map maps to a vector of arcs.
	 */
	std::map< std::string , _Node_t > _node_map;
	std::map< std::string ,
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
	 * @brief Mark unknown items.
	 * Parses all the arcs and checks
	 * whether any target nodes are missing (unknown).
	 * These won't be appended to the results.
	 */
	WantParser & _markUnknownItems();

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

	/**
	 * @brief Check if item is dummy.
	 * Parses the given item and checks if it's dummy (begins with '%').
	 * @param item The item to be checked.
	 * @return true if it's dummy, false otherwise.
	 */
	static bool _dummy( const std::string & item );

	/**
	 * @brief Append username if dummy.
	 * Appends the username to the item name, if it's dummy.
	 * @param item The item to be parsed.
	 * @return Item name with appended username if needed, enclosed in quotation marks.
	 */
	static std::string _appendIfDummy( const std::string & item,
			const std::string & username );


	/**
	 * @brief Append quotation marks.
	 * Appends quotation marks to str, if not there.
	 * Does not append a second one.
	 * @str Input string.
	 * @return String with quotation marks.
	 */
	static std::string _quotationMarks( const std::string & str );
};

#endif /* _WANTPARSER_HPP_ */
