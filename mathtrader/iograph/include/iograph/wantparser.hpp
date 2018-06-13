/* This file is part of MathTrader++.
 *
 * Copyright (C) 2018 George Ioannidis
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
#include <list>
#include <map>
#include <unordered_map>
#include <vector>

/*! @brief Convert online want-list file to graph
 *  and parse options.
 *
 *  This class processes want-list files generated by
 *  the [Online Want List Generator (OLWLG)](http://bgg.activityclub.org/olwlg),
 *  developed by Jeff Michaud (aka [JeffyJeff](https://www.boardgamegeek.com/user/jeffyjeff)).
 *  The input file is mapped to a **Lemon Graph Format (LGF)** output file,
 *  which may be subsequently processed by the [LEMON](http://lemon.cs.elte.hu) Graph Library.
 *  The LGF output may be retrieved through @ref print().
 *
 *  ## Want-file structure
 *  The want-list file is expected to be structured as follows:
 *
 *  1. Options
 *  2. Item official names
 *  3. Item want-lists
 *
 *  Any line beginning with ``#`` is treated as a comment and will not be parsed.
 *  Exception are directives, beginning with ``#!``.
 *
 *  ### Options
 *
 *  Options begin with ``#!`` and consist of a single-word instruction,
 *  possibly with a value.
 *  Example:
 *
 *  	#! ALLOW-DUMMIES
 *  	#! REQUIRE-COLONS
 *  	#! REQUIRE-USERNAMES
 *  	#! HIDE-NONTRADES
 *  	#! SHOW-ELAPSED-TIME
 *  	#! ITERATIONS=100
 *  	#! SEED=1234567
 *  	#! METRIC=USERS-TRADING
 *
 *  ### Item official names
 *
 *  Official names are optional.
 *  If provided, the directive ``!BEGIN-OFFICIAL-NAMES``
 *  should be prepended at the start
 *  and the directive ``!END-OFFICIAL-NAMES``
 *  should be appended at the end.
 *  Official names bear the format:
 *
 *  	ITEMID ==> "Official Name With Whitespaces" (from USERNAME)
 *
 *  Example:
 *
 *  	!BEGIN-OFFICIAL-NAMES
 *  	# Summary generated: Sat Nov 14 23:05:17 2015
 *  	0001-HYPER ==> "Hyperborea" (from aksosa)
 *  	0002-LOA ==> "Legends of Andor" (from aksosa)
 *  	0003-STUOJ ==> "Sekigahara: The Unification of Japan" (from aksosa)
 *  	0004-DOWFD ==> "Dead of Winter: Felicia Day" (from aksosa)
 *  	0005-ABBEY ==> "Mystery of the Abbey" (from aksosa)
 *  	...
 *  	2720-LASWIL ==> "Last Will" (from DonRab|o)
 *  	!END-OFFICIAL-NAMES
 *
 *  ### Item want-lists
 *
 *  Following are the item want-lists, grouped by username.
 *
 *  	(USERNAME) OFFERED_ITEM_1 : WANTED_ITEM_1 WANTED_ITEM_2 WANTED_ITEM_3
 *  	(USERNAME) OFFERED_ITEM_2 : WANTED_ITEM_1 WANTED_ITEM_3 WANTED_ITEM_4 WANTED_ITEM_5
 *
 *  Remarks:
 *  * Colons ``:`` are optional, unless the ``#!REQUIRE-COLONS`` directive has been given.
 *    In that case, they are required.
 *  * Usernames are optional, unless the ``#!REQUIRE-USERNAMES`` directive has been given.
 *    In that case, they are required.
 *  * Items beginning with ``%`` are dummy items.
 *    These are only allowed if the ``#!ALLOW-DUMMIES`` directive has been specified.
 *  * An item want-list is allowed to be empty, i.e., to not match with any wanted item.
 *
 *  Example:
 *
 *  	#pragma user "540howdy"
 *  	# Confirmation code: NYTjRAcie65D3YL7NUYzSw
 *  	# Tue Nov 17 18:42:15 2015
 *
 *  	(540howdy) %BTB :
 *  	(540howdy) %CoMKL : 0056-COMKL 0557-COMKL 1526-COMKL 1721-COMKL
 *  	(540howdy) %DoWACG : 0906-DOWACG 1090-DOWACG 1648-DOWACG 1932-DOWACG 2154-DOWACG 2241-DOWACG
 *  	(540howdy) %SoN : 0132-SON 0238-SON 1920-SON
 *  	(540howdy) %TCoB :
 *  	(540howdy) 2378-NANZA : %CoMKL %DoWACG 0756-PANDE %SoN 2441-RESAVA 1337-C1YSE
 */
class WantParser {

public:
	WantParser();
	~WantParser() = default;

