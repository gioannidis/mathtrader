/* This file is part of MathTrader++, a C++ utility
 * for finding, on a directed graph whose arcs have costs,
 * a set of vertex-disjoint cycles that maximizes the number
 * of covered vertices as a first priority
 * and minimizes the total cost as a second priority.
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
#include <iograph/wantparser.hpp>

#include <regex>
#include <sstream>
#include <stdexcept>


/************************************//*
 * 	PUBLIC METHODS - CONSTRUCTORS
 **************************************/

WantParser::WantParser() :
	BaseParser(),
	_bool_options( MAX_BOOL_OPTIONS, false ),
	_int_options( MAX_INT_OPTIONS ),
	_priority_scheme( "" ),
	_status( BEGIN )
{
	/**
	 * Default integer options.
	 */
	_int_options[SMALL_STEP] = 1;
	_int_options[BIG_STEP] = 9;
	_int_options[NONTRADE_COST] = 1e9;
}


/************************************//*
 * 	PUBLIC METHODS - OUTPUT
 **************************************/

const WantParser &
WantParser::print( std::ostream &os ) const {

	/**
	 * Print Nodes
	 */
	os << "@nodes"
		<< std::endl
		<< "label" << "\t"
		<< "item" << "\t"
		<< "official_name" << "\t"
		<< "username" << "\t"
		<< "dummy"
		<< std::endl;

	for ( auto const & node : _node_map ) {

		/**
		 * Skip if want list is missing
		 **/
		if ( node.second.has_wantlist ) {

			const std::string
				&item = node.second.item,
				&official_name = node.second.official_name,
				&username = node.second.username;
			bool dummy = node.second.dummy;

			os << item << "\t"	/**< item also used as label */
				<< item << "\t"
				<< official_name << "\t"
				<< username << "\t"
				<< dummy << "\t"
				<< std::endl;
		}
	}

	/**
	 * Print Arcs
	 */
	os << "@arcs"
		<< std:: endl
		<< "\t" << "\t"
		<< "rank" << "\t"
		<< std::endl;

	for ( auto const & it : _arc_map ) {

		auto const & arc_vector = it.second;

		for ( auto const & arc : arc_vector ) {

			auto const & target = arc.item_t;

			/**
			 * Valid if:
			 * 1) Arc is not unknown.
			 * 2) Target is present
			 * 	(TODO same as (1)?)
			 * 3) Target has want list
			 */
			const bool valid = (!arc.unknown)
				&& (_node_map.find(target) != _node_map.end())
				&& (_node_map.find(target)->second.has_wantlist);

			if ( valid ) {
				os << arc.item_s << "\t"
					<< arc.item_t << "\t"
					<< arc.rank
					<< std::endl;
			}
		}
	}

	return *this;
}

const WantParser &
WantParser::printOptions( std::ostream & os ) const {

	os << "Options: ";
	for ( auto const option : _given_options ) {
		os << option << " ";
	}
	os << std::endl;

	return *this;
}

const WantParser &
WantParser::printMissing( std::ostream & os ) const {

	unsigned count = 0;
	std::stringstream ss;

	for ( auto const & node_pair : _node_map ) {

		const std::string & item = node_pair.first;
		auto const & node = node_pair.second;
		if ( !_dummy(item) && !node.has_wantlist ) {
			count ++ ;
			ss << "**** Missing want list for item "
				<< item
				<< std::endl;
		}
	}

	if ( count > 0 ) {
		os << "MISSING ITEMS: "
			<< "(" << count
			<< " occurrence"
			<< ((count > 1)?"s":"")
			<< ")"
			<< std::endl
			<< ss.rdbuf()
			<< std::endl;
	}

	return *this;
}


/************************************//*
 * PUBLIC METHODS - EXTERNAL OPTIONS OUTPUT
 **************************************/

std::string
WantParser::getPriorityScheme() const {
	return _priority_scheme;
}

bool
WantParser::hideErrors() const {
	return _bool_options[HIDE_ERRORS];
}

bool
WantParser::hideLoops() const {
	return _bool_options[HIDE_LOOPS];
}

bool
WantParser::hideNonTrades() const {
	return _bool_options[HIDE_NONTRADES];
}

bool
WantParser::hideStats() const {
	return _bool_options[HIDE_STATS];
}

