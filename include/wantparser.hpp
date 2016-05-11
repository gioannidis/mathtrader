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
#include <list>
#include <map>
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
	 * @brief Print LGF file.
	 * Prints the nodes & arcs in a lemon-LGF format.
	 * @param os The output stream (default: std::cout).
	 * @return *this
	 */
	const WantParser & print( std::ostream & os = std::cout ) const ;

	/**
	 * @brief Print LGF file to output file.
	 * Prints the nodes & arcs in a lemon-LGF format.
	 * @param fn The file output name.
	 * @return *this
	 */
	const WantParser & print( const std::string & fn ) const ;


	/**
	 * @brief Show options.
	 * Prints the options of WantParser.
	 * @param os The output stream (default: std::cout).
	 * @return *this
	 */
	const WantParser & showOptions( std::ostream & os = std::cout ) const ;

	/**
	 * @brief Show errors.
	 * Prints any logged errors that were encountered
	 * during parse().
	 * @param os The output stream (default: std::cout).
	 * @return *this;
	 */
	const WantParser & showErrors( std::ostream & os = std::cout ) const ;

	/**
	 * @brief Get priority scheme.
	 * Gets the given priority scheme, in the form of
	 * 	"PRIORITY-XXX"
	 * Note that the priority scheme might be invalid
	 * or not implemented yet;
	 * it's not the responsibility of WantParser to check
	 * its validity.
	 * @return std::string with the priority scheme.
	 */
	std::string getPriorityScheme() const ;

private:
	/***************************//*
	 * 	OPTIONS
	 *****************************/

	/**
	 * 1. Boolean options.
	 * 2. Integer options.
	 * 3. Priority option.
	 */

	/**
	 * Boolean & integer options enum
	 */
	enum BoolOption {	/**< Bool options enum */
		ALLOW_DUMMIES,
		REQUIRE_COLONS,
		REQUIRE_USERNAMES,
		HIDE_LOOPS,
		HIDE_SUMMARY,
		HIDE_NONTRADES,
		HIDE_ERRORS,
		HIDE_REPEATS,
		HIDE_STATS,
		SORT_BY_ITEM,
		CASE_SENSITIVE,
		SHOW_MISSING,
		SHOW_ELAPSED_TIME,
		MAX_BOOL_OPTIONS	/**< Not an option */
	};
	enum IntOption {	/**< Int options enum */
		SMALL_STEP,
		BIG_STEP,
		NONTRADE_COST,
		MAX_INT_OPTIONS		/**< Not an option */
	};


	/**
	 * Static members: unordered_maps
	 * to map string options -> enums.
	 */
	static const std::unordered_map< std::string, BoolOption  >
		_bool_option_map;
	static const std::unordered_map< std::string, IntOption >
		_int_option_map;

	/**
	 * The actual vectors members
	 * with all options.
	 */
	std::vector< bool > _bool_options;
	std::vector< int > _int_options;

	/**
	 * The priority scheme
	 */
	std::string _priority_scheme;

	/**
	 * Enum of current status
	 */
	enum Status {
		PARSE_WANTS,
		PARSE_NAMES,
		UNKNOWN,
	};
	Status _status;


	/***************************//*
	 * INTERNAL DATA STRUCTURES
	 *****************************/

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
	 * Unknown items map:
	 */
	std::unordered_map< std::string, int > _unknown_item_map;

	/**
	 * Errors list.
	 */
	std::list< std::string > _errors;

	/**
	 * @brief File Stream
	 * File stream to read the want list from,
	 * when applicable.
	 */
	std::ifstream _fs;


	/********************************//*
	 *  UTILITY NON_STATIC FUNCTIONS
	 ***********************************/

	/**
	 * @brief Parse option
	 */
	WantParser & _parseOption( const std::string & option );

	/**
	 * @brief Parse official name
	 * Parses lines giving the official names of nodes.
	 * @param line line to be parsed
	 * @return *this
	 */
	WantParser & _parseOfficialName( const std::string & line );

	/**
	 * @brief Parse want list
	 * Parses lines giving the want lists.
	 * @return *this
	 */
	WantParser & _parseWantList( const std::string & line );

	/**
	 * @brief Parse Item Name.
	 * Checks whether the item is dummy, appends username if needed
	 * and finally appends quotation marks.
	 * Converts to uppercase unless items are case sensitive.
	 * Raises an exception if a dummy is found but dummies are not allowed
	 * or the username is undefined.
	 * @param item item name to be parsed.
	 * @param username username of item's owner (default: empty)
	 * @return *this
	 */
	WantParser & _parseItemName( std::string & item,
			const std::string username = "" );

	/**
	 * @brief Parse username.
	 * Appends quotation marks and converts to uppercase.
	 * Usernames on BGG are always case-insensitive.
	 * @param username username to be parsed.
	 * @return *this
	 */
	WantParser & _parseUsername( std::string & username );

	/**
	 * @brief Mark unknown items.
	 * Parses all the arcs and checks
	 * whether any target nodes are missing (unknown).
	 * These won't be appended to the results.
	 * @return *this
	 */
	WantParser & _markUnknownItems();


	/********************************//*
	 *  UTILITY STATIC FUNCTIONS
	 ***********************************/

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
	 * @brief Append quotation marks.
	 * Appends quotation marks to string,
	 * except if there are opening/closing quotation marks.
	 * @str Input string to modify.
	 */
	static void _quotationMarks( std::string & str );

	/**
	 * @brief Convert to uppercase.
	 * Converts the given string to uppercase.
	 */
	static void _toUpper( std::string & str );


	/***************************//*
	 * UTILITY STATIC MEMBERS
	 *****************************/

	/**
	 * Regex to define fields.
	 * The regular expression defines what the fields **are**,
	 * therefore it's not a field separator.
	 * It's used like FPAT from gawk.
	 */
	static const std::regex _FPAT;
};

#endif /* _WANTPARSER_HPP_ */