	/*! @brief Convert want-lists input file to graph.
	 *
	 *  Reads a want-list from the given input stream
	 *  and converts it to a graph.
	 *  Opens the input file and calls @ref parseStream().
	 *
	 *  @param[in]	fn	the input file to read the want-lists from
	 *  @throws	std::runtime_error if file ``fn`` cannot be opened
	 */
	void parseFile( const std::string & fn );

	/*! @brief Convert want-lists input stream to graph.
	 *
	 *  Reads a want-list from the given input stream
	 *  and converts it to a graph.
	 *
	 *  @param[in]	is	the input stream to read the want-lists from
	 */
	void parseStream( std::istream & is );

	/*! @name Output methods
	 *
	 *  Methods to retrieve the generated graph
	 *  and provided options from the want-list file.
	 */
	/*! @{ */ // start of group

	/*! @brief Print LGF output file to file.
	 *
	 *  Prints the graph generated by the want-list input file
	 *  in the Lemon Graph Format (LGF)
	 *  to an output stream.
	 *
	 *  @param[in]	fn	output file to write the LGF file to
	 *  @return	reference to current object
	 *  @throws	std::runtime_error if file ``fn`` cannot be opened
	 */
	const WantParser & print( const std::string & fn ) const ;

	/*! @brief Print LGF output file to stream.
	 *
	 *  Prints the graph generated by the want-list input file
	 *  in the Lemon Graph Format (LGF)
	 *  to an output file.
	 *
	 *  @param	os	output stream to write the LGF file to
	 *  @return	reference to current object
	 */
	const WantParser & print( std::ostream & os = std::cout ) const ;

	/*! @brief Print want-list file options.
	 *
	 *  Prints all options that were provided to the input
	 *  want-list file.
	 *
	 *  @param	os	output stream to write the options to
	 *  @return	reference to current object
	 */
	const WantParser & printOptions( std::ostream & os = std::cout ) const ;

	/*! @brief Print missing items.
	 *
	 *  Prints all non-dummy items without a given want-list.
	 *
	 *  @param	os	output stream to write the missing items to
	 *  @return	reference to current object
	 */
	const WantParser & printMissing( std::ostream & os = std::cout ) const ;

	/*! @brief Print errors to stream.
	 *
	 *  Prints all errors generated during parsing
	 *  to an output stream.
	 *
	 *  @param	os	output stream to write the missing items to
	 *  @return	reference to current object
	 */
	const WantParser & printErrors( std::ostream & os = std::cout ) const ;

	/*! @} */ // end of group

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

	/*! @name Want-List file options
	 *
	 *  These methods indicate whether the various options
	 *  have been provided in the want-list file.
	 *  These options are intended to be used by external modules.
	 *  Other supported options, e.g., ``REQUIRE-USERNAMES``, are processed
	 *  internally by WantParser.
	 *  The following options are supported for external use:
	 *
	 *  	HIDE-ERRORS
	 *  	HIDE-LOOPS
	 *  	HIDE-NONTRADES
	 *  	HIDE-STATS
	 *  	HIDE-SUMMARY
	 *  	SHOW-ELAPSED-TIME
	 *  	SHOW-MISSING
	 *  	SORT-BY-ITEM
	 */
	/*! @{ */ // start of group

	/*! @brief Show ``HIDE-ERRORS`` option.
	 *
	 *  Indicates whether the ``HIDE-ERRORS`` option
	 *  has been specified in the want-lists file.
	 *  WantParser will suppress any generated errors
	 *  if the option has been specified.
	 *
	 *  @return ``true`` if ``HIDE-ERRORS`` has been specified.
	 */
	bool hideErrors() const ;

	/*! @brief Show ``HIDE-LOOPS`` option.
	 *
	 *  Indicates whether the ``HIDE-LOOPS`` option
	 *  has been specified in the want-lists file.
	 *  MathTrader will not report the generated trade loops
	 *  if the option has been specified.
	 *
	 *  @return ``true`` if ``HIDE-LOOPS`` has been specified.
	 */
	bool hideLoops() const ;

	/*! @brief Show ``HIDE-NONTRADES`` option.
	 *
	 *  Indicates whether the ``HIDE-NONTRADES`` option
	 *  has been specified in the want-lists file.
	 *  MathTrader will not report the non-traded items
	 *  if the option has been specified.
	 *
	 *  @return ``true`` if ``HIDE-NONTRADES`` has been specified.
	 */
	bool hideNonTrades() const ;

	/*! @brief Show ``HIDE-STATS`` option.
	 *
	 *  Indicates whether the ``HIDE-STATS`` option
	 *  has been specified in the want-lists file.
	 *  MathTrader will not report the item stats
	 *  if the option has been specified.
	 *
	 *  @return ``true`` if ``HIDE-STATS`` has been specified.
	 */
	bool hideStats() const ;