bool
WantParser::hideSummary() const {
	return _bool_options[HIDE_SUMMARY];
}

bool
WantParser::showElapsedTime() const {
	return _bool_options[SHOW_ELAPSED_TIME];
}

bool
WantParser::showMissing() const {
	return _bool_options[SHOW_MISSING];
}

bool
WantParser::sortByItem() const {
	return _bool_options[SORT_BY_ITEM];
}


/************************************//*
 * 	PUBLIC METHODS - UTILITIES
 **************************************/


/************************************//*
 * 	PRIVATE METHODS - PARSING
 **************************************/

void
WantParser::_parse( const std::string & buffer ) {

	/**
	 * Parse line by content:
	 * - Options: "#!"
	 * - Other directives
	 * - Comments
	 * - Items
	 */
	try {
		if ( buffer.compare(0, 2, "#!") == 0 ) {

			/**
			 * Option line;
			 * May only be given at the beginning.
			 * Isolate option (exlude "#!") and parse it.
			 */
			switch ( _status ) {
				case BEGIN: {
					const std::string option = buffer.substr(2, std::string::npos);
					_parseOption( option );
					break;
				}
				default:
					throw std::runtime_error("Options can only"
							" be given at the beginning"
							" of the file");
					break;
			}

		} else if ( buffer.compare(0, 1, "!") == 0 ) {

			/**
			 * Directives.
			 */
			if ( buffer.compare(0, 21, "!BEGIN-OFFICIAL-NAMES") == 0 ) {

				switch ( _status ) {
					case BEGIN:
						_status = PARSE_NAMES;
						break;

					case PARSE_NAMES:
						throw std::runtime_error("Official names"
								" are already being given");
						break;

					case PARSE_WANTS_WITHNAMES:
						throw std::runtime_error("Official names"
								" have already been given");
						break;

					default:
						throw std::runtime_error("Official names"
								" can only be declared"
								" before the want lists");
						break;
				}

			} else if ( buffer.compare(0, 19, "!END-OFFICIAL-NAMES") == 0 ) {\

				_status = PARSE_WANTS_WITHNAMES;

			} else {
				throw std::runtime_error("Unrecognized directive: "
						+ buffer);
			}
		} else {
			/**
			 * Item to be parsed. This is the default option
			 * and should be handled last.
			 * Use appropriate handler for current status.
			 */
			switch ( _status ) {
				case BEGIN:
					/**
					 * Always first.
					 * Do not break.
					 * Set status and continue.
					 */
					_status = PARSE_WANTS_NONAMES;

				case PARSE_WANTS_NONAMES:
				case PARSE_WANTS_WITHNAMES:
					_parseWantList( buffer );
					break;

				case PARSE_NAMES:
					_parseOfficialName( buffer );
					break;
				default:
					throw std::logic_error("Unknown handler for"
							" internal status "
							+ std::to_string(_status));
					break;
			}
		}

	} catch ( const std::exception & e ) {
		throw ;
	}
}

WantParser &
WantParser::_postParse() {

	/**
	 * Mark unknown items
	 */
	_markUnknownItems();
	return *this;
}

