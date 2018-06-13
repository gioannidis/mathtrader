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

#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>

#include <iograph/baseparser.hpp>


/************************************//*
 * 	PUBLIC METHODS - CONSTRUCTORS
 **************************************/

WantParser::WantParser() :
	_bool_options( MAX_BOOL_OPTIONS, false ),
	_int_options( MAX_INT_OPTIONS ),
	_priority_scheme( "" ),
	_status( INITIALIZATION )
{
	_int_options[SMALL_STEP] = 1;
	_int_options[BIG_STEP] = 9;
	_int_options[NONTRADE_COST] = 1e9;
}


/************************************//*
 * 	PUBLIC METHODS - OUTPUT
 **************************************/

const WantParser &
WantParser::print( const std::string & fn ) const {

	/* Open the file. */
	std::filebuf fb;
	auto fb_ptr = fb.open(fn, std::ios::out);

	/* Check if failed */
	if ( fb_ptr == NULL ) {
		throw std::runtime_error("Failed to open "
				+ fn);
	}

	/* Print the want-file. */
	std::ostream os(&fb);
	try {
		this->print(os);
	} catch ( const std::exception & e ) {
		/* If any exception is caught, close the file first.
		 * Then re-throw. */
		fb.close();
		throw;
	}

	/* Close the file. */
	fb.close();
	return *this;
}

