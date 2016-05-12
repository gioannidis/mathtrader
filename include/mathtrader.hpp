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
#ifndef _MATHTRADER_HPP_
#define _MATHTRADER_HPP_

#include <lemon/smart_graph.h>
#include <map>

class MathTrader {

public:
	/**
	 * @brief Constructor.
	 * Details.
	 */
	MathTrader();

	/**
	 * @brief Destructor.
	 * Details.
	 */
	~MathTrader();

	/**
	 * @brief Read graph from istream.
	 * Constructs the input trade graph,
	 * from the given input stream.
	 * @param is input stream (default: stdin)
	 * @return *this
	 */
	MathTrader & graphReader( std::istream & is = std::cin );

	/**
	 * @brief Read graph from file.
	 * Constructs the input trade graph,
	 * from the given file.
	 * @param fn file name
	 * @return *this
	 */
	MathTrader & graphReader( const std::string & fn );

	/**
	 * @brief Read graph from file.
	 * Constructs the input trade graph,
	 * from the given file.
	 * @param fn file name
	 * @return *this
	 */
	MathTrader & graphReader( const char * fn );

	/**
	 * @brief Set priorities.
	 * Set the priorities to be used by
	 * the math trade algorithm.
	 * @param priorities The type of priorities to be used. Accepted values:
	 * 	LINEAR-PRIORITIES
	 * 	TRIANGLE-PRIORITIES
	 * 	SQUARE-PRIORITIES
	 * 	SCALED-PRIORITIES
	 * @return *this
	 */
	MathTrader & setPriorities( const std::string & priorities );

	/**
	 * @brief Clear priorities.
	 * Clears any previously given priorities.
	 * No priorities will be used.
	 * @return *this
	 */
	MathTrader & clearPriorities();

	/**
	 * @brief Hide non trades.
	 * Items not being traded will not be shown
	 * in the results.
	 * @return *this
	 */
	MathTrader & hideNonTrades();

	/**
	 * @brief Show non trades.
	 * Items not being traded will be shown
	 * in the results.
	 * This is the default behavior.
	 * @return *this
	 */
	MathTrader & showNonTrades();

	/**
	 * @brief Minimum Cost Flow Algorithms
	 * Enum showing all minimum cost flow algorithms
	 * that are currently implemented.
	 * MCFA::NETWORK_SIMPLEX is the default.
	 */
	enum MCFA {
		NETWORK_SIMPLEX,
		COST_SCALING,
		CAPACITY_SCALING,
		CYCLE_CANCELING,
	};

	/**
	 * @brief Select MCFA
	 * Select the minimum cost flow algorithm to be used.
	 * If not called, MCFA::NETWORK_SIMPLEX will be used.
	 */
	MathTrader & setMCFA( MCFA algorithm );

	/**
	 * @brief MathTrade algorithm.
	 * Runs the MathTrade algorithm.
	 */
	void run();

	/**
	 * @brief Process results.
	 */
	MathTrader & processResults();

	/**
	 * @brief Print results to ostream.
	 * Prints the results of the trade
	 * to the given output stream.
	 * @param os output stream (default: stdout)
	 * @return *this
	 */
	const MathTrader & printResults( std::ostream & os = std::cout ) const ;

	/**
	 * @brief Print results to file.
	 * Prints the results of the trade
	 * to the given file.
	 * @param fn file name (default: stdout)
	 * @return *this
	 */
	const MathTrader & printResults( const std::string & fn ) const ;

	/**
	 * @brief Print results to file.
	 * Prints the results of the trade
	 * to the given file.
	 * @param fn file name (default: stdout)
	 * @return *this
	 */
	const MathTrader & printResults( const char * fn ) const ;

private:
	/**
	 * @brief Priorities enum.
	 * Enumerates all available priority implementations.
	 * The priority will be set by the moderator.
	 * Default is NO_PRIORITIES.
	 */
	enum PriorityScheme {
		NO_PRIORITIES,
		LINEAR_PRIORITIES,
		TRIANGLE_PRIORITIES,
		SQUARE_PRIORITIES,
		SCALED_PRIORITIES,
	};

	PriorityScheme _priority_scheme;
	bool _hide_non_trades;

	/**
	 * @brief Graphs
	 */
	typedef lemon::SmartDigraph InputGraph;
	InputGraph _input_graph;

	/**
	 * @brief Node Maps
	 */
	typedef InputGraph::NodeMap< std::string > StringMap;
	typedef InputGraph::NodeMap< bool > BoolMap;
	StringMap  _name, _username;
	InputGraph::NodeMap< InputGraph::Node > _send, _receive;
	BoolMap _dummy, _trade;

	/**
	 * @brief Arc Maps
	 */
	InputGraph::ArcMap< int > _rank;
	InputGraph::ArcMap< bool > _chosen_arc;

	/**
	 * @brief Username-to-item map
	 */
	std::multimap< std::string, int > _username_to_item;


	/**
	 * @brief Chosen MCFA.
	 * NetworkSimplex is the default.
	 */
	MCFA _mcfa;

	/**
	 * @brief Run math trade algorithm.
	 * Runs the math trade algorithm on the whole graph.
	 */
	void _runFlowAlgorithm();

	/**
	 * @brief Convert rank to cost.
	 * Receives the rank of an arc
	 * and generates the cost to be used
	 * for the minimum flow algorithm.
	 * @param rank The rank of the arc.
	 * @return The corresponding cost.
	 */
	int64_t _getCost( int rank ) const ;

	/**
	 * @brief Merge dummies.
	 * Removes all dummy nodes and merges the arcs.
	 * @return *this
	 */
	MathTrader & _mergeDummies();

	/**
	 * @brief Export to .dot format.
	 * Exports a graph to a file compatible with
	 * graphviz, to visualize the graph.
	 */
	template < typename DGR >
	static void exportToDot( const DGR & g,
			const typename DGR::template NodeMap< std::string > & name );

};

#endif /* _MATHTRADER_HPP_ */
