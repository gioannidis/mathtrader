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
#include "resultparser.hpp"
#include "routechecker.hpp"

#include <exception>
#include <iomanip>
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
	 * Make it throw in problems.
	 */
	lemon::ArgParser ap(argc,argv);
	ap.throwOnProblems();

	/**
	 * Input/Output
	 */
	ap.stringOption("f", "input official wants file", "", true);
	ap.synonym("-official-wants", "f");

	ap.stringOption("r", "input official results file", "", true);
	ap.synonym("-official-results", "r");

	ap.stringOption("o", "output official results file (default: stdout)");
	ap.synonym("-output-file", "o");
	ap.synonym("-results-official", "o");

	ap.stringOption("-output-lgf-file",
			"print the lemon graph format (LGF) file"
			" (default: stdout)");


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


	/********************************************//*
	 * 	Other command-line-only options
	 **********************************************/

	/**
	 * Export input or output to dot files.
	 */
	ap.stringOption("-export-input-dot-file",
			"export the input graph to .dot formatted file");


	/********************************************//*
	 * 	Argument Parsing
	 **********************************************/

	try {
		ap.parse();
	} catch ( const lemon::ArgParserException & error ) {
		return 1;
	}

	/**
	 * Start the timer after parsing the arguments.
	 */
	lemon::TimeReport t("Total execution time: ");


	/**************************************//*
	 * INPUT OPERATIONS
	 ****************************************/

	/**
	 * The Math Trader object.
	 */
	RouteChecker route_checker;

	/*
	 * Want lists and results parsers.
	 * Will parse the the want-list
	 * and the result files.
	 */
	WantParser   want_parser;
	ResultParser result_parser;

	/**
	 * Input File Operations
	 * Read file from wantlist
	 */
	try {
		lemon::TimeReport t("Want-list reading:    ");
		const std::string & fn = ap["f"];
		want_parser.inputFile(fn);

		/**
		 * Run the parser
		 */
		want_parser.parse();

	} catch ( const std::exception & error ) {
		std::cerr << "WantParser error: " << error.what()
			<< std::endl;
		return -1;
	}

	/**
	 * Input File Operations
	 * Read file from results.
	 */
	try {
		lemon::TimeReport t("Result reading:       ");
		const std::string & fn = ap["r"];
		result_parser.inputFile(fn);

		/**
		 * Run the parser
		 */
		result_parser.parse();

	} catch ( const std::exception & error ) {
		std::cerr << "ResultParser error: " << error.what()
			<< std::endl;
		return -1;
	}

	/**
	 * Print the Nodes & Arcs;
	 * forward them to RouteChecker.
	 */
	try {
		lemon::TimeReport t("Passing input graph:  ");
		std::stringstream ss;
		want_parser.print(ss);
		route_checker.graphReader(ss);

	} catch ( const std::exception & error ) {
		std::cerr << "Error during reading "
			" the produced LGF file: "
			<< error.what()
			<< std::endl;

		const std::string fn = "error_graph.lgf";
		want_parser.print(fn);

		std::cerr << "The produced LGF file"
			" has been written to "
			<< fn
			<< std::endl;

		return -1;
	}

	/**
	 * Print the trade loops;
	 * forward them to RouteChecker.
	 */
	try {
		lemon::TimeReport t("Passing trade loops:  ");
		std::stringstream ss;
		result_parser.print(ss);
		route_checker.loopReader(ss);

	} catch ( const std::exception & error ) {

		std::cerr << "Error during reading "
			" the trade loops: "
			<< error.what()
			<< std::endl;

		const std::string fn = "error_loops.txt";
		result_parser.print(fn);

		std::cerr << "The extracted trade loops"
			" have been written to "
			<< fn
			<< std::endl;

		return -1;
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
			route_checker.setPriorities( priorities );

		} else {

			/* Get priority scheme from want file, if any.
			 * Set the priorities if this option has been given.
			 * TODO avoid code repetition in making uppercase.
			 */
			std::string priorities = want_parser.getPriorityScheme();
			if ( priorities.length() > 0 ) {
				std::transform( priorities.begin(), priorities.end(),
						priorities.begin(), ::toupper );
				route_checker.setPriorities( priorities );
			}

		}

	} catch ( const std::exception & error ) {
		std::cerr << "Error during initialization: " << error.what()
			<< std::endl;

		return -1;
	}



	/**************************************//*
	 * 	EXECUTION - CHECK ROUTES
	 ****************************************/

	/**
	 * Run the RouteChecker.
	 */
	try {
		lemon::TimeReport t("RouteChecker execution:  ");
		route_checker.run();

	} catch ( const std::exception & error ) {
		std::cerr << "Error during execution: "
			<< error.what()
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
		lemon::TimeReport t("Result report:        ");

		want_parser.showErrors(os);
		route_checker.printResults(os);

	} catch ( const std::exception & error ) {
		std::cerr << "Error during printing the results: " << error.what()
			<< std::endl;
		fs.close();
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
		want_parser.print(fn);
	}

	/**
	 * Export the input/output graphs to .dot files,
	 * if requested.
	 */
	if ( ap.given("-export-input-dot-file") ) {
		const std::string &fn = ap["-export-input-dot-file"];
		route_checker.exportInputToDot(fn);
	}

	return 0;
}
