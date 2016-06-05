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
#ifndef _BASEPARSER_HPP_
#define _BASEPARSER_HPP_

#include <iostream>
#include <fstream>
#include <list>
#include <regex>
#include <string>

class BaseParser {

public:
	/**
	 * @brief Constructor.
	 * Details.
	 */
	BaseParser();

	/**
	 * @brief Destructor.
	 * Details.
	 */
	virtual ~BaseParser();

	/**
	 * @brief Set wantlist input file.
	 * Sets the input file where the wantlist
	 * will be read from.
	 * If not called, std::cin will be used, instead.
	 * @param fn file name
	 * @return *this
	 */
	BaseParser & inputFile( const std::string & fn );

	/**
	 * @brief Parse the input.
	 * Parses the input from a given istream
	 * (default: std::cin)
	 */
	void parse( std::istream & is = std::cin );

	/**
	 * @brief Parse the input.
	 * Parses the input from a given file.
	 */
	void parse( const std::string & fn );

	/**
	 * @brief Print implementation-dependent output.
	 * Prints the results of the parsing process.
	 * The content and format is implementation-dependent
	 * of the derived classes.
	 * @param os The output stream (default: std::cout).
	 * @return *this
	 */
	virtual const BaseParser & print( std::ostream & os = std::cout ) const = 0;

	/**
	 * @brief Print implementation-dependent output.
	 * Prints the results of the parsing process.
	 * The content and format is implementation-dependent
	 * of the derived classes.
	 * @param fn The file output name.
	 * @return *this
	 */
	virtual const BaseParser & print( const std::string & fn ) const ;

	/**
	 * @brief Show errors.
	 * Prints any logged errors that were encountered
	 * during parse().
	 * @param os The output stream (default: std::cout).
	 * @return *this;
	 */
	const BaseParser & showErrors( std::ostream & os = std::cout ) const ;

protected:

	/********************************//*
	 *  UTILITY STATIC FUNCTIONS
	 ***********************************/

	/**
	 * @brief Split string.
	 * Splits the string based on a regular expression.
	 * @param input The input string.
	 * @param regex Regular expression defining the fields.
	 * @return Vector with matches.
	 */
	static std::vector<std::string> _split( const std::string & input,
			const std::string & str );
	static std::vector<std::string> _split( const std::string & input,
			const std::regex & regex );

	/**
	 * @brief Parse username.
	 * Appends quotation marks and converts to uppercase.
	 * Usernames on BGG are always case-insensitive.
	 * @param username username to be parsed.
	 * @return *this
	 */
	static void _parseUsername( std::string & username );

	/**
	 * @brief Append quotation marks.
	 * Appends quotation marks to string,
	 * except if there are opening/closing quotation marks.
	 * @str Input string to modify.
	 */
	static void _quotationMarks( std::string & str );

	/**
	 * @brief Convert to uppercase.
	 * Converts the given string to uppercase.
	 */
	static void _toUpper( std::string & str );

private:

	/***************************//*
	 * 	PARSING
	 *****************************/

	/**
	 * @brief Specialized parsing.
	 * Implementation dependent.
	 * Continues input parsing;
	 * called by parse().
	 */
	virtual void _parse( const std::string & line ) = 0;

	/**
	 * @brief Post Parsing sequence.
	 * Base class does nothing;
	 * however, child classes
	 * may require bookkeeping after parsing is done.
	 */
	virtual BaseParser & _postParse();

	/**
	 * Errors list.
	 */
	std::list< std::string > _errors;


	/***************************//*
	 * INTERNAL DATA STRUCTURES
	 *****************************/

	/**
	 * @brief File Stream
	 * File stream to read the want list from,
	 * when applicable.
	 */
	std::ifstream _fs;
};

#endif /* _BASEPARSER_HPP_ */
