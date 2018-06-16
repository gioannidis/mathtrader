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

#include <fstream>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>


/************************************//*
 * 	PUBLIC METHODS - CONSTRUCTORS
 **************************************/

WantParser::WantParser() {
	int_options_[SMALL_STEP] = 1;
	int_options_[BIG_STEP] = 9;
	int_options_[NONTRADE_COST] = 1e9;
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

		/* Get references to item details. */
		const std::string
			&item = node.second.item,
			&official_name = node.second.official_name,
			&username = node.second.username;

		/* Check if it has a want-list (present in the arc map). */
		const bool has_wantlist =
			(this->_arc_map.find(item) != this->_arc_map.end());

		/* Skip if it has no want-list. */
		if ( has_wantlist ) {

			const bool dummy = isDummy_(item);

			os << '"' << item << '"'	/* item also used as label */
				<< '\t'
				<< '"' << item << '"'
				<< '\t'
				<< '"' << official_name << '"'
				<< '\t'
				<< '"' << username << '"'
				<< '\t'
				<< dummy
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
			 * 1) Target is in node map.
			 * 2) Target has a want-list.
			 */
			const bool valid =
				(_node_map.find(target) != _node_map.end())	/* valid target node */
				&& (_arc_map.find(target)  != _arc_map.end());	/* target node has a want-list too */

			if ( valid ) {
				os << '"' << arc.item_s << '"'
					<< '\t'
					<< '"' << arc.item_t << '"'
					<< '\t'
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
	for ( auto const option : given_options_ ) {
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

		const bool has_wantlist =
			(this->_arc_map.find(item) != this->_arc_map.end());

		/* Report if want-list is empty and is NOT a dummy item. */
		if ( !this->isDummy_(item) && !has_wantlist ) {
			++ count;
			ss << "**** Missing want list for item "
				<< "\"" << item << "\""
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
	return priority_scheme_;
}

bool
WantParser::hideErrors() const {
	return this->bool_options_[HIDE_ERRORS];
}

bool
WantParser::hideLoops() const {
	return this->bool_options_[HIDE_LOOPS];
}

bool
WantParser::hideNonTrades() const {
	return this->bool_options_[HIDE_NONTRADES];
}

bool
WantParser::hideStats() const {
	return this->bool_options_[HIDE_STATS];
}

bool
WantParser::hideSummary() const {
	return this->bool_options_[HIDE_SUMMARY];
}

bool
WantParser::showElapsedTime() const {
	return this->bool_options_[SHOW_ELAPSED_TIME];
}

bool
WantParser::showMissing() const {
	return this->bool_options_[SHOW_MISSING];
}

bool
WantParser::sortByItem() const {
	return this->bool_options_[SORT_BY_ITEM];
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

		} else if ( buffer.compare(0, 19, "!END-OFFICIAL-NAMES") == 0 ) {\

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

	/* Add the item to _node_map. */
	this->addSourceItem_( item, official_name, username );
}

void
WantParser::parseWantList_( const std::string & line ) {

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

	/* Summary:
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

	/* Tokenize the line. */
	auto const match = split_( line, FPAT_want );
	if ( match.empty() ) {
		throw std::runtime_error("Bad format of want list");
	}

	/********************************
	 * 	PARSE USERNAME		*
	 ********************************/

	unsigned n_pos = 0;	/* current item that is being parsed */

	std::string username = extractUsername_(match.at(n_pos));

	/* Go to the next element if we have a valid username.
	 * If we are missing a required username stop here. */
	if ( !username.empty() ) {
		++ n_pos ;
	} else if (  this->bool_options_[ REQUIRE_USERNAMES ] ) {
		throw std::runtime_error("Missing username from want list");
	}

	/****************************************
	 *	OFFERED ITEM NAME (source)	*
	 ****************************************/

	/* Check whether we have reached the end of the line.
	 * If so, the wanted item name is missing. */
	if ( n_pos >= match.size() ) {
		throw std::runtime_error("Missing offered item from want list");
	}
	const std::string & original_source = match.at(n_pos);

	/* Convert item name. */
	const std::string source = convertItemName_( original_source, username );

	/* Add source item.
	 * Item name is also used as the 'official' name
	 */
	this->addSourceItem_( source, source, username );

	/* Finally, advance n_pos.
	 * We should always have an offering item. */
	++ n_pos;


	/********************************
	 *	CHECK COLONS		*
	 ********************************/

	/* Dependent scope to open local variables. */
	{
		const bool has_colon = (n_pos < match.size())
				&& (match.at(n_pos).compare(":") == 0);

		/* Advance n_pos if we have a colon. */
		if ( has_colon ) {
			++ n_pos;
		} else if ( this->bool_options_[REQUIRE_COLONS] ) {
			throw std::runtime_error("Missing colon from want list");
		}
	}


	/********************************
	 *	WANTED ITEMS (targets)	*
	 ********************************/

	/* Copy wanted items; first begins at n_pos */
	const auto wanted_items = std::vector< std::string >(
			match.begin() + n_pos,
			match.end());

	this->addTargetItems_( source, wanted_items );
}

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

void
WantParser::addSourceItem_( const std::string & source,
		const std::string & official_name,
		const std::string & username) {

	/* Check if the source item is present in node_map. */
	auto it = _node_map.find( source );

	if ( it == _node_map.end() ) {

		/* Source item is NOT in node_map. */
		switch ( this->status_ ) {
			case PARSE_NAMES:
			case PARSE_WANTS_NONAMES: {

				/* We are either:
				 * 1. currently reading official names; OR
				 * 2. currently reading want-lists without
				 *    official names.
				 *
				 * These should be no entries in these cases.
				 * Break here, add later.
				 */
				break;
			}
			case PARSE_WANTS_WITHNAMES: {

				/* We are currently reading want-lists
				 * WITH official names previously given.
				 * There SHOULD be an entry
				 * with a corresponding official name,
				 * unless it's a dummy item.
				 * This check usually catches spelling errors.
				 *
				 * Therefore, raise an error only if it's
				 * a non-dummy item.
				 * Otherwise, proceed to add.
				 */
				if ( !isDummy_(source) ) {

					throw std::runtime_error("Non-dummy item "
							+ source
							+ " has no official name."
							" Hint: spelling error?");
				}
				break;
			}
			default: {
				throw std::logic_error("Unknown handler for internal status "
						+ std::to_string( this->status_ )
						+ "; source item not found in node map.");
				break;
			}
		}

		/* Insert the item in the node_map. */
		auto const pair = this->_node_map.emplace(
				source,
				Node_t_(source, official_name, username)
				);

		/* Insert should have succeeded. */
		if ( !pair.second ) {
			throw std::logic_error("Could not insert node in _node_map.");
		}

	} else {
		/* Source item IS in node_map. */
		switch ( this->status_ ) {
			case PARSE_NAMES: {

				/* We are currently reading official names.
				 * Ignore any item attempted to be inserted twice.
				 */
				throw std::runtime_error("Existing entry for item "
						+ source);
				break;
			}
			case PARSE_WANTS_WITHNAMES:
			case PARSE_WANTS_NONAMES: {

				/* We are currently reading want-lists either:
				 * 1. without official names; OR
				 * 2. with official names.
				 *
				 * If a want-list exists: ignore.
				 * Otherwise:
				 * - If we have official names, do nothing. We had previously inserted the item
				 *   in parseOfficialName_.
				 * - If we do not have official names, it's a logic error.
				 */
				const bool source_has_wantlist =
					(this->_arc_map.find(source) != this->_arc_map.end());

				if ( source_has_wantlist ) {
					/* Condition must be true. */
					throw std::runtime_error("Ignoring multiple wantlist for item "
							+ source);
				} else if ( this->status_ == PARSE_WANTS_NONAMES ) {
					/* Sanity check. If no official names are being read
					 * an existing item MUST have a want-list. */
					throw std::logic_error("Existing item found in node list map "
							"without a want-list, but official names "
							"have not been given.");
				}
				break;
			}
			default: {
				throw std::logic_error("Unknown handler for internal status "
						+ std::to_string( this->status_ )
						+ "; source item already found in node map.");
				break;
			}
		}
	}
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

void
WantParser::addTargetItems_( const std::string & source, const std::vector< std::string > & wanted_items ) {

	/* Check if want list already exists.
	 * This may happen if a user has defined multiple want lists
	 * or another line was split over two lines.
	 */
	if ( _arc_map.find(source) != _arc_map.end() ) {
		throw std::runtime_error("Multiple want lists for item "
				+ source
				+ ". Hint: check if an item want-list line has been split"
				+ " over two lines.");
	}

	/* Initialize rank. */
	int rank = 1;

	/* Initialize list with want-lists to be added.
	 * All parsed want lists are added to this map
	 * and they will be eventually added
	 * to the official graph
	 * if *no errors* whatsoever are detected.
	 * On errors, the whole line is discarded.
	 */
	std::list< Arc_t_ > arcs_to_add;

	/********************************
	 *	WANTED ITEMS ITERATOR	*
	 ********************************/

	for ( const auto & target : wanted_items ) {

		/* Small and big steps. */
		const auto register & small_step = int_options_[SMALL_STEP];
		const auto register & big_step   = int_options_[BIG_STEP];

		/* Cases:
		 * 1. Semicolon:
		 * 	"increase the rank of the next item by the big-step value"
		 * 	NOTE: the small-step of the previous item will also be applied.
		 * 2. Colon: raise an error. There should be no colon here.
		 * 3. Actual wanted item.
		 */
		if ( target.compare(";") == 0 ) {
			rank += big_step;
		} else if ( target.compare(":") == 0 ) {
			throw std::runtime_error("Invalid colon occurence.");
		} else {

			/* Parse the item name (dummy, uppercase, etc). */
			const auto & username = this->_node_map.at( source ).username;
			const auto converted_target_name = convertItemName_( target, username );

			/* Push (item-target) arc to map. */
			arcs_to_add.push_back(Arc_t_( source, converted_target_name, rank ));
		}

		/* Advance always the rank by small-step. */
		rank += small_step;
	}

	/* Create ArcMap entry for item;
	 * in C++11 we can directly move the items from the original list
	 * to the vector; we don't have to copy them!
	 */
	auto pair = _arc_map.emplace(
			source,
			std::vector< Arc_t_ > {
				std::make_move_iterator(std::begin(arcs_to_add)),
				std::make_move_iterator(std::end(arcs_to_add)) }
			);

	/* Insertion should have succeeded. */
	if ( !pair.second ) {
		throw std::logic_error("Could not insert arcs in _arc_map.");
	}
}

/************************************//*
 * 	PRIVATE METHODS - UTILS
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