	/*! @brief Show ``HIDE-SUMMARY`` option.
	 *
	 *  Indicates whether the ``HIDE-SUMMARY`` option
	 *  has been specified in the want-lists file.
	 *  MathTrader will not report the trade summary
	 *  if the option has been specified.
	 *
	 *  @return ``true`` if ``HIDE-SUMMARY`` has been specified.
	 */
	bool hideSummary() const ;

	/*! @brief Show ``SHOW-ELAPSED-TIME`` option.
	 *
	 *  Indicates whether the ``SHOW-ELAPSED-TIME`` option
	 *  has been specified in the want-lists file.
	 *  The elapsed time will be appended at the end of the results
	 *  if the option has been specified.
	 *
	 *  @return ``true`` if ``SHOW-ELAPSED-TIME`` has been specified.
	 */
	bool showElapsedTime() const ;

	/*! @brief Show ``SHOW-MISSING`` option.
	 *
	 *  Indicates whether the ``SHOW-MISSING`` option
	 *  has been specified in the want-lists file.
	 *  WantParser will report non-dummy items without a want-list
	 *  if the option has been specified.
	 *
	 *  @return ``true`` if ``SHOW-MISSING`` has been specified.
	 */
	bool showMissing() const ;

	/*! @brief Show ``SORT-BY-ITEM`` option.
	 *
	 *  Indicates whether the ``SORT-BY-ITEM`` option
	 *  has been specified in the want-lists file.
	 *  MathTrader will sort the item summary by item,
	 *  instead of by username,
	 *  if the option has been specified.
	 *
	 *  @return ``true`` if ``SORT-BY-ITEM`` has been specified.
	 */
	bool sortByItem() const ;

	/*! @} */ // end of group

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
	enum BoolOption {
		/* Internal options */
		ALLOW_DUMMIES,
		CASE_SENSITIVE,
		HIDE_REPEATS,
		REQUIRE_COLONS,
		REQUIRE_USERNAMES,
		SHOW_ELAPSED_TIME,
		/* External options */
		HIDE_ERRORS,
		HIDE_LOOPS,
		HIDE_NONTRADES,
		HIDE_STATS,
		HIDE_SUMMARY,
		SHOW_MISSING,
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
	 * Given options in want file.
	 * Used in @printOptions()
	 */
	std::list< std::string > _given_options;

	/**
	 * Enum of current status
	 */
	enum Status {
		INITIALIZATION,		/**< options may be given */
		PARSE_NAMES,		/**< parsing official names */
		PARSE_WANTS_NONAMES,	/**< parsing wants; no given official names */
		PARSE_WANTS_WITHNAMES,	/**< parsing wants; official names given */
	};
	Status _status;

	/**
	 * Indicates whether official names have been given.
	 */
	bool _official_given;

	std::list< std::string > errors_; /*!< errors during parsing */

	/***************************//*
	 * INTERNAL DATA STRUCTURES
	 *****************************/

	/**
	 * @brief Node struct
	 * Structure to hold node information
	 */
	typedef struct _Node_s {

		std::string item;		/*!< item name; e.g., 0001-PUERTO */
		std::string official_name;	/*!< official name; e.g., "Puerto Rico" */
		std::string username;		/*!< username; "Aldie" */
		bool dummy;			/*!< dummy node */
		bool has_wantlist;		/*!< want-list has been given */

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


	/***********************************
	 *  PARSING FUNCTIONS
	 ***********************************/

	/*! @brief Parse want-file line.
	 *
	 *  Receives a line from a want-file list
	 *  and analyzes it as follows:
	 *
	 *  1. Deduces its type based on the leading characters
	 *  (comment, directive, item, name).
	 *  2. If a directive, changes the @ref state_ of the class.
	 *  (TODO write own method)
	 *  3. Otherwise, calls the respective method:
	 *  @ref parseOption_(), @ref _parseOfficialName(), or @ref _parseWantList().
	 *
	 *  @param[in]	line	the entire line to parse
	 *  @throws	std::runtime_error if the line fails to parse
	 */
	void parseLine_( const std::string & line );

	/*! @brief Parse want-file option.
	 *
	 *  Parses a want-file option. Multiple options may be present
	 *  in the same line. Accepted formats:
	 *
	 *  * ``OPTION``
	 *  * ``OPTION-FOO``
	 *  * ``OPTION-FOO=0``
	 *  * ``OPTION=42``
	 *  * ``OPTION=-32``
	 *  * ``OPTION=+64``
	 *  * ``OPTION=VALUE-ABC-123``
	 *
	 *  @param[in]	option	option to parse, without the leading ``#!``
	 *  @throws	std::runtime_error if an unsupported ``option`` is given
	 */
	void parseOption_( const std::string & option );

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
