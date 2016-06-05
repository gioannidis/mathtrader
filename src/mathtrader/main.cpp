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

#include "PracticalSocket.h"

#include <exception>
#include <iomanip>
#include <lemon/arg_parser.h>
#include <lemon/time_measure.h>
#include <new>
#include <sstream>

/* Version */
#define MT_VERSION "1.2e"
#define MT_YEAR "2016"

/* Tabular width for timer output */
#define TABWIDTH (32)


/**********************************************//*
 * 		INTERFACE DECLARATION
 ************************************************/

class Interface {

public:
	Interface( const lemon::ArgParser & ap,
			const std::list< std::string > & argv);
	~Interface();
	int run();

	static void showVersion( std::ostream & os = std::cout );

private:
	/**
	 * Argument parser
	 */
	const lemon::ArgParser & _ap;

	/**
	 * Command line arguments.
	 * Just echoed to output.
	 */
	const std::list< std::string > _argv;

	/**
	 * Output file stream,
	 * when writing to a file.
	 */
	std::ofstream _ofs;

	/**
	 * Retrieve want file from remote url via HTTP.
	 */
	static int _getUrl( const std::string & url,
			std::string & data );


	/**
	 * Make uppercase
	 */
	static void _toUpper( std::string & );
};


/**********************************************//*
 * 		MAIN FUNCTION
 ************************************************/

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
	ap.stringOption("-input-file", "input official wants file (default: stdin)");
	ap.synonym("f", "-input-file");
	ap.synonym("-official-wants", "-input-file");

	ap.stringOption("-output-file", "output official results file (default: stdout)");
	ap.synonym("o", "-output-file");
	ap.synonym("-results-official", "-output-file");

	ap.stringOption("-input-url",
			"input official wants file from url");

	ap.stringOption("-input-lgf-file",
			"parse directly a lemon graph format (LGF) file;"
			" no wants file will be read");

	ap.onlyOneGroup("input_file").
		optionGroup("input_file", "-input-file").
		optionGroup("input_file", "-input-url").
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
			"no priorities will be used; "
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

	ap.boolOption("-benchmark", "run a benchmark"
			" on all implemented minimum cost flow algorithms");

	ap.onlyOneGroup("algorithm").
		optionGroup("algorithm", "-algorithm").
		optionGroup("algorithm", "-benchmark");


	/********************************************//*
	 * 	Other command-line-only options
	 **********************************************/

	/**
	 * Show dummy nodes at the output.
	 * Do not eliminate them.
	 */
	ap.boolOption("-show-dummy-items",
			"show the dummy items instead of merging them; "
			"only useful for debugging purposes");

	/**
	 * Export input to lgf file.
	 */
	ap.stringOption("-export-input-lgf-file",
			"export the input graph to .lgf (LEMON) formatted file");

	/**
	 * Export input or output to dot files.
	 */
	ap.stringOption("-export-input-dot-file",
			"export the input graph to .dot formatted file");
	ap.stringOption("-export-output-dot-file",
			"export the result graph to .dot formatted file");

	/**
	 * Show version
	 */
	ap.boolOption("-version", "show version information and exit");
	ap.synonym("v", "-version");


	/********************************************//*
	 * 	Argument Parsing
	 **********************************************/

	try {
		ap.parse();
	} catch ( const lemon::ArgParserException & error ) {
		return 1;
	}


	/********************************************//*
	 * 	Show version
	 **********************************************/

	if ( ap.given("-version") ) {
		Interface::showVersion();
		std::cout << "Copyright (C) " << MT_YEAR << "."
			<< std::endl
			<< "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>"
			<< std::endl
			<< "There is NO WARRANTY; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."
			<< std::endl
			<< "Written by George Ioannidis."
			<< std::endl;

		return 0;
	}


	/********************************************//*
	 * 	Tokenize arguments
	 **********************************************/

	/**
	 * Useful to report to output.
	 */
	std::list< std::string > arg_list;
	for ( int i = 0; i < argc; ++ i ) {
		arg_list.push_back( argv[i] );
	}


	/********************************************//*
	 * 	Running MathTrader++
	 **********************************************/

	/**
	 * Construct the Interface.
	 * Note that it cannot change the ArgParser
	 * in any way.
	 */
	Interface runner(ap, arg_list);

	/**
	 * Run the application.
	 * Any caught exceptions should be considered FATAL.
	 * In general, class Interface should handle exceptions
	 * and re-throw them only when FATAL.
	 */
	try {
		runner.run();
	} catch ( const std::exception & error ) {
		std::cerr << "FATAL error: "
			<< error.what()
			<< std::endl;
		return -1;
	} catch ( ... ) {
		std::cerr << "FATAL error: "
			<< "unknown exception caught"
			<< std::endl;
		return -2;
	}

	return 0;
}