WantParser &
WantParser::_parseOption( const std::string & option_line ) {

	/**
	 * Tokenize line: ignore whitespaces
	 */
	static const std::regex e(R"(\S+)");
	auto elems = BaseParser::_split( option_line, e );

	/**
	 * Verify that we've found an option;
	 * ignore if empty.
	 */
	if ( elems.empty() ) {
		return *this;
	}

	/**
	 * Handle option according to type in order:
	 * - Integer
	 * - Priorities
	 * - String
	 */
	for ( auto const & option : elems ) {

		/**
		 * Always add to given options list
		 */
		this->_given_options.push_back( option );

		/**
		 * Regex to match:
		 * - Integer options
		 * - Priorities
		 */
		static const std::regex
			regex_int(R"(\b\S+=[0-9]+)"),
			regex_prio(R"(\b([^-])+-PRIORITIES)");

		if ( std::regex_match(option, regex_int) ) {

			const std::string digits(R"(([0-9]+)|(\b([^=])+))");
			auto const int_elems = _split( option, digits );

			/**
			 * Int option; tokenize to retrieve name value.
			 * Accepted format:
			 * 	#! VARIABLE=42
			 */
			if ( int_elems.empty() ) {
				throw std::logic_error("Integer option not matched");
			}

			/**
			 * Get name of int option.
			 */
			const std::string int_option_name = int_elems[0];
			if ( int_elems.size() < 2 ) {
				throw std::runtime_error("Value for integer option "
						+ int_option_name + " not found");
			}

			/**
			 * Get value of int option.
			 */
			const std::string &value = int_elems[1];

			/**
			 * Get int option from map and set it.
			 */
			auto const it = _int_option_map.find( int_option_name );
			if ( it == _int_option_map.end() ) {
				throw std::runtime_error("Unknown integer option "
						+ int_option_name);
			}

			const IntOption int_option = it->second;
			_int_options[ int_option ] = std::stoi(value);

		} else if ( std::regex_match(option, regex_prio) ) {

			/**
			 * Priority scheme. Must have the form of ("XXXX-PRIORITY").
			 * Do not check its validity,
			 * that's the responsibility of MathTrader.
			 */
			_priority_scheme = option;

		} else if ( _bool_option_map.find(option) != _bool_option_map.end() ) {

			/**
			 * String option; set to true.
			 */
			auto const it = _bool_option_map.find(option);
			const BoolOption bool_option = it->second;
			_bool_options[ bool_option ] = true;


		} else {
			throw std::runtime_error("Unknown option " + option);
		}
	}

	return *this;
}

WantParser &
WantParser::_parseOfficialName( const std::string & line ) {

	/**
	 * Regular expression to separate fields.
	 *
	 * Example of an official name line:
	 * 0042-PUERTO ==> "Puerto Rico" (from username) [copy 1 of 2]
	 *
	 * $1	0042-PUERTO
	 * $2	==>
	 * $3	"Puerto Rico"
	 * $4	(from username)
	 * $5	[copy 1 of 2]
	 *
	 * We may also parse single-nested quotation marks, e.g.:
	 * 0042-IPOPTSE ==> ""In Pursuit of Par" TPC Sawgrass Edition" (from username)
	 *
	 * NOTE: regexes are eager, meaning that the longest/more specialized
	 * matching should be put first.
	 * A g++-4.9 bug might produce the correct results if group W
	 * is in the wrong position. g++-5.3 fixes this.
	 */
	static const std::regex FPAT_names(
		R"(\"([\"]|[^\"])+\")"	// Group 1: quotation mark pairs
					// with possible quotation marks included.
					// This finds the longest stream of quotation marks.
					// TODO If usernames or descriptions have
					// quotation marks, we will have a problem here.
		"|"
		R"(\([^\)]+\))"		// Group 2: parentheses
		"|"
		R"(\[[^\]]+\])"		// Group 3: brackets
		"|"
		R"(\S+)"		// Group 4: any non-whitespace
	);

	/**
	 * Tokenize the line
	 */
	auto match = _split( line, FPAT_names );

	/**
	 * Sanity check for minimum number of matches
	 * TODO the description (4th item) is optional.
	 */
	if ( match.size() < 4 ) {
		throw std::runtime_error("Bad format of official name line");
	}

	/**
	 * Item name: to be used as a hash key.
	 */
	const std::string
		&orig_item = match[0],
		&orig_official_name = match[2],
		&from_username = match[3];

	/**
	 * Parse item name (quotation marks, uppercase).
	 * As we're not providing a username,
	 * it will raise an error if it's dummy.
	 */
	std::string item( orig_item );
	_parseItemName( item );

	/**
	 * Replace nested quotation marks in official_name with "'".
	 * Replace backslashes with forward slashes;
	 * we are not going to escape them.
	 * Ignore the first and the last position of the string.
	 * TODO create static private method.
	 */
	std::string official_name( orig_official_name );
	size_t pos = 1;
	while ((( pos = official_name.find( "\"", pos )) != std::string::npos )
			&& ( pos != official_name.length()-1 )) {

		official_name.replace( pos, 1, "'");
		pos += 1;
	}
	pos = 1;
	while ((( pos = official_name.find( "\\", pos )) != std::string::npos )
			&& ( pos != official_name.length()-1 )) {

		official_name.replace( pos, 1, "/");
		pos += 1;
	}

	/**
	 * from_username is in format:
	 * 	(from user name)
	 * Change to:
	 * 	"user name"
	 * TODO it might not be a username;
	 * match with "(from xxx)" and ignore otherwise.
	 */
	std::string username;
	try {
		username = from_username.substr(6, std::string::npos); /**< remove "(from " */
	} catch ( const std::out_of_range & e ) {
		std::cerr << "Out of range when parsing username: "
			<< from_username
			<< " in line "
			<< line
			<< "; received error: "
			<< e.what()
			<< std::endl;
		std::cerr << "Tokens:" << std::endl;
		for ( auto const & token : match ) {
			std::cerr << "\t" << token << std::endl;
		}
		return *this;
	} catch ( const std::exception & e ) {
		throw;
	}

	username.pop_back(); /**< remove last ')' */
	_parseUsername( username ); /**< quotation marks, uppercase */

	/**
	 * Emplace the item. It should not exist in the node_map.
	 */
	auto rv = this->_node_map.emplace( item, _Node_s(item,official_name,username) );

	/**
	 * Throw if it's already in the map.
	 * TODO necessary?
	 */
	if ( !rv.second ) {
		throw std::runtime_error("Existing entry for item "
				+ item);
	}

	return *this;
}

