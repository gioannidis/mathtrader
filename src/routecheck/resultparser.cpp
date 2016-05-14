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
#include "resultparser.hpp"

#include <regex>
#include <stdexcept>


/************************************//*
 * 	PUBLIC METHODS - CONSTRUCTORS
 **************************************/

ResultParser::ResultParser() :
	BaseParser(),
	_status( BEGIN ),
	_new_loop( true )
{
}

ResultParser::~ResultParser() {
}


/************************************//*
 * 	PUBLIC METHODS - OUTPUT
 **************************************/

const ResultParser &
ResultParser::print( std::ostream &os ) const {

	for ( auto const & item : _item_list ) {
		os << item << std::endl;
	}

	return *this;
}

const ResultParser &
ResultParser::print( const std::string & fn ) const {

	std::filebuf fb;
	fb.open(fn, std::ios::out);
	std::ostream os(&fb);
	print(os);
	fb.close();
	return *this;
}


/************************************//*
 * 	PRIVATE METHODS - PARSING
 **************************************/

void
ResultParser::_parse( const std::string & buffer ) {

	/**
	 * Parse line by content:
	 * - Options: "#!"
	 * - Other directives
	 * - Comments
	 * - Items
	 */
	try {
		if ( buffer.compare(0, 2, "#") == 0 ) {

			/**
			 * Options, comments, etc;
			 * do nothing.
			 */

		} else if ( buffer.compare(0, 1, "!") == 0 ) {

			/**
			 * Directives;
			 * do nothing.
			 */

		} else if ( buffer.compare(0, 11, "TRADE LOOPS") == 0 ) {

			/**
			 * Start parsing trade loops
			 */
			_status = TRADE_LOOPS;

		} else if ( buffer.compare(0, 12, "ITEM SUMMARY") == 0 ) {

			/**
			 * Start parsing item summary
			 */
			_status = ITEM_SUMMARY;

		} else {
			/**
			 * Item to be parsed. This is the default option
			 * and should be handled last.
			 * Use appropriate handler for current status.
			 */
			switch ( _status ) {
				case BEGIN:
				case ITEM_SUMMARY:
					/* Do nothing */
					break;
				case TRADE_LOOPS:
					_parseLoop( buffer );
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

ResultParser &
ResultParser::_postParse() {
	return *this;
}

ResultParser &
ResultParser::_parseLoop( const std::string & line ) {

	static const std::regex FPAT_loop(
		R"(\([^\)]+\))"		// Group 1: parentheses
		"|"
		R"([^\s]+)"		// Group 2: any non-whitespace
	);

	/**
	 * Tokenize the line
	 */
	auto const match = _split( line, FPAT_loop );
	if ( match.empty() ) {
		throw std::runtime_error("Bad format of want list: "
				+ line);
	}

	/**
	 * TODO check if match[0]
	 * is username.
	 */

	const std::string
		& source = match[1],
		& target = match[4];

	if ( _new_loop ) {

		/* New cycle */
		_item_list.push_back( source );
		_first_item = source;
		_new_loop = false;

	} else if ( target.compare(_first_item) == 0 ) {

		/* Cycle end */
		_new_loop = true;
	}

	/* Always add the target */
	_item_list.push_back( target );

	return *this;
}
