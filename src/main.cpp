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
	ap.stringOption("f", "input official wants file (default: stdin)");
	ap.synonym("-input-file", "f");
	ap.synonym("-official-wants", "f");

	ap.stringOption("o", "output official results file (default: stdout)");
	ap.synonym("-output-file", "o");
	ap.synonym("-results-official", "o");

	ap.stringOption("-input-lgf-file",
			"parse directly a lemon graph format (LGF) file"
			" (default: stdin);"
			" no wants file will be read");

	ap.stringOption("-output-lgf-file",
			"print the lemon graph format (LGF) file"
			" (default: stdout)");

	ap.onlyOneGroup("input_file").
		optionGroup("input_file", "f").
		optionGroup("input_file", "-input-lgf-file");


	/********************************************//*
	 * Overriding options from want file
	 **********************************************/

	/**
	 * Priority scheme.
	 */
	const std::string info_override("overrides option in official-wants file");

	ap.stringOption("-priorities", "set the priorities:"
			" LINEAR-PRIORITIES"
			" TRIANGLE-PRIORITIES"
			" SQUARE-PRIORITIES"
			" SCALED-PRIORITIES"
			"; "
			+ info_override);
	ap.boolOption("-no-priorities",
			"clears any priorites; "
			+ info_override);

	ap.onlyOneGroup("priority_scheme").
		optionGroup("priority_scheme", "-priorities").
		optionGroup("priority_scheme", "-no-priorities");

	/**
	 * Show or hide non-trades.
	 * Only one option may be given.
	 */
	ap.boolOption("-show-non-trades",
			"show non-trading items; "
			+ info_override);
	ap.boolOption("-hide-non-trades",
			"do not show non-trading items; "
			+ info_override);

	ap.onlyOneGroup("non_trades").
		optionGroup("non_trades", "-show-non-trades").
		optionGroup("non_trades", "-hide-non-trades");


	/********************************************//*
	 * 	Minimum Cost Flow Algorithm options
	 **********************************************/

	ap.stringOption("-algorithm", "set the minimum cost"
			" flow algorithm:"
			" NETWORK-SIMPLEX"
			" COST-SCALING"
			" CAPACITY-SCALING"
			" CYCLE-CANCELING"
			" (default: NETWORK-SIMPLEX)");


	/********************************************//*
	 * 	Argument Parsing
	 **********************************************/

	try {
		ap.parse();
	} catch ( lemon::ArgParserException & error ) {
		return 1;
	}

	/**
	 * Start the timer after parsing the arguments.
	 */
	lemon::TimeReport t("Total execution time of "
			+ std::string(argv[0])
			+ ": ");


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
	const bool input_lgf_file = ap.given("-input-lgf-file");
	if ( input_lgf_file ) {

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
		/**
		 * Priority scheme:
		 * - Any option from command line overrides given options
		 *   in the want file.
		 * Make uppercase.
		 */
		if ( ap.given("-no-priorities") ) {

			/* Do nothing;
			 * override want file.
			 */

		} else if ( ap.given("-priorities") ) {

			/* Set priorities;
			 * override want file.
			 */
			std::string priorities( ap["-priorities"] );
			std::transform( priorities.begin(), priorities.end(),
					priorities.begin(), ::toupper );
			math_trader.setPriorities( priorities );

		} else if ( !input_lgf_file ) {

			/* Get priority scheme from want file, if any.
			 * Set the priorities if this option has been given.
			 * TODO avoid code repetition in making uppercase.
			 */
			std::string priorities = want_parser.getPriorityScheme();
			if ( priorities.length() > 0 ) {
				std::transform( priorities.begin(), priorities.end(),
						priorities.begin(), ::toupper );
				math_trader.setPriorities( priorities );
			}

		}

		/**
		 * Show/Hide non-trading items
		 */
		if ( ap.given("-show-non-trades") ) {

			math_trader.showNonTrades();

		} else if ( ap.given("-hide-non-trades") || want_parser.hideNonTrades() ) {

			math_trader.hideNonTrades();
		}

		/**
		 * Algorithm to be used
		 * Make uppercase.
		 */
		if ( ap.given("-algorithm") ) {

			std::string algorithm( ap["-algorithm"] );
			std::transform( algorithm.begin(), algorithm.end(),
					algorithm.begin(), ::toupper );
			math_trader.setAlgorithm( algorithm );
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
	 * OUTPUT OPERATIONS - MATH TRADES
	 ****************************************/

	/**
	 * We will print the the output to either a file
	 * or std::cout.
	 * Open the output file, if needed.
	 */
	std::ofstream fs;
	bool write_to_file = ap.given("o");

	if ( write_to_file ) {

		const std::string & fn = ap["o"];
		fs.open(fn, std::ios_base::out);

		/**
		 * On fail, just append to std::cout
		 */
		if ( fs.fail() ) {
			std::cerr << "Error opening output file "
				+ fn
				+ "; will append to standard output instead."
				<< std::endl;
			write_to_file = false;
		}

	}


	/**
	 * Set the output stream to the file stream
	 * or std::cout, whichever is applicable.
	 */
	std::ostream & os = (write_to_file) ? fs : std::cout;


	/**
	 * Print the WantParser and the MathTrader results
	 * to the same output stream, i.e., either std::cout
	 * or the output file.
	 */
	try {
		lemon::TimeReport t("Results time: ");

		want_parser.showOptions(os);
		want_parser.showErrors(os);
		math_trader.tradeLoops(os);
		//math_trader.itemSummary(os);

	} catch ( std::exception & error ) {
		std::cerr << "Error during printing the results: " << error.what()
			<< std::endl;
		return -1;
	}

	/**
	 * Close output file stream if needed.
	 */
	if ( fs.is_open() ) {
		fs.close();
	}



	/**************************************//*
	 * OUTPUT OPERATIONS - UTILITIES
	 ****************************************/

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
