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
#include "mathtrader.hpp"

#include <exception>
#include <iostream>
#include <lemon/arg_parser.h>
#include <lemon/time_measure.h>

int main(int argc, char **argv) {

	/**
	 * Argument parser
	 * - Initializer
	 */
	lemon::ArgParser ap(argc,argv);
	ap.stringOption("f", "input file (default: stdin)");
	ap.synonym("-input-file", "f");
	ap.stringOption("o", "output file (default: stdout)");
	ap.synonym("-output-file", "o");
	ap.stringOption("p", "set the priorities:"
			" LINEAR-PRIORITIES"
			" TRIANGLE-PRIORITIES"
			" SQUARE-PRIORITIES"
			" SCALED-PRIORITIES");
	ap.synonym("-priorities", "p");
	ap.boolOption("-hide-no-trades",
			"do not show non-trading items",
			true);

	ap.throwOnProblems();
	try {
		ap.parse();
	} catch ( lemon::ArgParserException & error ) {
		return 1;
	}

	WantParser want_parser;
	try {
		//want_parser.wantlist("ss");
		want_parser.parse();
	} catch ( std::exception & error ) {
		std::cerr << "WantParser error: " << error.what()
			<< std::endl;
		return -1;
	}
	return 0;

	MathTrader math_trader;
	try {
		if ( ap.given("p") ) {
			math_trader.setPriorities(ap["p"]);
		}
		if ( ap.given("-hide-no-trades") ) {
			math_trader.hideNoTrades();
		}
	} catch ( std::exception & error ) {
		std::cerr << "Error during initialization: " << error.what()
			<< std::endl;
		return -1;
	}

	/**
	 * Read input graph from file or stdin
	 */
	try {
		lemon::TimeReport t("Reading time: ");
		if ( ap.given("f") ) {
			math_trader.graphReader(ap["f"]);
		} else {
			math_trader.graphReader();
		}
	} catch ( std::exception & error ) {
		std::cerr << "Error during reading: " << error.what()
			<< std::endl;
		return -1;
	}

	/**
	 * Run the math trading algorithm
	 */
	try {
		lemon::TimeReport t("Execution time: ");
		math_trader.run();
	} catch ( std::exception & error ) {
		std::cerr << "Error during execution: " << error.what()
			<< std::endl;
		return -1;
	}

	/**
	 * Print the results to file or stdout
	 */
	try {
		lemon::TimeReport t("Results time: ");
		math_trader.processResults();
		if ( ap.given("o") ) {
			math_trader.printResults(ap["o"]);
		} else {
			math_trader.printResults();
		}
	} catch ( std::exception & error ) {
		std::cerr << "Error during printing the results: " << error.what()
			<< std::endl;
		return -1;
	}

	return 0;
}
