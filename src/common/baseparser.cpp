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
#include "baseparser.hpp"

#include <regex>
#include <stdexcept>


/************************************//*
 * 	PUBLIC METHODS - CONSTRUCTORS
 **************************************/

BaseParser::BaseParser() {
}

BaseParser::~BaseParser() {

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
	
BaseParser &
BaseParser::inputFile( const std::string & fn ) {

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
BaseParser::parse( std::istream & is ) {

	/**
	 * Read buffer
	 */
	const size_t BUFSIZE = (1<<10);
	std::string buffer;
	buffer.reserve(BUFSIZE);

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
				 * implementation dependent.
				 */
				_parse( buffer );

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

			} else {
				/**
				 * Anything else is implementation-dependent.
				 */
				_parse( buffer );
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
	 * Apply any post parsing,
	 * if applicable.
	 */
	_postParse();
}

void
BaseParser::parse( const std::string & fn ) {

	std::filebuf fb;
	fb.open(fn, std::ios::in);
	std::istream is(&fb);
	parse(is);
	fb.close();
}


/************************************//*
 * 	PUBLIC METHODS - OUTPUT
 **************************************/

const BaseParser &
BaseParser::print( const std::string & fn ) const {

	std::filebuf fb;
	fb.open(fn, std::ios::out);
	std::ostream os(&fb);
	print(os);
	fb.close();
	return *this;
}

const BaseParser &
BaseParser::showErrors( std::ostream & os ) const {

	if ( ! _errors.empty() ) {
		os << "ERRORS" << std::endl;
		for ( auto const & err : _errors ) {
			os << "**** " << err << std::endl;
		}
	}
	return *this;
}


/************************************//*
 * 	PROTECTED STATIC METHODS
 **************************************/

std::vector<std::string>
BaseParser::_split( const std::string & input, const std::string & str ) {
	std::regex regex(str);
	return _split( input, regex );
}

std::vector<std::string>
BaseParser::_split( const std::string & input, const std::regex & regex ) {
	std::sregex_token_iterator
		first{input.begin(), input.end(), regex, 0},
		last;
	return {first, last};
}


/************************************//*
 * 	PRIVATE METHODS - PARSING
 **************************************/

BaseParser &
BaseParser::_postParse() {
	return *this;
}


/************************************//*
 * 	PRIVATE METHODS - UTILS
 **************************************/

/************************************//*
 * 	PRIVATE STATIC METHODS
 **************************************/

void
BaseParser::_parseUsername( std::string & username ) {

	/**
	 * Convert to uppercase.
	 * Append quotation marks.
	 */
	_toUpper( username );
	_quotationMarks( username );
}

void
BaseParser::_quotationMarks( std::string & str ) {

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
BaseParser::_toUpper( std::string & str ) {
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}
