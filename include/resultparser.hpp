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
#ifndef _RESULTPARSER_HPP_
#define _RESULTPARSER_HPP_

#include "baseparser.hpp"
#include <map>
#include <vector>

class ResultParser : public BaseParser {

public:
	/**
	 * @brief Constructor.
	 * Details.
	 */
	ResultParser();

	/**
	 * @brief Destructor.
	 * Details.
	 */
	~ResultParser();

	/**
	 * @brief Print LGF file.
	 * Prints the nodes & arcs in a lemon-LGF format.
	 * @param os The output stream (default: std::cout).
	 * @return *this
	 */
	using BaseParser::print;	/**< Consider base class overloads */
	const ResultParser & print( std::ostream & os = std::cout ) const ;

	/**
	 * @brief Print LGF file to output file.
	 * Prints the nodes & arcs in a lemon-LGF format.
	 * @param fn The file output name.
	 * @return *this
	 */
	const ResultParser & print( const std::string & fn ) const ;

private:
	/***************************//*
	 * 	OPTIONS
	 *****************************/

	/**
	 * Enum of current status
	 */
	enum Status {
		BEGIN,		/**< Default */
		TRADE_LOOPS,
		ITEM_SUMMARY,
		UNKNOWN,
	};
	Status _status;


	/***************************//*
	 * INTERNAL DATA STRUCTURES
	 *****************************/

	/**
	 * @brief Cycle list.
	 * List to keep cycle nodes.
	 * Expected: A B C D A E F G H E
	 */
	std::list< std::string > _item_list;

	/**
	 * @brief Loop flags
	 * Flag to indicate that a new loop will begin.
	 * First item of cycle to detect its end.
	 */
	bool _new_loop;
	std::string _first_item;


	/********************************//*
	 *  UTILITY NON_STATIC FUNCTIONS
	 ***********************************/

	/**
	 * @brief Parse want file line.
	 * Parses a want list file line:
	 * options, official names and want lists.
	 */
	void _parse( const std::string & line );

	/**
	 * @brief Post Parsing
	 * Marks unknown items.
	 * @return *this
	 */
	ResultParser & _postParse();

	/**
	 * @brief Parse loop
	 * @return *this
	 */
	ResultParser & _parseLoop( const std::string & option );

};

#endif /* _RESULTPARSER_HPP_ */