WantParser &
WantParser::_parseWantList( const std::string & line ) {

	static const std::regex FPAT_want(
		R"(\([^\)]+\))"		// Group 1: parentheses
		"|"
		R"([^\s:;]+)"		// Group 2: any non-whitespace,
					// colon, or semicolon
		"|"
		R"(:)"
		"|"
		R"(;)"
	);

	/**
	 * Summary:
	 * 1. Tokenize the line.
	 * 2. Parse username.
	 * 3. Parse offering item name (source).
	 * 4. Parse colon.
	 * 5. Parse wanted items (targets).
	 *
	 * Full want list format:
	 * (user name) ITEM_A : ITEM_B ITEM_C ; ITEM_D %DUMMY1 %DUMMY2 ITEM_E
	 *
	 * REQUIRE-USERNAMES: "(user name)" are mandatories; optional otherwise.
	 * REQUIRE-COLONS: ":" are mandatories; optional otherwise.
	 * ALLOW-DUMMIES: not possible if REQUIRE-USERNAMES not set.
	 */

	/**
	 * Tokenize the line
	 */
	auto const match = _split( line, FPAT_want );
	if ( match.empty() ) {
		throw std::runtime_error("Bad format of want list");
	}


	/**********************************//*
	 *	USERNAME
	 *************************************/

	/**
	 * Check if there is a username;
	 * first & last characters should be '(' && ')'.
	 * It should be the first token.
	 */
	unsigned n_pos = 0;
	const std::string &username_p = match[n_pos];

	const bool has_username = (( username_p.front() == '(' )
			&& ( username_p.back() == ')' ));

	/**
	 * Advance n_pos if we have a username.
	 */
	if ( has_username ) {
		++ n_pos ;
	} else if (  _bool_options[ REQUIRE_USERNAMES ] ) {
		throw std::runtime_error("Missing username from want list");
	}


	/**********************************//*
	 *	OFFERED ITEM NAME (source)
	 *************************************/

	/**
	 * Check whether there is a wanted item name.
	 */
	if ( match.size() < (n_pos + 1) ) {
		throw std::runtime_error("Missing offered item from want list");
	}
	const std::string &source = match[n_pos];

	/**
	 * Append username on item name, if dummy.
	 */
	std::string item = source;

	/**
	 * Parse the item name (dummy, uppercase, etc).
	 */
	if ( has_username ) {
		_parseItemName( item, username_p );
	} else {
		_parseItemName( item );
	}

	/**
	 * Remove parentheses from username: first and last character.
	 * Parse the username.
	 */
	std::string username = username_p.substr(1, std::string::npos);
	username.pop_back();
	_parseUsername( username );

	/**
	 * Check if the item is present in node_map.
	 * Handle cases when present or not.
	 * Check if it has a want list.
	 */
	bool * has_wantlist = NULL;	/**< Will point to has_wantlist member */
	{
		auto it = _node_map.find(item);

		if ( it == _node_map.end() ) {

			/**
			 * Handle case when offering item
			 * is NOT in node_map:
			 * --> dependent on when official names
			 *     have been given or not.
			 */
			switch ( _status ) {
				case PARSE_WANTS_NONAMES: {

					/**
					 * No official names have been given;
					 * there should be no entry.
					 * Sanity check:
					 * If an entry is found, then it should have a wantlist.
					 */
					break;
				}

				case PARSE_WANTS_WITHNAMES: {

					/**
					 * Official names have been given;
					 * there SHOULD be an entry,
					 * unless it's a dummy item.
					 * This check usually catches spelling errors.
					 */
					if ( !_dummy(item) ) {

						throw std::runtime_error("Non-dummy item "
								+ item
								+ " has no official name."
								" Hint: spelling error?");
					}
					break;
				}

				default:
					throw std::logic_error("Bad internal status during"
							" want list parsing: "
							+ std::to_string( _status ));
					break;
			}

			/**
			 * Insert the item in the node_map.
			 * Point the iterator to the new item.
			 */
			auto const & pair = this->_node_map.emplace(item,
					_Node_s(item, item, username, _dummy(item)));
			it = pair.first;
		}

		/**
		 * Now, iterator @it should point to the item's entry,
		 * whether it's just been added or not.
		 * Check if it has a wantlist
		 */
		has_wantlist = &(it->second.has_wantlist);

		if ( *has_wantlist ) {
			throw std::runtime_error("Ignoring multiple wantlist for item "
					+ item);
		}
	}

	/**
	 * Finally, advance n_pos.
	 * We should always have an offering item.
	 */
	++ n_pos;


	/**********************************//*
	 *	CHECK COLONS
	 *************************************/

	/**
	 * Dependent scope, to limit
	 * the unnecessary variables.
	 */
	{
		const bool has_colon = ((match.size() >= (n_pos + 1))
				&& (match[n_pos].compare(":") == 0));

		/**
		 * Advance n_pos if we have a colon
		 */
		if ( has_colon ) {
			++ n_pos;
		} else if ( _bool_options[REQUIRE_COLONS] ) {
			throw std::runtime_error("Missing colon from want list");
		}
	}


	/**********************************//*
	 *	WANTED ITEMS (targets)
	 *************************************/

	/**
	 * Check if want list already exists.
	 * This may happen if a user has defined multiple want lists
	 * or another line was split over two lines.
	 * FIXME use "has_wantlist" pointer?
	 */
	if ( _arc_map.find(item) != _arc_map.end() ) {
		throw std::runtime_error("Multiple want lists for item "
				+ item);
	}

	/**
	 * First wanted item.
	 */
	const unsigned first_wanted_pos = n_pos;

	/**
	 * Initialize rank
	 */
	int rank = 1;

	/**
	 * Initialize list with want-lists to be added.
	 * All parsed want lists are added to this map
	 * and they will be eventually added
	 * to the official graph
	 * if *no errors* whatsoever are detected.
	 * On errors, the whole line is discarded.
	 */
	std::list< _Arc_t > arcs_to_add;


	/**********************************//*
	 *	WANTED ITEMS ITERATOR
	 *************************************/

	for ( unsigned i = first_wanted_pos; i < match.size(); ++ i ) {

		auto const & small_step = _int_options[SMALL_STEP];
		auto const & big_step   = _int_options[BIG_STEP];

		std::string target = match[i];

		/**
		 * Cases:
		 * 1. Semicolon:
		 * 	"increase the rank of the next item by the big-step value"
		 * 	NOTE: the small-step of the previous item will also be applied.
		 * 2. Colon: raise an error. There should be no colon here.
		 * 3. Actual wanted item.
		 */
		if ( target.compare(";") == 0 ) {
			rank += big_step;
		} else if ( target.compare(":") == 0 ) {
			throw std::runtime_error("Colon occurence at invalid position "
					+ std::to_string(i+1));
		} else {

			/**
			 * Parse the item name (dummy, uppercase, etc).
			 */
			if ( has_username ) {
				_parseItemName( target, username_p );
			} else {
				_parseItemName( target );
			}

			/**
			 * Push (item-target) arc to map.
			 */
			arcs_to_add.push_back(_Arc_t( item, target, rank ));
		}

		/**
		 * Advance always the rank by small-step
		 */
		rank += small_step;
	}

	/**
	 * Create ArcMap entry for item;
	 * in C++11 we can directly move the items from the original list
	 * to the vector; we don't have to copy them!
	 */
	_arc_map.emplace( item, std::vector< _Arc_t > {
			std::make_move_iterator(std::begin(arcs_to_add)),
			std::make_move_iterator(std::end(arcs_to_add)) }
			);

	/**
	 * Mark the item has having a want_list.
	 */
	if ( has_wantlist != NULL ) {
		*has_wantlist = true;
	} else {
		/* Should never get here */
		throw std::logic_error("Internal has_wantlist pointer"
				" is NULL");
	}

	return *this;
}

