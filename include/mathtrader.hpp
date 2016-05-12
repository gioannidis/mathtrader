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

#include <lemon/list_graph.h>
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
	 * If not called, no priorities will be used.
	 * @param priorities The type of priorities to be used. Accepted values:
	 * 	LINEAR-PRIORITIES
	 * 	TRIANGLE-PRIORITIES
	 * 	SQUARE-PRIORITIES
	 * 	SCALED-PRIORITIES
	 * @return *this
	 */
	MathTrader & setPriorities( const std::string & priorities );

	/**
	 * @brief Select Algorithm
	 * Set the minimum cost flow algorithm to be used.
	 * If not called, Network Simplex will be used.
	 * @param algorithm The algorithm to be used. Accepted values:
	 * 	NETWORK-SIMPLEX
	 * 	COST-SCALING
	 * 	CAPACITY-SCALING
	 * 	CYCLE-CANCELING
	 * @return *this
	 */
	MathTrader & setAlgorithm( const std::string & algorithm );

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
	 * @brief MathTrade algorithm.
	 * Runs the MathTrade algorithm.
	 */
	void run();

	/**
	 * @brief Display trade loops.
	 * Displays the trade loops that have been found
	 * to the given output stream.
	 * @param os output stream (default: stdout)
	 * @return *this
	 */
	const MathTrader & tradeLoops( std::ostream & os = std::cout ) const ;

	/**
	 * @brief Display item summary.
	 * Displays the item summary
	 * to the given output stream.
	 * - If trading
	 * - Sending item
	 * - Receiving item
	 * Items are sorted by usernames.
	 * @param os output stream (default: stdout)
	 * @return *this
	 */
	const MathTrader & itemSummary( std::ostream & os = std::cout ) const ;

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

	/**
	 * @brief Minimum Cost Flow Algorithms
	 * Enumerates all available minimum cost flow algorithm implementations.
	 * Default is NETWORK_SIMPLEX.
	 */
	enum MCFA {
		NETWORK_SIMPLEX,
		COST_SCALING,
		CAPACITY_SCALING,
		CYCLE_CANCELING,
	};

	PriorityScheme _priority_scheme;
	MCFA _mcfa;

	bool _hide_non_trades;

	/**
	 * @brief Input Graph
	 * Type of Input Graph, member and maps.
	 * Nodes: items
	 * Arc A->B: item A is offered for item B
	 */
	typedef lemon::SmartDigraph InputGraph;	/**< type of input graph */
	const InputGraph _input_graph;		/**< actual input graph */


	InputGraph::NodeMap< std::string >	/**< node maps: strings */
		_name,				/**< official item name	*/
		_username;			/**< owner's username	*/

	InputGraph::NodeMap< bool >	/**< node maps: boolean	*/
		_dummy;			/**< item is a dummy */

	InputGraph::ArcMap< int >	/**< arc maps: integer	*/
		_rank;			/**< rank of want	*/

	/**
	 * @brief Output Graph
	 * Type of Output Graph, member and maps.
	 */
	typedef lemon::ListDigraph OutputGraph;
	OutputGraph _output_graph;

	InputGraph::NodeMap< OutputGraph::Node >	/**< node reference: in -> out */
		_node_in2out;
	InputGraph::ArcMap< OutputGraph::Arc >		/**< arc reference: in -> out */
		_arc_in2out;

	OutputGraph::NodeMap< InputGraph::Node >	/**< node cross reference: out -> in */
		_node_out2in;
	OutputGraph::ArcMap< InputGraph::Arc >		/**< arc cross reference: out -> in */
		_arc_out2in;

	OutputGraph::NodeMap< OutputGraph::Node >	/**< node maps: nodes -> nodes */
		_send,					/**< will send to this node */
		_receive;				/**< will receive from this node */

	OutputGraph::NodeMap< bool >	/**< node maps: boolean	*/
		_trade;			/**< item will trade */
	OutputGraph::ArcMap< bool >	/**< arc maps: boolean	*/
		_chosen_arc;		/**< want has been chosen */


	/**
	 * @brief Username-to-item map
	 */
	std::multimap< std::string, int > _username_to_item;

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

#if 0
	/**
	 * @brief Export to .dot format.
	 * Exports a graph to a file compatible with
	 * graphviz, to visualize the graph.
	 */
	template < typename DGR >
	static void exportToDot( const DGR & g,
			const typename DGR::template NodeMap< std::string > & name );
#endif
};

#endif /* _MATHTRADER_HPP_ */
