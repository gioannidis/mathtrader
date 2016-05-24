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

#include "baseparser.hpp"

#include <map>
#include <unordered_map>
#include <vector>

class WantParser : public BaseParser {

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
	 * @brief Print LGF file.
	 * Prints the nodes & arcs in a lemon-LGF format.
	 * @param os The output stream (default: std::cout).
	 * @return *this
	 */
	using BaseParser::print;	/**< Consider base class overloads */
	const WantParser & print( std::ostream & os = std::cout ) const ;

	/**
	 * @brief Show options.
	 * Prints the options of WantParser.
	 * @param os The output stream (default: std::cout).
	 * @return *this
	 */
	const WantParser & showOptions( std::ostream & os = std::cout ) const ;

	/**
	 * @brief Get priority scheme.
	 * Gets the given priority scheme, in the form of
	 * 	"PRIORITY-XXX"
	 * Note that the priority scheme might be invalid
	 * or not implemented yet;
	 * it's not the responsibility of WantParser to check
	 * its validity.
	 * @return std::string with the priority scheme.
	 * @return Null string if no priority has been given.
	 */
	std::string getPriorityScheme() const ;

	/**
	 * @brief Show "HIDE-ERRORS".
	 * Shows if wantparser should hide errors.
	 * @return true if HIDE-ERRORS has been given.
	 */
	bool hideErrors() const ;

	/**
	 * @brief Show "HIDE-LOOPS".
	 * Shows if mathtrader should hide trade loops.
	 * @return true if HIDE-LOOPS has been given.
	 */
	bool hideLoops() const ;

	/**
	 * @brief Show "HIDE-NONTRADES".
	 * Shows if mathtrader should hide non-trades.
	 * @return true if HIDE-NONTRADES has been given.
	 */
	bool hideNonTrades() const ;

	/**
	 * @brief Show "HIDE-STATS".
	 * Shows if mathtrader should hide result statistics
	 * (other than number of trades).
	 * @return true if HIDE-NONTRADES has been given.
	 */
	bool hideStats() const ;

	/**
	 * @brief Show "HIDE-SUMMARY".
	 * Shows if mathtrader should hide the item summary.
	 * @return true if HIDE-NONTRADES has been given.
	 */
	bool hideSummary() const ;

	/**
	 * @brief Show "SHOW-ELAPSED-TIME".
	 * Shows if the elapsed real time should be appended
	 * to the results.
	 * @return true if SHOW-ELAPSED-TIME has been given.
	 */
	bool showElapsedTime() const ;

	/**
	 * @brief Show "SORT-BY-ITEM".
	 * Shows if mathtrader should sort the item summary
	 * by item, instead of by username.
	 * If hideSummary() is false, this option has no effect.
	 * @return true if SORT-BY-ITEM has been given.
	 */
	bool sortByItem() const ;

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
		/* Internal options */
		ALLOW_DUMMIES,
		CASE_SENSITIVE,
		HIDE_REPEATS,
		REQUIRE_COLONS,
		REQUIRE_USERNAMES,
		SHOW_ELAPSED_TIME,
		SHOW_MISSING,
		/* External options */
		HIDE_ERRORS,
		HIDE_LOOPS,
		HIDE_NONTRADES,
		HIDE_STATS,
		HIDE_SUMMARY,
		SORT_BY_ITEM,
		/* Not implemented */
		MAX_BOOL_OPTIONS	/**< Not an option; always LAST */
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
		BEGIN,			/**< options may be given */
		PARSE_NAMES,		/**< parsing official names */
		PARSE_WANTS_NONAMES,	/**< parsing wants; no given official names */
		PARSE_WANTS_WITHNAMES,	/**< parsing wants; official names given */
	};
	Status _status;

	/**
	 * Indicates whether official names have been given.
	 */
	bool _official_given;


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
		bool has_wantlist;		/**< Wantlist has been given */

		inline _Node_s ( const std::string & _item,
				const std::string & _official,
				const std::string & _user,
				bool _dummy = false,
				bool _has_wantlist = false
				) :
			item( _item ),
			official_name( _official ),
			username( _user ),
			dummy( _dummy ),
			has_wantlist( _has_wantlist ) {}

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


	/********************************//*
	 *  UTILITY NON_STATIC FUNCTIONS
	 ***********************************/

	/**
	 * @brief Parse want file line.
	 * Parses a want list file line:
	 * options, official names and want lists.
	 */
	void _parse( const std::string & line );

	/**
	 * @brief Post Parsing
	 * Marks unknown items.
	 * @return *this
	 */
	WantParser & _postParse();

	/**
	 * @brief Parse option
	 * @return *this
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
	 * If any errors are detected the whole line is discarded!
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
	 * @brief Check if item is dummy.
	 * Parses the given item and checks if it's dummy (begins with '%').
	 * @param item The item to be checked.
	 * @return true if it's dummy, false otherwise.
	 */
	static bool _dummy( const std::string & item );
};

#endif /* _WANTPARSER_HPP_ */