WantParser &
WantParser::_parseItemName( std::string & item,
		const std::string username ) {

	if ( _dummy(item) ) {

		/**
		 * Sanity check for dummy item
		 */
		if ( !_bool_options[ALLOW_DUMMIES] ) {

			throw std::runtime_error("Dummy item "
					+ item
					+ " detected, but dummy items"
					" not allowed");

		} else if ( username.length() == 0 ) {

			throw std::runtime_error("Dummy item "
					+ item
					+ " detected, but username "
					" not defined");
		}

		/**
		 * Append username
		 */
		item.push_back('-');
		item.append(username);
	}

	/**
	 * Unless item names are case sensitive,
	 * convert to uppercase.
	 */
	if ( !_bool_options[CASE_SENSITIVE] ) {
		_toUpper( item );
	}

	/**
	 * Enclose in quotation marks
	 */
	_quotationMarks(item);

	return *this;
}


/************************************//*
 * 	PRIVATE METHODS - UTILS
 **************************************/

WantParser &
WantParser::_markUnknownItems() {

	_unknown_item_map.clear();

	for ( auto & it : _arc_map ) {

		auto & arc_vector = it.second;
		for ( auto & arc : arc_vector ) {

			/**
			 * Target not found?
			 */
			const std::string & target = arc.item_t;
			if ( _node_map.find(target) == _node_map.end() ) {
				arc.unknown = true;

				auto it = _unknown_item_map.find(target);
				if ( it != _unknown_item_map.end()) {
					it->second ++ ;
				} else {
					_unknown_item_map.emplace(target,1);
				}
			}
		}
	}

	return *this;
}

