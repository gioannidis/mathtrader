/* This file is part of MathTrader++, a C++ utility
 * for finding, on a directed graph whose arcs have costs,
 * a set of vertex-disjoint cycles that maximizes the number
 * of covered vertices as a first priority
 * and minimizes the total cost as a second priority.
 *
 * Copyright (C) 2018 George Ioannidis
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

#include "basemath.hpp"
#include <lemon/list_graph.h>

class MathTrader : public BaseMath {

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
	 * @brief MathTrade algorithm.
	 * Runs the MathTrade algorithm.
	 */
	void run();

	/**
	 * @brief Merge dummies.
	 * If not run, dummy nodes will be printed
	 * by writeResults().
	 * Run to merge them.
	 * @return *this
	 */
	MathTrader & mergeDummyItems();

	/**
	 * @brief Hide loops.
	 * Trade loops will not be shown
	 * in the results.
	 * @param option Set the option (default: true)
	 * @return *this
	 */
	MathTrader & hideLoops( bool option = true );

	/**
	 * @brief Hide non-trades.
	 * Items not being traded will not be shown
	 * in the results.
	 * @param option Set the option (default: true)
	 * @return *this
	 */
	MathTrader & hideNonTrades( bool option = true );

	/**
	 * @brief Hide stats.
	 * No statistics will be shown
	 * in the results, except trading items.
	 * @param option Set the option (default: true)
	 * @return *this
	 */
	MathTrader & hideStats( bool option = true );

	/**
	 * @brief Hide item summary.
	 * Items summary will not be shown
	 * in the results.
	 * @param option Set the option (default: true)
	 * @return *this
	 */
	MathTrader & hideSummary( bool option = true );

	/**
	 * @brief Sort items summary by item name.
	 * Items shown in the item summary
	 * will be sorted by item name, instead of username.
	 * If hideSummary() has been called,
	 * this option has no effect.
	 * @param option Set the option (default: true)
	 * @return *this
	 */
	MathTrader & sortByItem( bool option = true );

	/**
	 * @brief Display trade loops.
	 * Displays the trade loops that have been found
	 * to the given output stream.
	 * @param os output stream (default: stdout)
	 * @return *this
	 */
	const MathTrader & writeResults( std::ostream & os = std::cout ) const ;

	/**
	 * @brief Export output graph to dot format.
	 * Writes the results graph
	 * to the given output stream as .dot format.
	 * @param os output stream (default: stdout)
	 * @return *this
	 */
	const MathTrader & exportOutputToDot( std::ostream & os = std::cout ) const ;

	/**
	 * @brief Export output graph to dot format.
	 * Writes the results graph
	 * to the given file as .dot format.
	 * @param fn file name to write to
	 * @return *this
	 */
	const MathTrader & exportOutputToDot( const std::string & fn ) const ;

private:
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

	MCFA _mcfa;

	/**
	 * @brief Output Options
	 * Options that will determine what should be printed
	 * or not by writeResults().
	 * Default value is false.
	 */
	bool _hide_loops;	/**< hide trade loops */
	bool _hide_non_trades;	/**< hide non-trading items */
	bool _hide_stats;	/**< hide statistics, apart from items traded */
	bool _hide_summary;	/**< hide item summary */
	bool _sort_by_item;	/**< sort item summary by item name;
				  * _hide_summary must be false
				  */

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
		_trade;			/**< item will trade	*/
	OutputGraph::ArcMap< int >	/**< arc maps: integer	*/
		_out_rank;		/**< rank of want	*/
	OutputGraph::ArcMap< bool >	/**< arc maps: boolean	*/
		_chosen_arc;		/**< want has been chosen */


	/**
	 * @brief Solve the trade; maximize trading items
	 * Runs the math trading algorithm.
	 * The goal is to maximize the trading items.
	 */
	void _runMaximizeTrades();

	/**
	 * @brief Run math trade algorithm.
	 * Runs the math trade algorithm on a given map and provides
	 * the flow map.
	 * @param g the trade graph
	 * @param supply the supply node map
	 * @param capacity the capacity arc map
	 * @param cost the cost arc map
	 * @param flow the flow arc map
	 */
	template < typename DGR >
	void _runFlowAlgorithm( const DGR & g,
			const typename DGR::template NodeMap< int64_t > & supply,
			const typename DGR::template  ArcMap< int64_t > & capacity,
			const typename DGR::template  ArcMap< int64_t > & cost,
			      typename DGR::template  ArcMap< int64_t > & flow );
};

#endif /* _MATHTRADER_HPP_ */
