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
#ifndef _MATHTRADER_MATHTRADER_IOGRAPH_INCLUDE_IOGRAPH_WANTPARSER_HPP_
#define _MATHTRADER_MATHTRADER_IOGRAPH_INCLUDE_IOGRAPH_WANTPARSER_HPP_

/*! @file wantparser.hpp
 *  @brief Want-list to graph conversions
 *
 *  Process a want-list file and convert it
 *  to Lemon Graph Format.
 */

#include <iostream>
#include <list>
#include <map>
#include <regex>
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
	/*! @brief Default constructor.
	 */
	WantParser();

	~WantParser() = default;

	/************************
	 * 	INPUT		*
	 ************************/

	/*! @name Want-list input
	 *
	 *  Methods to feed a want-list file to be parsed
	 *  and converted to a graph.
	 */
	/*! @{ */ // start of group

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

	/*! @brief Convert want-lists from URL to graph.
	 *
	 *  Reads a want-list from the given URL
	 *  and converts it to a graph.
	 *  The input url is expected to be in the form of:
	 *
	 *  	http://bgg.activityclub.org/olwlg/XXXXXX-officialwants.txt
	 *
	 *  Where ``XXXXXX`` is the unique identifier of the trade.<br>
	 *  Example:
	 *
	 *  	http://bgg.activityclub.org/olwlg/207635-officialwants.txt
	 *
	 *  @param[in]	url	URL of input stream to fetch and read
	 */
	void parseUrl( const std::string & url );

	/*! @} */ // end of group

	/************************
	 * 	GRAPH OUTPUT	*
	 ************************/

	/*! @name Graph and debug output
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
	 *  @throws	std::runtime_error if file ``fn`` cannot be opened
	 */
	void print( const std::string & fn ) const ;

	/*! @brief Print LGF output file to stream.
	 *
	 *  Prints the graph generated by the want-list input file
	 *  in the Lemon Graph Format (LGF)
	 *  to an output file.
	 *
	 *  @param	os	output stream to write the LGF file to
	 */
	void print( std::ostream & os = std::cout ) const ;

	/*! @brief Print want-list file options.
	 *
	 *  Prints all options that were provided to the input
	 *  want-list file.
	 *
	 *  @param	os	output stream to write the options to
	 */
	void printOptions( std::ostream & os = std::cout ) const ;

	/*! @brief Print missing items.
	 *
	 *  Prints all non-dummy items without a given want-list.
	 *
	 *  @param	os	output stream to write the missing items to
	 */
	void printMissing( std::ostream & os = std::cout ) const ;

	/*! @brief Print errors to stream.
	 *
	 *  Prints all errors generated during parsing
	 *  to an output stream.
	 *
	 *  @param	os	output stream to write the missing items to
	 */
	void printErrors( std::ostream & os = std::cout ) const ;

	/*! @} */ // end of group

	/********************************
	 * 	INDIVIDUAL OPTIONS	*
	 ********************************/

	/*! @name Query individual options in want-list file
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

	/*! @brief Get priority scheme.
	 *
	 *  Gets the given priority scheme, in the form of "PRIORITY-XXX".
	 *
	 *  Note that the priority scheme might be invalid or not implemented yet;
	 *  it's not the responsibility of WantParser to check its validity.
	 *
	 *  @returns	the priority scheme; empty if no priority scheme has been given
	 */
	std::string getPriorityScheme() const ;

	/*! @} */ // end of group

	/************************
	 * 	OUTPUT STATS	*
	 ************************/

	/*! @name Stats of the generated want-list graph
	 *
	 *  These methods allow the user to query for specific
	 *  stats regarding the generated want-list graph,
	 *  such as total number of trading items.
	 */
	/*! @{ */ // start of group

	/*! @brief Total number of submitted items.
	 *
	 *  Queries the object for
	 *  the total number of officially trading items.
	 *  This includes items without a want-list,
	 *  but not unknown or dummy items.
	 *  Multiple copies of the same item are reported as a unique item.
	 *
	 *  @returns number of officially trading items
	 */
	unsigned getNumItems() const ;

	/*! @brief Number of missing want-lists.
	 *
	 *  Queries the object for
	 *  the number of missing want-lists.
	 *  This does not include dummy items with missing want-lists.
	 *  Multiple copies of the same item are reported as a unique item.
	 *
	 *  @returns number of missing want-lists
	 */
	unsigned getNumMissingItems() const ;

	/*! @} */ // end of group