bool
WantParser::_dummy( const std::string & item ) {

	int offset = 0;

	/**
	 * Is the first character a quotation mark?
	 * Then offset by 1.
	 */
	if ( item.front() == '"' ) {
		offset = 1;
	}

	return ( item.compare(0 + offset, 1, "%") == 0 );
}


/************************************//*
 * 	PRIVATE STATIC MEMBERS
 **************************************/

/**
 * Unordered_map to map string-options to enums.
 * Tip: list them alphabetically here
 * to ease error-detection.
 */
const std::unordered_map< std::string, WantParser::BoolOption  >
WantParser::_bool_option_map = {
	{"ALLOW-DUMMIES",	ALLOW_DUMMIES},
	{"CASE_SENSITIVE",	CASE_SENSITIVE},
	{"HIDE-ERRORS",		HIDE_ERRORS},
	{"HIDE-LOOPS",		HIDE_LOOPS},
	{"HIDE-NONTRADES",	HIDE_NONTRADES},
	{"HIDE-REPEATS",	HIDE_REPEATS},
	{"HIDE-STATS",		HIDE_STATS},
	{"HIDE-SUMMARY",	HIDE_SUMMARY},
	{"REQUIRE-COLONS",	REQUIRE_COLONS},
	{"REQUIRE-USERNAMES",	REQUIRE_USERNAMES},
	{"SHOW-ELAPSED-TIME",	SHOW_ELAPSED_TIME},
	{"SHOW-MISSING",	SHOW_MISSING},
	{"SORT-BY-ITEM",	SORT_BY_ITEM},
};


/**
 * Unordered_map to map string-options to enums.
 */
const std::unordered_map< std::string, WantParser::IntOption  >
WantParser::_int_option_map = {
	{"SMALL-STEP",		SMALL_STEP},
	{"BIG-STEP",		BIG_STEP},
	{"NONTRADE_COST",	NONTRADE_COST},
};
