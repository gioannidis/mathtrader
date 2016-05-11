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
#include <sstream>


int main(int argc, char **argv) {


	/**************************************//*
	 * COMMAND LINE ARGUMENT PARSING
	 ****************************************/

	/**
	 * Argument parser object.
	 * Will parse the command line options.
	 */
	lemon::ArgParser ap(argc,argv);
	ap.throwOnProblems();

	/**
	 * Input/Output
	 */
	ap.stringOption("f", "input want list file (default: stdin)");
	ap.synonym("-input-file", "f");
	ap.synonym("-official-wants", "f");

	ap.stringOption("o", "output file (default: stdout)");
	ap.synonym("-output-file", "o");

	ap.stringOption("-input-lgf-file",
			"input lemon graph format (LGF) file"
			" (default: stdin)");

	ap.stringOption("-output-lgf-file",
			"print the lemon graph format (LGF) file"
			" (default: stdout)");

	/**
	 * Overriding options from want file
	 */
	ap.stringOption("p", "set the priorities:"
			" LINEAR-PRIORITIES"
			" TRIANGLE-PRIORITIES"
			" SQUARE-PRIORITIES"
			" SCALED-PRIORITIES");
	ap.synonym("-priorities", "p");

	ap.boolOption("-hide-no-trades",
			"do not show non-trading items",
			true);

	/**
	 * Run the argument parser.
	 */
	try {
		ap.parse();
	} catch ( lemon::ArgParserException & error ) {
		return 1;
	}



	/**************************************//*
	 * INPUT OPERATIONS
	 ****************************************/

	/**
	 * The Math Trader object.
	 */
	MathTrader math_trader;

	/*
	 * Want List parser.
	 * Will parse the wantlist and configure
	 * the Math Trader.
	 * Make it throw in problems.
	 */
	WantParser want_parser;

	/**
	 * Input File Operations
	 * - If "--input-lgf-file" is given
	 *   we will skip the wantlist parser
	 *   and directly read the graph.
	 *
	 * - Otherwise, we will invoke the wantlist parser.
	 */
	if ( ap.given("-input-lgf-file") ) {

		/**
		 * Read LGF from file or stdin
		 */
		try {
			lemon::TimeReport t("Reading time: ");

			const std::string & fn = ap["-input-lgf-file"];
			if ( fn.length() > 0 ) {
				math_trader.graphReader(fn);
			} else {
				math_trader.graphReader();
			}
		} catch ( std::exception & error ) {
			std::cerr << "Error during reading"
				" the LGF file: "
				<< error.what()
				<< std::endl;
			return -1;
		}

	} else {

		/**
		 * Read file from wantlist
		 */
		try {
			lemon::TimeReport t("Wantlist parsing time: ");

			/**
			 * Configure input file
			 */
			if ( ap.given("f") ) {
				const std::string & fn = ap["f"];
				if ( fn.length() > 0 ) {
					want_parser.wantlist(fn);
				}
			}

			/**
			 * Run the parser
			 */
			want_parser.parse();

		} catch ( std::exception & error ) {
			std::cerr << "WantParser error: " << error.what()
				<< std::endl;
			return -1;
		}


		/**
		 * Print the Nodes & Arcs;
		 * forward them to Math Trader.
		 */
		std::stringstream ss;
		want_parser.print(ss);

		try {
			lemon::TimeReport t("Reading the produced"
					" LGF file time: ");

			math_trader.graphReader(ss);
		} catch ( std::exception & error ) {
			std::cerr << "Error during reading "
				" the produced LGF file: "
				<< error.what()
				<< std::endl;

			const std::string fn = "error_graph.lgf";
			want_parser.print(fn);

			std::cerr << "The produced LGF file"
				" has been printed to file"
				<< fn
				<< std::endl;

			return -1;
		}
	}


	/**************************************//*
	 * PARAMETER CONFIGURATION
	 ****************************************/

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



	/**************************************//*
	 * MATH TRADING EXECUTION
	 ****************************************/

	/**
	 * Run the math trading algorithm.
	 * Do not exit on error; there might be pending
	 * output operations.
	 */
	try {
		lemon::TimeReport t("Execution time: ");
		math_trader.run();
	} catch ( std::exception & error ) {
		std::cerr << "Error during execution: " << error.what()
			<< std::endl;

		return -1;
	}


	/**************************************//*
	 * OUTPUT OPERATIONS
	 ****************************************/

	want_parser.showOptions();
	/**
	 * Print the results to file or stdout.
	 * Skip if the algorithm has failed.
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


	/**
	 * Print the produced lgf file, if requested.
	 */
	if ( ap.given("-output-lgf-file") ) {

		const std::string &fn = ap["-output-lgf-file"];
		if ( fn.length() > 0 ) {
			want_parser.print(fn);
		} else {
			want_parser.print();
		}
	}

	return 0;
}
