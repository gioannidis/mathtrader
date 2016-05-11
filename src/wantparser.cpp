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
#include "wantparser.hpp"

#include <regex>
#include <stdexcept>


/************************************//*
 * 	PUBLIC METHODS - CONSTRUCTORS
 **************************************/

WantParser::WantParser() :
	_bool_options( MAX_BOOL_OPTIONS, false ),
	_int_options( MAX_INT_OPTIONS ),
	_priority_scheme( "" ),
	_status( PARSE_WANTS )
{
	/**
	 * Default integer options.
	 */
	_int_options[SMALL_STEP] = 1;
	_int_options[BIG_STEP] = 9;
	_int_options[NONTRADE_COST] = 1e9;
}

WantParser::~WantParser() {

	/**
	 * Close file stream, if applicable.
	 */
	if ( _fs.is_open() ) {
		_fs.close();
	}
}


/************************************//*
 * 	PUBLIC METHODS - INPUT
 **************************************/
	
WantParser &
WantParser::wantlist( const std::string & fn ) {

	/**
	 * Open file for reading
	 */
	_fs.open(fn, std::ios_base::in);
	if ( _fs.fail() ) {
		throw std::runtime_error("Could not open file "
				+ fn + " for reading");
	}
	return *this;
}


/************************************//*
 * 	PUBLIC METHODS - PARSING
 **************************************/