private:
	/*! @brief Supported want-list boolean options.
	 *
	 *  All want-list boolean options supported by the WantParser.
	 *  Boolean options are specified in the want-list file as,
	 *  e.g., ``#! ALLOW-DUMMIES``.
	 *
	 *  @ref bool_option_map_ is used to map
	 *  an option string to the actual @ref BoolOption_ value.
	 */
	enum BoolOption_ {
		/* Internal options */
		ALLOW_DUMMIES = 0,	/*!< allow dummy items */
		CASE_SENSITIVE,		/*!< want-lists are case-sensitive */
		HIDE_REPEATS,		/*!< WantParser will not report repeated items */
		REQUIRE_COLONS,		/*!< WantParser requires colons after usernames */
		REQUIRE_USERNAMES,	/*!< WantParser requires usernames to be given in want-lists */
		/* External options */
		HIDE_ERRORS,		/*!< WantParser will suppress any errors */
		HIDE_LOOPS,		/*!< MathTrader will not report the trade loops */
		HIDE_NONTRADES,		/*!< MathTrader will not report the non-traded items */
		HIDE_STATS,		/*!< MathTrader will not report item stats */
		HIDE_SUMMARY,		/*!< MathTrader will not print the final summary */
		SHOW_ELAPSED_TIME,	/*!< Total elapsed time will be appended at the end */
		SHOW_MISSING,		/*!< WantParser will report missing items */
		SORT_BY_ITEM,		/*!< MathTrader will summarize the items by item name, instead of username */
		/* Not implemented */
		MAX_BOOL_OPTIONS	/*!< not an option; always the __last__ option */
	};

	/*! @brief Supported want-list integer options.
	 *
	 *  All want-list integer options supported by the WantParser.
	 *  Integer options are specified in the want-list file as,
	 *  e.g., ``#! BIG-STEP=42``.
	 *
	 *  @ref int_option_map_ is used to map
	 *  an option string to the actual @ref BoolOption_ value.
	 */
	enum IntOption_ {
		SMALL_STEP = 0,		/*!< default arc cost increment in want-lists */
		BIG_STEP,		/*!< additional arc cost increment in want-lists if ``;`` is given */
		NONTRADE_COST,		/*!< cost of arcs that should not be chosen by default by the trade solver */
		/* Not implemented */
		MAX_INT_OPTIONS		/*!< not an option; always the __last__ option */
	};

	/*! @brief String to @ref BoolOption_ map.
	 *
	 *  Maps boolean option strings detected in a want-list file
	 *  to a @ref @BoolOption_ value.<br>
	 *  Example: the ``"ALLOW-DUMMIES"`` option string is mapped to @ref ALLOW_DUMMIES.
	 */
	static const std::unordered_map< std::string, BoolOption_ >
		bool_option_map_;

	/*! @brief String to @ref IntOption_ map.
	 *
	 *  Maps integer option strings detected in a want-list file
	 *  to a @ref @IntOption_ value.<br>
	 *  Example: the ``"BIG-STEP=42"`` option string is mapped to @ref BIG_STEP.
	 */
	static const std::unordered_map< std::string, IntOption_ >
		int_option_map_;

	/*! @brief Boolean option vector.
	 *
	 *  Vector containing all supported boolean options.
	 *  All options are initialized to ``false``.
	 *  Each position corresponds to a boolean option from @ref BoolOption_.
	 */
	std::vector< bool > bool_options_
		= std::vector< bool >( MAX_BOOL_OPTIONS, false );

	/*! @brief Integer option vector.
	 *
	 *  Vector containing all supported integer options.
	 *  Each position corresponds to a integer option from @ref IntOption_.
	 */
	std::vector< int > int_options_
		= std::vector< int >( MAX_INT_OPTIONS );

	/*! @brief The priority scheme.
	 *
	 *  Holds the priority scheme that has been specified by the want-list file.
	 */
	std::string priority_scheme_ = "";

	/*! @brief Provided options.
	 *
	 *  List of all options that were provided in the parsed
	 *  want file.
	 */
	std::list< std::string > given_options_;

	/*! @brief Parsing status.
	 *
	 *  This is used to control whether official names have been given or not,
	 *  since they may only be given at the beginning.
	 */
	enum Status {
		INITIALIZATION = 0,	/*!< options may be given */
		PARSE_NAMES,		/*!< parsing official names */
		PARSE_WANTS_NONAMES,	/*!< parsing wants; no given official names */
		PARSE_WANTS_WITHNAMES,	/*!< parsing wants; official names given */
		/* Not implemented */
		MAX_STATUS_ENUMS	/*!< not a status; always the __last__ option */
	};

	/*! @brief Current parsing status.
	 *
	 *  Indicates the current parsing status of the object.
	 */
	Status status_ = INITIALIZATION;

	/*! @brief Generated errors.
	 *
	 *  Holds a list of errors that were generated during
	 *  the want-list file parsing,
	 *  including the number of the line that generated the error.
	 */
	std::list< std::string > errors_;

	/****************************************
	 *	INTERNAL DATA STRUCTURES	*
	 ****************************************/

	/*! @brief Graph Node.
	 *
	 *  Represents an item, which is mapped to a graph node.
	 */
	typedef struct Node_s_ {

		std::string item;		/*!< item name, representing the node ID; e.g., 0001-PUERTO */
		std::string official_name;	/*!< official name; e.g., "Puerto Rico" */
		std::string username;		/*!< username; "Aldie" */

		inline Node_s_ ( const std::string & _item,
				const std::string & _official,
				const std::string & _user
				) :
			item( _item ),
			official_name( _official ),
			username( _user ) {}
	} Node_t_;

	/*! @brief Graph Arc.
	 *
	 *  Represents a "want-item" relationship, which is mapped to a graph arc.
	 *  The source node respresents the offered item,
	 *  while the target node respresents the wanted item.
	 */
	typedef struct Arc_s_ {

		std::string item_s;	/**< item name; source ID */
		std::string item_t;	/**< item name; target ID */
		int rank;		/**< rank (cost) of arc */

		inline Arc_s_ ( const std::string & _source,
				const std::string & _target,
				int _rank ) :
			item_s( _source ),
			item_t( _target ),
			rank( _rank ) {}

	} Arc_t_;

	/*! @brief Map of graph nodes.
	 *
	 *  Map of all graph nodes. The node ID (item name)
	 *  is used as the map key.
	 *  Using ``std::map`` instead of ``std::unordered_map``
	 *  to print the nodes in alphabetical order.
	 */
	std::map< std::string , Node_t_ > node_map_;

	/*! @brief Map of graph arcs.
	 *
	 *  Map of all graph arcs. The source node ID (source item name)
	 *  is used as the map key.
	 *  Using ``std::map`` instead of ``std::unordered_map``
	 *  to print the arcs in source-node alphabetical order.
	 */
	std::map< std::string ,
		std::vector< Arc_t_ > >  arc_map_;

	/***********************************
	 *  PARSING METHODS
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
	 *  @ref parseOption_(), @ref parseOfficialName_(), or @ref parseWantList_().
	 *
	 *  @param[in]	line	the entire line to parse
	 *  @throws	std::runtime_error if the line fails to parse
	 */
	void parseLine_( const std::string & line );

	/*! @brief Parse want-file option.
	 *
	 *  Parses line containing want-file options. Multiple options may be present
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
	 *  @param[in]	line to parse, without the leading ``#!``
	 *  @throws	std::runtime_error if an unsupported ``option`` is given
	 */
	void parseOption_( const std::string & option );

	/*! @brief Parse official item name.
	 *
	 * Parses a line giving the official name of an item.
	 * Extracts the item ID, the username and the official item name
	 * and passes them to @ref addSourceItem_().
	 *
	 *  @param[in]	line to parse
	 *  @throws	std::runtime_error if the line fails to parse, e.g., bad format
	 *  @throws	std::runtime_error if the item ID has been already parsed
	 */
	void parseOfficialName_( const std::string & line );

	/*! @brief Parse entire want-list line.
	 *
	 *  Minimum accepted format, indicating that target (wanted)
	 *  items ``T1``, ``T2`` or ``T3`` are wanted for offered (source)
	 *  item ``S``.
	 *
	 *  	S T1 T2 T3
	 *
	 *  If @ref REQUIRE_COLONS has been given,
	 *  then the following format is required (otherwise, optional):
	 *
	 *  	S : T1 T2 T3
	 *
	 *  If @ref REQUIRE_USERNAMES has been given,
	 *  then the following format is required (otherwise, optional):
	 *
	 * 	(USERNAME) S T1 T2 T3
	 *
	 *  Combined, if both @ref REQUIRE_COLONS and @ref REQUIRE_USERNAMES
	 *  have been specified:
	 *
	 *  	(USERNAME) S : T1 T2 T3
	 *
	 *  The last example is the most usual format.
	 *  No wanted items are registered if any errors are detected.
	 *
	 *  Calls:
	 *  1. @ref extractUsername_() to retrieve the username from the line
	 *  2. @ref addSourceItem_() to add the source item, if not already there
	 *  3. @ref addTargetItems_() to add the target items for the extracted source item
	 *
	 *  @param[in]	line	line to extract and parse the want-list from
	 *  @throws	std::runtime_error if bad line format is detected
	 *  @throws	std::runtime_error if the username is absent
	 *  		but @ref REQUIRE_USERNAMES has been given.
	 *  @throws	std::runtime_error if a colon after the source item is absent
	 *  		but @ref REQUIRE_COLONS has been given.
	 */
	void parseWantList_( const std::string & line );

	/*! @brief Extract username from token.
	 *
	 *  Parses a string token and extracts the username from it.
	 *  Expected token format: a username enclosed in parentheses, e.g., ``(USERNAME)``
	 *
	 *  Example: if ``(Username123)`` is given, it extracts ``Username123``.
	 *
	 *  @param[in]	token	token to parse and extract username
	 *  @returns	extracted username; empty if ``token`` does not have a valid format
	 */
	static std::string extractUsername_( const std::string & token );

	/*! @brief Add source (offered) item.
	 *
	 *  Registers a new source item,
	 *  i.e., an item offered up for trade.
	 *
	 *  @param[in]	item	new source item to register
	 *  @param[in]	official_name	official name of the item
	 *  @param[in]	username	username of the item's owner
	 *  @throws conditionally a std::runtime_error if the item
	 *  has been already registered, except if we are currently reading
	 *  the want lists and already have the official names.
	 */
	void addSourceItem_( const std::string & item,
			const std::string & official_name,
			const std::string & username );

	/*! @brief Convert to upper case and append username.
	 *
	 *  Converts the item name to be subsequently stored
	 *  as a graph node in @ref Node_s_.
	 *  It applies the following conversions:
	 *
	 *  1. Checks whether the item is dummy through @ref isDummy_()
	 *  and appends ``username`` to the target item name, enclosed in parentheses.
	 *  2. Converts the target item name to uppercase,
	 *  unless the @ref CASE_SENSITIVE option in the @ref bool_options_
	 *  has been given.
	 *  3. Encloses the entire item name in quotation marks.
	 *
	 *  Example: dummy item ``%Puerto`` from user ``Aldie``
	 *  with no case-sensitive items will become ``%PUERTO-(ALDIE)``.
	 *
	 *  @param	item	the item name to be converted
	 *  @param	username	username to append to ``item``, if dummy
	 *  @returns	converted item name
	 *
	 *  @throws std::runtime_error if a dummy item is given,
	 *  but @ref ALLOW_DUMMIES in @ref bool_options_ is ``false``.
	 *  @throws std::runtime_error if ``item`` is dummy, but the ``username`` is empty.
	 */
	std::string convertItemName_( const std::string & item,
			const std::string username = std::string() ) const;

	/*! @brief Add target (wanted) items.
	 *
	 *  Adds new wanted items for a given source (offered) item.
	 *  The wanted items are registered if and only if no errors are generated.
	 *
	 *  @param[in]	source	the source (offered) item
	 *  @param[in]	wanted_items	vector of target (wanted) items
	 *
	 *  @throws	std::runtime_error if ``source`` item has already a want-list
	 *  @throws	std::runtime_error if bad line format is detected
	 *  @throws std::runtime_error if a dummy target item is detected,
	 *  but @ref ALLOW_DUMMIES in @ref bool_options_ is ``false``.
	 */
	void addTargetItems_( const std::string & source,
			const std::vector< std::string > & wanted_items );


	/****************************************
	 *  	UTILITY STATIC FUNCTIONS	*
	 ****************************************/

	/*! @brief Check if item name is dummy.
	 *
	 *  Parses the given item name and checks whether it's dummy.
	 *  Dummy items begin with ``%``, excluding any leading quotation mark (``"``).
	 *
	 *  @param[in]	item	the item name to be checked
	 *  @returns	``true`` if the item name is dummy, ``false`` otherwise or if empty
	 */
	static bool isDummy_( const std::string & item );

	/*! @brief Tokenize line.
	 *
	 *  Tokenizes a line based on a given regular expression.
	 *  @param[in]	input	line to tokenize
	 *  @param[in]	regex	regular expression to use
	 *  @returns	vector with individual tokens
	 */
	static std::vector< std::string > split_(
			const std::string & input,
			const std::regex & regex );

	/*! @brief Tokenize line.
	 *
	 *  Tokenizes a line based on a given regular expression.
	 *  Converts input string to regex and calls @ref split_().
	 *  @param[in]	input	line to tokenize
	 *  @param[in]	str	string to convert to regular experssion
	 *  @returns	vector with individual tokens
	 */
	static std::vector< std::string > split_(
			const std::string & input,
			const std::string & str );

	/*! @brief Retrieve payload from URL.
	 *
	 *  Accepts a URL, downloads the data and returns the payload.
	 *
	 *  @param[in]	url	the URL to fetch the data from
	 *  @param[out]	data	stores the retrieved payload; existing data is overwritten
	 *  @throw	SocketException		if a socket error occurs
	 *  @throw	std::runtime_error	if malformed data is received
	 *  @throw	std::logic_error	if the receive buffer is too short
	 */
	static void getUrl_( const std::string & url,
			std::string & data );
};

#endif /* _WANTPARSER_HPP_ */
