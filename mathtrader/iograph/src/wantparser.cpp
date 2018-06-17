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
#include <iograph/wantparser.hpp>


/**************************************
 * 	PUBLIC METHODS - CONSTRUCTORS
 **************************************/

WantParser::WantParser() {
	int_options_[SMALL_STEP] = 1;
	int_options_[BIG_STEP] = 9;
	int_options_[NONTRADE_COST] = 1e9;
}

/**************************************
 * 	PRIVATE METHODS - PARSING
 **************************************/

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
		switch ( this->status_ ) {
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
			switch ( this->status_ ) {
				case INITIALIZATION: {
					/* Initialization: official names will
					 * be parsed now. */
					this->status_ = PARSE_NAMES;
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

		} else if ( buffer.compare(0, 19, "!END-OFFICIAL-NAMES") == 0 ) {

			/* Directive indicating that official names have ended. */
			//TODO check if it's given multiple times
			this->status_ = PARSE_WANTS_WITHNAMES;

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
		if ( this->status_ == INITIALIZATION ) {
			this->status_ = PARSE_WANTS_NONAMES;
		}

		/* Parsing items or names? */
		switch ( this->status_ ) {
			case PARSE_WANTS_NONAMES:
			case PARSE_WANTS_WITHNAMES:
				parseWantList_( buffer );
				break;

			case PARSE_NAMES:
				parseOfficialName_( buffer );
				break;

			default:
				throw std::logic_error("Unknown handler for"
						" internal status "
						+ std::to_string(this->status_));
				break;
		}
	}
}

void
WantParser::parseOption_( const std::string & option_line ) {

	/* Tokenize line via regex: ignore whitespaces.
	 * Multiple options may be present in the same line. */
	static const std::regex e(R"(\S+)");
	auto tokens = split_( option_line, e );

	/* Handle option according to type in order:
	 * - Integer
	 * - Priorities
	 * - String
	 */
	for ( auto const & option : tokens ) {

		/* Add to given options list. */
		this->given_options_.push_back( option );

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
			auto const int_elems = split_( option, digits );

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
			auto const it = int_option_map_.find( int_option_name );
			if ( it == int_option_map_.end() ) {
				throw std::runtime_error("Unknown integer option "
						+ int_option_name);
			}

			/* Set the value of the int option. */
			const IntOption_ int_option = it->second;
			int_options_[ int_option ] = std::stoi(value);

		} else if ( std::regex_match(option, regex_prio) ) {
			/* Boolean value indicating the priority scheme.
			 * Must have the form of ("XXXX-PRIORITY").
			 * Do not check if supported.
			 * If given multiple times, the last value is considered.
			 * TODO throw if already given
			 */
			priority_scheme_ = option;

		} else {
			/* Finally, any other option without a value
			 * is considered to be a boolean option.
			 * Look up the option in the map to see if it's supported. */
			auto const it = bool_option_map_.find( option );
			if ( it != bool_option_map_.end() ) {

				/* Option is supported. */
				const BoolOption_ bool_option = it->second;
				this->bool_options_[ bool_option ] = true;

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
	auto match = split_( line, FPAT_names );

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
	const std::string item = convertItemName_( orig_item );

	/* Replace nested quotation marks in official_name with "'".
	 * Replace backslashes with forward slashes;
	 * we are not going to escape them.
	 * Ignore the first and the last position of the string.
	 * TODO create static private method.
	 */
	std::string official_name( orig_official_name );

	/* Remove quotations.
	 * std::remove pushes all quotes to the end
	 * and erase() removes them.
	 * Alternatively, replace double quotes with single quotes.
	 */
	auto remove_func = []( std::string & str, char c ) {
		str.erase(
				std::remove(str.begin(),str.end(),c),
				str.end()
				);
	};
	remove_func( official_name, '"');

	/* Replace backslashes '\' with slashes '/'. */
	std::replace(official_name.begin(), official_name.end(),
			'\\', '/');

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

	/* Capitalize. */
	if ( !this->bool_options_[CASE_SENSITIVE] ) {
		std::transform(username.begin(), username.end(), username.begin(), ::toupper);
	}

	/* Add the item to node_map_. */
	this->addSourceItem_( item, official_name, username );
}

/**************************************
 * 	PRIVATE METHODS - UTILS
 **************************************/

std::string
WantParser::extractUsername_( const std::string & token ) {

	/* Username to extract; empty if nothing is extracted. */
	std::string username;
	username.clear();

	if ( !token.empty() ) {

		/* Check username validity;
		 * first & last characters should be '(' and ')'. */
		const bool is_username = (( token.front() == '(' )
				&& ( token.back() == ')' ));

		if ( is_username ) {
			username = token.substr(1, token.size()-2);
		}
	}
	return username;
}

std::string
WantParser::convertItemName_( const std::string & item,
		const std::string username ) const {

	/* Target item name */
	std::string target(item);

	/* Handle cases where item is dummy */
	if ( this->isDummy_(item) ) {

		/* Only proceed if dummy names are allowed. */
		if ( !this->bool_options_[ALLOW_DUMMIES] ) {

			throw std::runtime_error("Dummy item "
					+ item
					+ " detected, but dummy items"
					" not allowed");

		} else if ( username.empty() ) {

			/* Usernames MUST be present when giving a dummy item. */
			throw std::runtime_error("Dummy item "
					+ item
					+ " detected, but username "
					" not defined");
		}

		/* Append username to dummy name. */
		target.append("-(");
		target.append(username);
		target.push_back(')');
	}

	/* Unless item names are case sensitive,
	 * convert to uppercase. */
	if ( !this->bool_options_[CASE_SENSITIVE] ) {
		std::transform(target.begin(), target.end(), target.begin(), ::toupper);
	}

	return target;
}

/***************************************
 * 	PRIVATE STATIC METHODS
 **************************************/

bool
WantParser::isDummy_( const std::string & item ) {

	/* Empty string? */
	if ( item.empty() ) {
		return false;
	}

	/* Dummy if first character is '%'. */
	return ( item.front() == '%' );
}

std::vector<std::string>
WantParser::split_( const std::string & input, const std::string & str ) {
	std::regex regex(str);
	return split_( input, regex );
}

std::vector< std::string >
WantParser::split_( const std::string & input, const std::regex & regex ) {
	std::sregex_token_iterator
		first{input.begin(), input.end(), regex, 0},
		last;
	return {first, last};
}


/****************************************
 * 	PRIVATE STATIC MEMBERS		*
 ****************************************/

const std::unordered_map< std::string, WantParser::BoolOption_ >
WantParser::bool_option_map_ = {
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

const std::unordered_map< std::string, WantParser::IntOption_ >
WantParser::int_option_map_ = {
	{"SMALL-STEP",		SMALL_STEP},
	{"BIG-STEP",		BIG_STEP},
	{"NONTRADE_COST",	NONTRADE_COST},
};