void
WantParser::parse() {

	/**
	 * Read buffer
	 */
	const size_t BUFSIZE = (1<<10);
	std::string buffer;
	buffer.reserve(BUFSIZE);

	/**
	 * Input stream:
	 * Given file, if applicable.
	 * Else, std::cin.
	 */
	std::istream & is = (_fs.is_open()) ? _fs : std::cin;

	uint64_t line_n = 0;
	while (std::getline( is, buffer )) {

		++ line_n;
		/**
		 * Parse line by content:
		 * - Empty lines
		 * - Options: "#!"
		 * - Other directives
		 * - Comments
		 * - Items
		 */
		try {
			if ( buffer.empty() ) {

				/**
				 * Empty line; do nothing.
				 */

			} else if ( buffer.compare(0, 2, "#!") == 0 ) {

				/**
				 * Option line;
				 * isolate option (exlude "#!") and parse it.
				 */
				const std::string option = buffer.substr(2, std::string::npos);
				_parseOption( option );

			} else if ( buffer.compare(0, 7, "#pragma") == 0 ) {

				/**
				 * No current implementation for #pragma
				 */

			} else if ( buffer.compare(0, 1, "#") == 0 ) {

				/**
				 * Comment line; do nothing.
				 * NOTE: parsing any directive beginning with
				 * "#" should go before this.
				 */

			} else if ( buffer.compare(0, 1, "!") == 0 ) {

				/**
				 * Directives.
				 */
				if ( buffer.compare(0, 21, "!BEGIN-OFFICIAL-NAMES") == 0 ) {

					_status = PARSE_NAMES;
					continue ;

				} else if ( buffer.compare(0, 19, "!END-OFFICIAL-NAMES") == 0 ) {\

					_status = PARSE_WANTS;
					continue ;

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
					case PARSE_NAMES:
						_parseOfficialName( buffer );
						break;
					case PARSE_WANTS:
						_parseWantList( buffer );
						break;
					default:
						throw std::logic_error("Unknown handler for"
								" internal status "
								+ std::to_string(_status));
						break;
				}
			}

		} catch ( const std::runtime_error & e ) {

			/**
			 * Add the exception text to the error list.
			 * Continue with the next line.
			 */
			this->_errors.push_back( std::to_string(line_n)
					+ ":"
					+ e.what() );
		}
	}

	/**
	 * Mark unknown items
	 */
	_markUnknownItems();
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

			/**
			 * Skip unknown items.
			 */
			if ( !arc.unknown ) {
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
WantParser::print( const std::string & fn ) const {

	std::filebuf fb;
	fb.open(fn, std::ios::out);
	std::ostream os(&fb);
	print(os);
	fb.close();
	return *this;
}

const WantParser &
WantParser::showOptions( std::ostream & os ) const {

	/**
	 * TODO: add int options if not default?
	 * Add stack with given options?
	 */

	os << "Options: ";
	for ( auto const it : _bool_option_map ) {
		if ( _bool_options[ it.second ] ) {
			os << it.first << " ";
		}
	}
	os << _priority_scheme;
	os << std::endl;

	return *this;
}

const WantParser &
WantParser::showErrors( std::ostream & os ) const {
	os << "ERRORS" << std::endl;
	for ( auto const & err : _errors ) {
		os << "**** " << err << std::endl;
	}
	return *this;
}


/************************************//*
 * 	PUBLIC METHODS - OPTIONS OUTPUT
 **************************************/

std::string
WantParser::getPriorityScheme() const {
	return _priority_scheme;
}


/************************************//*
 * 	PUBLIC METHODS - UTILITIES
 **************************************/

WantParser &
WantParser::clear() {

	_status = PARSE_WANTS;
	_node_map.clear();
	_arc_map.clear();
	_unknown_item_map.clear();
	_errors.clear();

	if ( _fs.is_open() ) {
		_fs.close();
	}

	return *this;
}


/************************************//*
 * 	PRIVATE METHODS - PARSING
 **************************************/

WantParser &
WantParser::_parseOption( const std::string & option_line ) {

	/**
	 * Tokenize option: ignore spaces or '='
	 */
	static const std::regex e("\\b([^\\s=]+)");
	auto elems = _split( option_line, e );

	/**
	 * Verify that we've found an option;
	 * ignore if empty.
	 */
	if ( elems.empty() ) {
		return *this;
	}


	/**
	 * Handle option according to type:
	 * - String
	 * - Integer
	 * - Priorities
	 */
	const std::string & option = elems[0];
	static const std::regex regex_prio("\\b([^-])*-PRIORITIES");

	if ( _bool_option_map.find(option) != _bool_option_map.end() ) {

		/**
		 * String option; set to true.
		 */
		auto const it = _bool_option_map.find(option);
		const BoolOption bool_option = it->second;
		_bool_options[ bool_option ] = true;

	} else if ( _int_option_map.find(option) != _int_option_map.end() ) {

		/**
		 * Int option; retrieve value.
		 * Accepted formats:
		 * 	#! VARIABLE = 42
		 * 	#! VARIABLE=42
		 * 	#! VARIABLE 42
		 */
		if ( elems.size() < 2 ) {
			throw std::runtime_error("Value for option"
					+ option + " not found");
		}
		const std::string &value = elems[1];

		/**
		 * Get int option and set it
		 */
		auto const it = _int_option_map.find(option);
		const IntOption int_option = it->second;

		_int_options[ int_option ] = std::stoi(value);

	} else if ( std::regex_match(option, regex_prio) ) {

		/**
		 * Priority scheme. Must have the form of ("XXXX-PRIORITY").
		 * Do not check its validity,
		 * that's the responsibility of MathTrader.
		 */
		_priority_scheme = option;

	} else {
		throw std::runtime_error("Unknown option " + option);
	}

	return *this;
}

WantParser &
WantParser::_parseOfficialName( const std::string & line ) {

	/**
	 * Tokenize the line
	 */
	auto match = _split( line );

	/**
	 * Sanity check for minimum number of matches
	 * TODO the description (4th item) is optional.
	 */
	if ( match.size() < 4 ) {
		throw std::runtime_error("Bad format of official name line:"
				+ line);
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
	 * Ignore the first and the last position of the string.
	 */
	std::string official_name( orig_official_name );
	size_t pos = 1;
	while ((( pos = official_name.find( "\"", pos )) != std::string::npos )
			&& ( pos != official_name.length()-1 )) {

		official_name.replace( pos, 1, "'");
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
	} catch ( std::out_of_range & e ) {
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
	} catch ( std::exception & e ) {
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
	auto const match = _split( line );
	if ( match.empty() ) {
		throw std::runtime_error("Bad format of want list: "
				+ line);
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
		throw std::runtime_error("Missing username from want list: "
				+ line);
	}


	/**********************************//*
	 *	OFFERING ITEM NAME (source)
	 *************************************/

	/**
	 * Check whether there is a wanted item name.
	 */
	if ( match.size() < (n_pos + 1) ) {
		throw std::runtime_error("Missing offering item from want list: "
				+ line);
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
	 * Insert node if not already present.
	 * Normally, non-dummy nodes will have already been inserted
	 * if the official names are given.
	 * Item name is used as official name.
	 */
	if ( _node_map.find(item) == _node_map.end() ) {
		this->_node_map.emplace( item, _Node_s(item,item,username,_dummy(item)) );
	}

	/**
	 * Finally, advance n_pos.
	 * We should always have an offering item.
	 */
	++ n_pos;


	/**********************************//*
	 *	CHECK COLONS
	 *************************************/

	bool has_colon = ((match.size() >= (n_pos + 1)) && (match[n_pos].compare(":") == 0));

	/**
	 * Advance n_pos if we have a colon
	 */
	if ( has_colon ) {
		++ n_pos;
	} else if ( _bool_options[REQUIRE_COLONS] ) {
		throw std::runtime_error("Missing colon from want list: "
				+ line);
	}


	/**********************************//*
	 *	WANTED ITEMS (targets)
	 *************************************/

	/**
	 * Create ArcMap entry with empty vector.
	 */
	auto rv_a = _arc_map.emplace( item, std::vector< _Arc_t >() );

	/**
	 * Failed to emplace arc list?
	 */
	if ( !rv_a.second ) {
		throw std::runtime_error("Want list for item "
				+ item + " already exists");
	}

	/**
	 * Optimization: reserve adequate space.
	 * The first n_pos elems were not wanted items.
	 */
	const unsigned first_wanted_pos = n_pos;
	const unsigned max_arcs = match.size() - first_wanted_pos;
	_arc_map[item].reserve( max_arcs );

	/**
	 * Initialize rank
	 */
	int rank = 1;


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
			throw std::runtime_error("Wrong position for semicolon in want list: "
					+ line);
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
			_arc_map[item].push_back(_Arc_t( item, target, rank ));
		}

		/**
		 * Advance always the rank by small-step
		 */
		rank += small_step;
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

WantParser &
WantParser::_parseUsername( std::string & username ) {

	/**
	 * Convert to uppercase.
	 * Append quotation marks.
	 */
	_toUpper( username );
	_quotationMarks( username );

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
			const std::string & item = arc.item_t;
			if ( _node_map.find(item) == _node_map.end() ) {
				arc.unknown = true;

				auto it = _unknown_item_map.find(item);
				if ( it != _unknown_item_map.end()) {
					it->second ++ ;
				} else {
					_unknown_item_map.emplace(item,1);
				}
			}
		}
	}

	return *this;
}

std::vector<std::string>
WantParser::_split( const std::string & input, const std::string & regex ) {

	std::regex re(regex);
	return _split( input, re );
}

std::vector<std::string>
WantParser::_split( const std::string & input, const std::regex & regex ) {

	std::sregex_token_iterator
		first{input.begin(), input.end(), regex, 0},
		last;

	return {first, last};
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

void
WantParser::_quotationMarks( std::string & str ) {

	/**
	 * Add leading quotation mark, if needed.
	 */
	if ( str.front() != '"' ) {
		str = "\"" + str;
	}

	/**
	 * Add closing quotation mark
	 */
	str.push_back('"');
}

void
WantParser::_toUpper( std::string & str ) {
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}


/************************************//*
 * 	PRIVATE STATIC MEMBERS
 **************************************/

/**
 * Unordered_map to map string-options to enums.
 */
const std::unordered_map< std::string, WantParser::BoolOption  >
WantParser::_bool_option_map = {
	{"ALLOW-DUMMIES",	ALLOW_DUMMIES},
	{"REQUIRE-COLONS",	REQUIRE_COLONS},
	{"REQUIRE-USERNAMES",	REQUIRE_USERNAMES},
	{"HIDE-LOOPS",		HIDE_LOOPS},
	{"HIDE-SUMMARY",	HIDE_SUMMARY},
	{"HIDE-NONTRADES",	HIDE_NONTRADES},
	{"HIDE-ERRORS",		HIDE_ERRORS},
	{"HIDE-REPEATS",	HIDE_REPEATS},
	{"HIDE-STATS",		HIDE_STATS},
	{"SORT-BY-ITEM",	SORT_BY_ITEM},
	{"HIDE-REPEATS",	CASE_SENSITIVE},
	{"SHOW-MISSING",	SHOW_MISSING},
	{"SHOW-ELAPSED-TIME",	SHOW_ELAPSED_TIME},
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
const std::regex
WantParser::_FPAT(
	R"(\"([\"]|[^\"])+\")"	// Group Q: quotation mark pairs
				// with possible quotation marks included.
				// This finds the longest stream of quotation marks.
				// TODO If usernames or descriptions have
				// quotation marks, we will have a problem here.
	"|"
	R"(\([^\)]+\))"		// Group P: parentheses
	"|"
	R"(\[[^\[\]]+\])"	// Group B: brackets
	"|"
	R"(\S+)"		// Group W: any non-whitespace
);