const WantParser &
WantParser::print( std::ostream &os ) const {

	// Print Nodes in LGF file
	os << "@nodes"
		<< std::endl
		<< "label" << "\t"
		<< "item" << "\t"
		<< "official_name" << "\t"
		<< "username" << "\t"
		<< "dummy"
		<< std::endl;

	for ( auto const & node : _node_map ) {

		// Skip if want-list is missing
		if ( node.second.has_wantlist ) {

			const std::string
				&item = node.second.item,
				&official_name = node.second.official_name,
				&username = node.second.username;
			bool dummy = node.second.dummy;

			os << item << "\t"	// item also used as label
				<< item << "\t"
				<< official_name << "\t"
				<< username << "\t"
				<< dummy << "\t"
				<< std::endl;
		}
	}

	// Print Arcs in LGF file
	os << "@arcs"
		<< std:: endl
		<< "\t" << "\t"
		<< "rank" << "\t"
		<< std::endl;

	for ( auto const & it : _arc_map ) {

		auto const & arc_vector = it.second;

		for ( auto const & arc : arc_vector ) {

			auto const & target = arc.item_t;

			/* Valid if:
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

const WantParser &
WantParser::printErrors( std::ostream & os ) const {

	/* Print the preliminary line 'ERRORS'
	 * only if there are any actual errors to report. */
	if ( ! this->errors_.empty() ) {
		os << "ERRORS" << std::endl;
		for ( auto const & err : this->errors_ ) {
			os << "**** " << err << std::endl;
		}
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
WantParser::parseFile( const std::string & fn ) {

	/* Open the file. */
	std::filebuf fb;
	auto fb_ptr = fb.open(fn, std::ios::in);

	/* Check if failed */
	if ( fb_ptr == NULL ) {
		throw std::runtime_error("Failed to open "
				+ fn);
	}

	/* Parse the want-file. */
	std::istream is(&fb);
	try {
		this->parseStream(is);
	} catch ( const std::exception & e ) {
		/* If any exception is caught, close the file first.
		 * Then re-throw. */
		fb.close();
		throw;
	}

	/* Close the file. */
	fb.close();
}

void
WantParser::parseStream( std::istream & is ) {

	/* We will read line-by-line.
	 * Allocate a buffer to read. */
	const size_t BUFSIZE = (1<<10);
	std::string buffer;
	buffer.reserve(BUFSIZE);

	/* The line number. */
	uint64_t line_n = 0;

	/* Repeat for every line
	 * until the end of the stream. */
	while (std::getline( is, buffer )) {

		/* Increase line number;
		 * useful to document the line number if it throws an error. */
		++ line_n;
		try {
			/* Parse the individual line. */
			this->parseLine_( buffer );

		} catch ( const std::runtime_error & e ) {

			/* Add the exception text to the error list.
			 * Continue with the next line. */
			this->errors_.push_back( std::to_string(line_n)
					+ ":"
					+ e.what() );
		}
	}
}

void
WantParser::parseLine_( const std::string & buffer ) {

	/* Parse line by content:
	 * - Empty lines
	 * - Options: "#!"
	 * - Other directives
	 * - Comments
	 * - Items
	 */
	if ( buffer.empty() ) {

		/* Empty line; do nothing */

	} else if ( buffer.compare(0, 7, "#pragma") == 0 ) {

		/* No current implementation for #pragma */

	} else if ( buffer.compare(0, 2, "#!") == 0 ) {

		/* Option line;
		 * May only be given during initialization.
		 * Isolate option (exlude "#!") and parse it. */
		switch ( _status ) {
			case INITIALIZATION: {
				const std::string option =
					buffer.substr(2, std::string::npos);
				parseOption_( option );
				break;
			}
			default: {
				throw std::runtime_error("Options can only"
						" be given at the beginning"
						" of the file");
				break;
			}
		}

	} else if ( buffer.front() == '#' ) {

		/* Comment line; do not parse.
		 * Any directives beginning with '#' should be parsed
		 * before this point. */

	} else if ( buffer.compare(0, 1, "!") == 0 ) {

		/* Directive. Change the WantParser status
		 * based on the given directive. */
		if ( buffer.compare(0, 21, "!BEGIN-OFFICIAL-NAMES") == 0 ) {

			/* Directive indicating that official names are following. */
			switch ( _status ) {
				case INITIALIZATION: {
					/* Initialization: official names will
					 * be parsed now. */
					_status = PARSE_NAMES;
					break;
				}

				case PARSE_NAMES: {
					throw std::runtime_error("Official names"
							" are already being given");
					break;
				}

				case PARSE_WANTS_WITHNAMES: {
					throw std::runtime_error("Official names"
							" have already been given");
					break;
				}

				default: {
					throw std::runtime_error("Official names"
							" can only be declared"
							" before the want lists");
					break;
				}
			}

		} else if ( buffer.compare(0, 19, "!END-OFFICIAL-NAMES") == 0 ) {\

			/* Directive indicating that official names have ended. */
			//TODO check if it's given multiple times
			_status = PARSE_WANTS_WITHNAMES;

		} else {
			throw std::runtime_error("Unrecognized directive: "
					+ buffer);
		}
	} else {
		/* This line contains something else to be parsed.
		 * This is the default option and should be handled last.
		 * Use appropriate handler for current status.
		 * Expecting to parse:
		 * 	- Official name
		 * 	- Item want list
		 */

		/* If we are already in the initialization,
		 * mark that no official names will be given. */
		if ( this->_status == INITIALIZATION ) {
			this->_status = PARSE_WANTS_NONAMES;
		}

		/* Parsing items or names? */
		switch ( _status ) {
			case PARSE_WANTS_NONAMES:
			case PARSE_WANTS_WITHNAMES:
				_parseWantList( buffer );
				break;

			case PARSE_NAMES:
				parseOfficialName_( buffer );
				break;

			default:
				throw std::logic_error("Unknown handler for"
						" internal status "
						+ std::to_string(_status));
				break;
		}
	}
}

void
WantParser::parseOption_( const std::string & option_line ) {

	/* Tokenize line via regex: ignore whitespaces.
	 * Multiple options may be present in the same line. */
	static const std::regex e(R"(\S+)");
	auto tokens = BaseParser::_split( option_line, e );

	/* Handle option according to type in order:
	 * - Integer
	 * - Priorities
	 * - String
	 */
	for ( auto const & option : tokens ) {

		/* Add to given options list. */
		this->_given_options.push_back( option );

		/* Regexes to match:
		 * - Integer options
		 * - Booleans (no value)
		 *   	- Priorities
		 *   	- Everything else
		 */
		static const std::regex
			regex_int(R"(\b\S+=[-+]?\d+)"),		/* OPTION-ONE=42 or -42 or +42 */
			regex_prio(R"(\b([^-])+-PRIORITIES)");	/* XXX-PRIORITIES */

		if ( std::regex_match(option, regex_int) ) {
			/* Option with an INTEGER value.
			 * Accepted format:
			 * 	OPTION-ONE=42
			 */

			/* Tokenize around '='.
			 * Isolate the variable name and any integer values. */
			static const std::regex digits(R"(([-+]?\d+)|(\b([^=])+))");
			auto const int_elems = BaseParser::_split( option, digits );

			/* Int option; tokenize to retrieve name value.
			 * Check whether the option has been tokenized
			 * and whether the integer value is present. */
			if ( int_elems.empty() ) {
				throw std::logic_error("Regex to tokenize integer-value option has failed.");
			} else if ( int_elems.size() < 2 ) {
				throw std::runtime_error("Value for integer option "
						+ int_elems.at(0) + " not found");
			}

			/* First element: name of int option.
			 * Second element: value of int option. */
			const std::string int_option_name = int_elems.at(0);
			const std::string &value = int_elems.at(1);

			/* Get int option from map, if supported. */
			auto const it = _int_option_map.find( int_option_name );
			if ( it == _int_option_map.end() ) {
				throw std::runtime_error("Unknown integer option "
						+ int_option_name);
			}

			/* Set the value of the int option. */
			const IntOption int_option = it->second;
			_int_options[ int_option ] = std::stoi(value);

		} else if ( std::regex_match(option, regex_prio) ) {
			/* Boolean value indicating the priority scheme.
			 * Must have the form of ("XXXX-PRIORITY").
			 * Do not check if supported.
			 * If given multiple times, the last value is considered.
			 * TODO throw if already given
			 */
			_priority_scheme = option;

		} else {
			/* Finally, any other option without a value
			 * is considered to be a boolean option.
			 * Look up the option in the map to see if it's supported. */
			auto const it = _bool_option_map.find( option );
			if ( it != _bool_option_map.end() ) {

				/* Option is supported. */
				const BoolOption bool_option = it->second;
				_bool_options[ bool_option ] = true;

			} else {
				/* Option not supported. */
				throw std::runtime_error("Unknown option " + option);
			}
		}
	}
}

void
WantParser::parseOfficialName_( const std::string & line ) {

	/* Regular expression to separate fields of official name.
	 * Example of an official name line:
	 *
	 * 	0042-PUERTO ==> "Puerto Rico" (from username) [copy 1 of 2]
	 *
	 *	$1	0042-PUERTO
	 * 	$2	==>
	 * 	$3	"Puerto Rico"
	 * 	$4	(from username)
	 * 	$5	[copy 1 of 2]
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

	/* Tokenize the line. */
	auto match = BaseParser::_split( line, FPAT_names );

	/* Sanity check for minimum number of matches
	 * TODO the description (4th item) is optional. */
	if ( match.size() < 4 ) {
		throw std::runtime_error("Bad format of official name line");
	}

	/* Item name: to be used as a hash key. */
	const std::string
		&orig_item = match[0],
		&orig_official_name = match[2],
		&from_username = match[3];

	/* Parse item name (quotation marks, uppercase).
	 * As we're not providing a username,
	 * it will raise an error if it's dummy. */
	std::string item( orig_item );
	_parseItemName( item );

	/* Replace nested quotation marks in official_name with "'".
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

	/* From_username is in format:
	 * 	(from user name)
	 * Change to:
	 * 	"user name"
	 * TODO it might not be a username;
	 * match with "(from xxx)" and ignore otherwise.
	 */
	std::string username;
	try {
		username = from_username.substr(6, std::string::npos); /* remove "(from " */
	} catch ( const std::out_of_range & e ) {
		throw std::runtime_error("Out of range when parsing username: "
			+ from_username
			+ ": "
			+ e.what()
			);
	}

	/* Remove last ')' from username, if not empty. */
	if ( !username.empty() ) {
		username.pop_back(); /* remove last ')' */
	}
	BaseParser::_parseUsername( username ); /* add quotation marks, make uppercase */

	/* Emplace the item. It should not exist in the node_map.
	 * Throw if present. */
	const auto rv = this->_node_map.emplace( item, _Node_s(item,official_name,username) );
	if ( !rv.second ) {
		throw std::runtime_error("Existing entry for item "
				+ item);
	}
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
	auto const match = BaseParser::_split( line, FPAT_want );
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
	BaseParser::_parseUsername( username );

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
					 * If an entry is found, then it should have a want-list.
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
		 * Check if it has a want-list
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
		BaseParser::_toUpper( item );
	}

	/**
	 * Enclose in quotation marks
	 */
	BaseParser::_quotationMarks(item);

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
	{"ALLOW-DUMMIES",	ALLOW_DUMMIES},		/*!< allow dummy items */
	{"CASE_SENSITIVE",	CASE_SENSITIVE},	/*!< want-lists are case-sensitive */
	{"HIDE-ERRORS",		HIDE_ERRORS},		/*!< WantParser will suppress any errors */
	{"HIDE-LOOPS",		HIDE_LOOPS},		/*!< MathTrader will not report the trade loops */
	{"HIDE-NONTRADES",	HIDE_NONTRADES},	/*!< MathTrader will not report the non-traded items */
	{"HIDE-REPEATS",	HIDE_REPEATS},		/*!< WantParser will not report repeated items */
	{"HIDE-STATS",		HIDE_STATS},		/*!< MathTrader will not report item stats */
	{"HIDE-SUMMARY",	HIDE_SUMMARY},		/*!< MathTrader will not print the final summary */
	{"REQUIRE-COLONS",	REQUIRE_COLONS},	/*!< WantParser requires colons after usernames */
	{"REQUIRE-USERNAMES",	REQUIRE_USERNAMES},	/*!< WantParser requires usernames to be given in want-lists */
	{"SHOW-ELAPSED-TIME",	SHOW_ELAPSED_TIME},	/*!< Total elapsed time will be appended at the end */
	{"SHOW-MISSING",	SHOW_MISSING},		/*!< WantParser will report missing items */
	{"SORT-BY-ITEM",	SORT_BY_ITEM},		/*!< MathTrader will summarize the items by item name, instead of username */
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
