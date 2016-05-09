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


	while (std::getline( is, buffer )) {

		/**
		 * Parse line by content:
		 * - Empty lines
		 * - Options: "#!"
		 * - Other directives
		 * - Comments
		 * - Items
		 */
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
					throw std::runtime_error("Unknown handler for status "
							+ std::to_string(_status));
					break;
			}
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

	os << "Options: ";
	for ( auto it : _int_options ) {
	}

	return *this;
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
	 * Append quotation marks to item name,
	 * if not already there.
	 */
	std::string item = _quotationMarks( orig_item );

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
	 */
	std::string username =
		from_username.substr(6, std::string::npos); /**< remove "(from " */
	username.pop_back(); /**< remove last ')' */
	username = _quotationMarks( username );

	/**
	 * Emplace official name.
	 * Emplace username.
	 * May override previous entry, if already present.
	 */
	auto rv = this->_node_map.emplace( item, _Node_s(item,official_name,username) );

	/**
	 * Throw if already there.
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
	 * Tokenize the line
	 */
	auto match = _split( line );

	/**
	 * Sanity check for minimum number of matches
	 */
	if (( match.size() < 3 ) || ( match[2].compare(":") != 0 )) {
		throw std::runtime_error("Bad format of wantlist line:"
				+ line);
	}

	/**
	 * Sample line:
	 * (user name) ITEM_A : ITEM_B ITEM_C ; ITEM_D %DUMMY1 %DUMMY2 ITEM_E
	 */
	const std::string
		&username_p = match[0],
		&source = match[1];

	/**
	 * Append username on item name, if dummy.
	 */
	std::string item = _appendIfDummy( source, username_p );

	/**
	 * Remove parentheses from username: first and last character.
	 * Add quotation marks.
	 */
	std::string username = username_p.substr(1, std::string::npos);
	username.pop_back();
	username = _quotationMarks(username);

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
	 */
	const int max_arcs = match.size() - 3;
	_arc_map[item].reserve( max_arcs );

	/**
	 * Initialize rank
	 */
	int rank = 1;

	/**
	 * Parse every single wanted item.
	 */
	for ( unsigned i = 3; i < match.size(); ++ i ) {

		auto const & small_step = _int_options[SMALL_STEP];
		auto const & big_step   = _int_options[BIG_STEP];

		std::string target = match[i];

		/**
		 * Cases:
		 * 1. Semicolon:
		 * 	"increase the rank of the next item by the big-step value"
		 * 	NOTE: the small-step of the previous item will also be applied.
		 * 2. Missing items: ignore them.
		 * 3. Actual wanted item.
		 */
		if ( target.compare(";") == 0 ) {
			rank += big_step;
		} else {

			/**
			 * Append username at target, if dummy.
			 */
			target = _appendIfDummy( target, username_p );

			/**
			 * Push target to map.
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

std::string
WantParser::_appendIfDummy( const std::string & orig_item,
		const std::string & username ) {

	std::string item( orig_item );

	/**
	 * Strip closing quotation mark, if there.
	 */
	if ( item.back() == '"' ) {
		item.pop_back();
	}

	/**
	 * Append username, if needed.
	 */
	if ( _dummy( item ) ) {
		item.push_back('-');
		item.append(username);
	}

	/**
	 * Append quotation markes.
	 */
	item = _quotationMarks(item);

	/**
	 * Convert to uppercase.
	 */
	std::transform(item.begin(), item.end(), item.begin(), ::toupper);

	return item;
}

std::string
WantParser::_quotationMarks( const std::string & str ) {

	std::string item( str );

	/**
	 * Add leading quotation mark, if needed.
	 */
	if ( item.front() != '"' ) {
		item = "\"" + item;
	}

	/**
	 * Add closing quotation mark
	 */
	item.push_back('"');

	/**
	 * Convert to uppercase.
	 */
	std::transform(item.begin(), item.end(), item.begin(), ::toupper);

	return item;
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
 */
const std::regex
WantParser::_FPAT(
	"(\\S+)"		// Group 1: any non-whitespace
	"|"
	"(\""			// Group 2: opening quotation mark
		"("
		"[^\"]"			// Subgroup 2.1:
					// anything not a quotation mark
		"|"
		"(\"[^\"]*\")"		// Subgroup 2.2:
					// two nested quotation marks
					// with any non-quotation mark
					// character between them
		")*"
	"\")"			// Group 2: closing quotation mark
	"|"
	"(\\([^\\)]+\\))"	// Group 3: parentheses
	"|"
	"(\\[[^\\[\\]]+\\])"	// Group 4: brackets
);