/**********************************************//*
 * 		INTERFACE Methods
 ************************************************/

Interface::Interface( const lemon::ArgParser & ap,
		const std::list< std::string > & argv ) :
	_ap( ap ),
	_argv( argv )
{
}

Interface::~Interface() {

	/**
	 * On destruction: always close file stream
	 * if applicable
	 */
	if ( _ofs.is_open() ) {
		_ofs.close();
	}
}

int
Interface::run() {

	/**
	 * Start the global timer
	 */
	std::stringstream time_ss;
	time_ss << std::left << std::setw(TABWIDTH)
		<< "Total execution time:";
	lemon::TimeReport t(time_ss.str());

	/**
	 * Argument Parsing reference.
	 */
	auto const & ap = this->_ap;


	/**************************************//*
	 * OPEN OUTPUT STREAM
	 ****************************************/

	/**
	 * First operation is always to open the output file stream.
	 * The output will be written to either a file or std::cout.
	 * Open the output file, if needed.
	 */
	std::ofstream & fs = this->_ofs;
	bool write_to_file = ap.given("-output-file");

	if ( write_to_file ) {

		const std::string & fn = ap["-output-file"];
		fs.open(fn, std::ios_base::out);

		/**
		 * On fail, just append to std::cout
		 * TODO consider printing this at the destructor,
		 * in order to be printed last,
		 * as the user might not see it.
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


	/**************************************//*
	 * HEADER OF OUTPUT FILE
	 ****************************************/

	/**
	 * 1. Always the version on top.
	 * 2. Command line arguments.
	 * 3. Source of input.
	 */
	showVersion(os);

	/* Command line arguments */
	os << "Command:";
	for ( auto x : _argv ) {
		os << " " << x;
	}
	os << std::endl;

	/* Input source */
	os << "Input from ";
	if ( ap.given("-input-lgf-file") ) {
		const std::string & fn = ap["-input-lgf-file"];
		os << "local LGF file: " << fn;
	} else if ( ap.given("-input-file") ) {
		const std::string & fn = ap["-input-file"];
		os << "local official-wants file: " << fn;
	} else {
		os << "stdin";
	}
	os << std::endl;
	os << std::endl; /**< double endl */


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
			/**
			 * Start the timer
			 */
			std::stringstream time_ss;
			time_ss << std::left << std::setw(TABWIDTH)
				<< "Reading the input graph:";
			lemon::TimeReport t(time_ss.str());

			/**
			 * Read the input LGF,
			 * from either std::cin or file.
			 */
			const std::string & fn = ap["-input-lgf-file"];
			math_trader.graphReader(fn);

		} catch ( const std::exception & error ) {
			std::cerr << "Error during reading"
				" the LGF file: "
				<< error.what()
				<< std::endl;
			return -1;
		}

	} else if ( ap.given("-input-url") ) {

		/**
		 * Start the timer
		 */
		std::stringstream time_ss;
		time_ss << std::left << std::setw(TABWIDTH)
			<< "Retrieving remote want lists:";
		lemon::TimeReport t(time_ss.str());

		std::stringstream want_ss;
		const std::string & url = ap["-input-url"];

		std::string data;
		try {
			_getUrl( url, data );

		} catch ( const SocketException & error ) {
			os << "Socket Exception: "
				<< error.what()
				<< std::endl;
			return -1;

		} catch ( const std::exception & error ) {
			os << "Error during retrieving data: "
				<< error.what()
				<< std::endl;
			return -2;
		}
		os << data << std::endl;

		return 0;

	} else {

		/**
		 * Read file from wantlist
		 */
		try {
			/**
			 * Start the timer
			 */
			std::stringstream time_ss;
			time_ss << std::left << std::setw(TABWIDTH)
				<< "Reading the want lists:";
			lemon::TimeReport t(time_ss.str());

			/**
			 * Configure input file
			 */
			if ( ap.given("-input-file") ) {
				const std::string & fn = ap["-input-file"];
				if ( fn.length() > 0 ) {
					want_parser.inputFile(fn);
				}
			}

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
		 * Print the Nodes & Arcs;
		 * forward them to Math Trader.
		 */
		std::stringstream ss;
		want_parser.print(ss);

		try {
			/**
			 * Start the timer
			 */
			std::stringstream time_ss;
			time_ss << std::left << std::setw(TABWIDTH)
				<< "Passing input graph:";
			lemon::TimeReport t(time_ss.str());

			/**
			 * Start the reading
			 */
			math_trader.graphReader(ss);

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
	}


	/**************************************//*
	 * PARAMETER CONFIGURATION
	 ****************************************/

	/* Outermost try to configure parameters.
	 * We should only catch unhandled exceptions.
	 */
	try {
		/**
		 * Set the priority scheme.
		 * Make uppercase.
		 * Fallback to default value on error.
		 */
		try {

			if ( ap.given("-no-priorities") ) {

				/* Do nothing;
				 * override want file.
				 */
				math_trader.clearPriorities();

			} else if ( ap.given("-priorities") ) {

				/* Set priorities;
				 * override want file.
				 */
				std::string priorities( ap["-priorities"] );
				_toUpper( priorities );
				math_trader.setPriorities( priorities );

			} else if ( !input_lgf_file ) {

				/* Get priority scheme from want file, if any.
				 * Set the priorities if this option has been given.
				 * Make uppercase.
				 */
				std::string priorities = want_parser.getPriorityScheme();
				if ( priorities.length() > 0 ) {
					_toUpper( priorities );
					math_trader.setPriorities( priorities );
				}

			}

		} catch ( const std::runtime_error & error ) {

			/* Catch only runtime errors */
			std::cerr << "Error in setting the priority scheme: "
				<< error.what()
				<< std::endl;

			/* On error: clear the priorities */
			math_trader.clearPriorities();
			std::cerr << "Warning: falling back to "
				<< "no priorities"
				<< std::endl;
		}

		/**
		 * Show/Hide non-trading items
		 */
		if ( ap.given("-show-non-trades") ) {
			math_trader.hideNonTrades(false);
		} else if ( ap.given("-hide-non-trades") || want_parser.hideNonTrades() ) {
			math_trader.hideNonTrades();
		}

		/**
		 * Algorithm to be used
		 * Make uppercase.
		 * Fallback to default value on error.
		 */
		try {
			if ( ap.given("-algorithm") ) {

				std::string algorithm( ap["-algorithm"] );
				_toUpper( algorithm );
				math_trader.setAlgorithm( algorithm );
			}

		} catch ( const std::runtime_error & error ) {

			/* Catch only runtime errors */
			std::cerr << "Error in setting the algorithm: "
				<< error.what()
				<< std::endl;

			/* On error: set the default algorithm */
			const std::string algorithm("NETWORK-SIMPLEX");
			math_trader.setAlgorithm( algorithm );
			std::cerr << "Warning: falling back to "
				<< algorithm
				<< std::endl;
		}

	} catch ( const std::exception & error ) {

		/* Any unhandled exceptions */
		std::cerr << "Error during initialization: " << error.what()
			<< std::endl;

		return -1;
	}



	/**************************************//*
	 * MATH TRADING EXECUTION
	 ****************************************/

	/**
	 * Run the math trading algorithm.
	 */
	try {
		if ( !ap.given("-benchmark") ) {

			/**
			 * Start the timer
			 */
			std::stringstream time_ss;
			time_ss << std::left << std::setw(TABWIDTH)
				<< "Execution:";
			lemon::TimeReport t(time_ss.str());

			/**
			 * Start the execution
			 */
			math_trader.run();

		} else {

			/**
			 * Benchmarking
			 * Run the application with all algorithms.
			 */
			const std::list< std::string > algorithms = {
				"NETWORK-SIMPLEX",
				"COST-SCALING",
				"CAPACITY-SCALING",
				"CYCLE-CANCELING"
			};

			for ( auto const & algo : algorithms ) {

				math_trader.setAlgorithm( algo );

				/**
				 * Start the timer
				 */
				std::stringstream time_ss;
				time_ss << std::left << std::setw(TABWIDTH)
					<< "Execution of " + algo + ":";
				lemon::TimeReport t(time_ss.str());

				/**
				 * Execute
				 */
				math_trader.run();
			}
		}

	} catch ( const std::exception & error ) {
		std::cerr << "Error during execution: "
			<< error.what()
			<< std::endl;

		if ( !input_lgf_file ) {
			const std::string fn = "error_graph.lgf";
			want_parser.print(fn);

			std::cerr << "The produced LGF file"
				" has been written to "
				<< fn
				<< std::endl;
		}

		return -1;
	}



	/**************************************//*
	 * OUTPUT OPERATIONS - MATH TRADES
	 ****************************************/

	/**
	 * Merge the dummy nodes, so that they will not
	 * appear in the results.
	 * Ignore if we want to see them.
	 */
	if ( !ap.given("-show-dummy-items") ) {
		try {
			math_trader.mergeDummyItems();
		} catch ( const std::exception & error ) {
			std::cerr << "Error during merging dummy items: "
				<< error.what()
				<< std::endl;
		}
	}


	/**
	 * Print the WantParser and the MathTrader results
	 * to the same output stream, i.e., either std::cout
	 * or the output file.
	 */
	try {
		/**
		 * Start the timer
		 */
		std::stringstream time_ss;
		time_ss << std::left << std::setw(TABWIDTH)
			<< "Result processing & report:";
		lemon::TimeReport t(time_ss.str());

		/**
		 * Print want_parser information:
		 * - Options
		 * - Errors
		 * - Missing items
		 */
		want_parser.printOptions(os);
		if ( !want_parser.hideErrors() ) {
			want_parser.showErrors(os);
		}
		if ( want_parser.showMissing() ) {
			want_parser.printMissing(os);
		}

		/**
		 * Process the results:
		 * - Eliminate dummies
		 * - Set output options
		 * TODO provide a better interface
		 * that checks command line overrides.
		 * Report the results.
		 */
		math_trader.
			hideLoops( want_parser.hideLoops() ).
			hideStats( want_parser.hideStats() ).
			hideSummary( want_parser.hideSummary() ).
			sortByItem( want_parser.sortByItem() ).
			writeResults(os);

	} catch ( const std::exception & error ) {
		std::cerr << "Error during printing the results: " << error.what()
			<< std::endl;
		return -1;
	}


	/**
	 * End of STANDARD mathtrader++ operations.
	 * The LAST thing to append to the standard output (file)
	 * is the elapsed REAL time until here,
	 * if it has been requested by the moderator.
	 * The elapsed real time of the application might
	 * be greater if further export operations have been defined.
	 * Finally, close the output file stream if needed.
	 */
	if ( want_parser.showElapsedTime() ) {
		os << "Elapsed real time = "
			<< t.realTime()
			<< "s"
			<< std::endl;
	}

	/* Close file stream */
	if ( fs.is_open() ) {
		fs.close();
	}


	/**************************************//*
	 * OUTPUT OPERATIONS - UTILITIES
	 ****************************************/

	/**
	 * NOTE: none of these operations
	 * should write to the default output stream,
	 */

	/**
	 * Export the input graph to .lgf file,
	 * if requested.
	 */
	if ( ap.given("-export-input-lgf-file") ) {

		const std::string &fn = ap["-export-input-lgf-file"];
		want_parser.print(fn);
	}

	/**
	 * Export the input/output graphs to .dot files,
	 * if requested.
	 */
	if ( ap.given("-export-input-dot-file") ) {
		const std::string &fn = ap["-export-input-dot-file"];
		math_trader.exportInputToDot(fn);
	}
	if ( ap.given("-export-output-dot-file") ) {
		const std::string &fn = ap["-export-output-dot-file"];
		math_trader.exportOutputToDot(fn);
	}

	return 0;
}

int
Interface::_getUrl( const std::string & url,
		std::string & data ) {

	/**
	 * Clear the @data
	 */
	data.clear();

	/**
	 * Sanity check: url
	 */
	if ( url.compare(0,7,"http://") != 0 ) {
		throw std::runtime_error("Provided url "
				"is not HTTP; "
				"expected url beginning "
				"with 'http://'");
	}

	size_t i = url.find("/",7);
	if ( i == std::string::npos ) {
		return -1;
	}

	/**
	 * Server and request
	 */
	const std::string
		server  = url.substr(7,i-7),	/**< strip 'http://' */
		request = "GET "
			+ url.substr(i,std::string::npos)
			+ " HTTP/1.1\r\n"
			+ "Host: " + server + "\r\n"
			+ "\r\n";

	/**
	 * Open the socket;
	 * socket destructor will close it.
	 * Throws exception on failure.
	 */
	TCPSocket sock(server, 80);

	/**
	 * Send the HTTP request.
	 */
	sock.send(request.c_str(), request.length());



	/**
	 * Receive buffer
	 */
	const int BUFSIZE = (10 * (1 << 20));
	std::unique_ptr< char > buffer( new char [BUFSIZE] );

	/**
	 * Receive response until
	 * no further bytes are received.
	 */
	int message_size = 0;
	int received_total = 0;
	while ((message_size = sock.recv(buffer.get(), BUFSIZE)) > 0 ) {
		data.append(buffer.get(), message_size);
		received_total += message_size;
	}
	std::cout << "Received " << received_total << std::endl;
	return 0;
}

void
Interface::_toUpper( std::string & str ) {

	std::transform( str.begin(), str.end(),
			str.begin(), ::toupper );
}

void
Interface::showVersion( std::ostream & os ) {
	os << "mathtrader++ version " << MT_VERSION << std::endl;
}
