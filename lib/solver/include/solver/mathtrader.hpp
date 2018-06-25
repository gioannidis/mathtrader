/* This file is part of MathTrader++.
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
#ifndef _MATHTRADER_LIB_SOLVER_INCLUDE_SOLVER_MATHTRADER_HPP_
#define _MATHTRADER_LIB_SOLVER_INCLUDE_SOLVER_MATHTRADER_HPP_

#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>

class MathTrader {

public:
	/*! @brief Default constructor.
	 */
	MathTrader();

	/************************
	 * 	GRAPH INPUT	*
	 ************************/

	/*! @name LEMON graph input.
	 *
	 *  Methods to feed a Lemon-Graph-Format (LGF) file
	 *  and construct a LEMON graph.
	 */
	/*! @{ */ // start of group

	/*! @brief Construct LEMON graph from input stream.
	 *
	 *  Constructs the input LEMON graph
	 *  from the given input stream.
	 *  A Lemon-Graph-Format (LGF) file is expected
	 *  to be given.
	 *
	 *  @param[in,out]	is	the input stream to read from
	 */
	void graphReader( std::istream & is = std::cin );

	/*! @brief Construct LEMON graph from input file.
	 *
	 *  Constructs the input LEMON graph
	 *  from the given input stream.
	 *  A Lemon-Graph-Format (LGF) file is expected
	 *  to be given.
	 *  Calls @ref graphReader().
	 *
	 *  @param[in]	file_name	the input file name to read the graph from
	 */
	void graphReader( const std::string & file_name );

	/*! @} */ // end of group

	/************************
	 * 	EXECUTION	*
	 ************************/

	/*! @brief Execute trading algorithm.
	 *
	 *  Executes the trading algorithm by finding an optimal solution
	 *  to the following problem:
	 *  > Given a weighted directed graph ``G=(V,E)`` with weights ``w(E)``,
	 *  > find a set of vertex-disjoint cycles that maximizes
	 *  > the number of covered vertices and minimizes
	 *  > the total edge cost among the set of optimal solutions.
	 *
	 *  In other words, it finds cycles without common vertices
	 *  and maximizes the number of chosen vertices.
	 *  Amongst the optimal solutions, it choses the one
	 *  with the minimum arc cost.
	 */
	void run();

	/************************
	 * 	RESULT GRAPH	*
	 ************************/

	/*! @brief Display trade loops.
	 * Displays the trade loops that have been found
	 * to the given output stream.
	 * @param os output stream (default: stdout)
	 * @return *this
	 */
	const MathTrader & writeResults( std::ostream & os = std::cout ) const ;

	/*! @brief Merge dummies.
	 *
	 * If not run, dummy nodes will be printed by @ref writeResults().
	 * Run to merge them.
	 */
	void mergeDummyItems();

	/************************
	 * 	GRAPH EXPORT	*
	 ************************/

	/*! @name Export input/output graphs.
	 *
	 *  Methods to export the LEMON input or output graphs
	 *  to text files. Supported formats:
	 *
	 *  * ``.dot`` format, compatibile with the ``graphviz`` visualization package
	 *  * ``.lgf`` format, the Lemon-Graph-Format, so that it may be
	 *  	subsequently used by other LEMON-based functions
	 */
	/*! @{ */ // start of group

	/*! @brief Export input graph to stream as ``.dot``.
	 *
	 *  Exports the input graph,
	 *  representing the want lists,
	 *  to the given output stream
	 *  as a ``.dot`` file.
	 *  @param[out]	os	the output stream to export the graph to
	 */
	void exportInputToDot( std::ostream & os = std::cout ) const ;

	/*! @brief Export input graph to file as ``.dot``.
	 *
	 *  Exports the input graph,
	 *  representing the want lists,
	 *  to the given output file
	 *  as a ``.dot`` file.
	 *  If the target file exists, it is overwritten.
	 *  @param[in]	file_name	the file name to export the graph to
	 */
	void exportInputToDot( const std::string & file_name ) const ;

	/*! @brief Export output graph to stream as ``.dot``.
	 *
	 *  Exports the output graph,
	 *  representing the trade cycles,
	 *  to the given output stream
	 *  as a ``.dot`` file.
	 *  @param[out]	os	the output stream to export the graph to
	 */
	void exportOutputToDot( std::ostream & os = std::cout ) const ;

	/*! @brief Export output graph to file as ``.dot``.
	 *
	 *  Exports the output graph,
	 *  representing the trade cycles,
	 *  to the given output file
	 *  as a ``.dot`` file.
	 *  If the target file exists, it is overwritten.
	 *  @param[in]	file_name	the file name to export the graph to
	 */
	void exportOutputToDot( const std::string & file_name ) const ;

	/*! @} */ // end of group


	/************************
	 * 	TRADING OPTIONS	*
	 ************************/

	/*! @name Set trading options.
	 *
	 *  Methods to set the trading options.
	 *  These options affect the execution
	 *  of the algorithm that solves the math trade.
	 *  Trading options must be set before @ref run() is called.
	 */
	/*! @{ */ // start of group

	/*! @brief Select MCF algorithm.
	 *
	 *  Set the minimum cost flow algorithm (MCFA) to be used.
	 *  By default, Network Simplex will be used.
	 *  Accepted string values:
	 *
	 *	 	"NETWORK-SIMPLEX"
	 * 		"COST-SCALING"
	 * 		"CAPACITY-SCALING"
	 * 		"CYCLE-CANCELING"
	 *
	 *  @param[in]	algorithm	the MCFA to be used
	 */
	void setAlgorithm( const std::string & algorithm );

	/*! @brief Set arc cost priorities.
	 *
	 *  Defines how the "cost" of the graph arc cost
	 *
	 *  Each input arc has a specified ``rank`` attribute,
	 *  which is translated to an arc ``cost`` via a function
	 *  ``cost = f(arc)``.
	 *  The priority scheme is the function ``f``.
	 *  If not called, no priorities will be used.<br>
	 *  The following values are accepted:
	 *
	 *  1. ``"LINEAR-PRIORITIES"``, where ``cost = rank``
	 *  2. ``"TRIANGLE-PRIORITIES"``, where ``cost = (rank*(rank+1))/2``
	 *  3. ``"SQUARE-PRIORITIES"``, where ``cost = rank*rank``
	 *  4. ``"SCALED-PRIORITIES"``, not yet implemented
	 *
	 * @param[in]	priorities	the priority to be used
	 */
	void setPriorities( const std::string & priorities );

	/**
	 * @brief Clear priorities.
	 * Clears any previously given priorities.
	 * No priorities will be used.
	 * @return *this
	 */
	MathTrader & clearPriorities();

	/*! @} */ // end of group


	/************************
	 * 	RESULT OPTIONS	*
	 ************************/

	/*! @name Set output options.
	 *
	 *  Methods to set options affecting
	 *  how and which output results and statistics
	 *  will be displayed by @ref writeResults().
	 *  These options affect the execution
	 *  of the algorithm that solves the math trade.
	 *  Trading options must be set before @ref run() is called.
	 */
	/*! @{ */ // start of group

	/*! @brief Hide trade loops.
	 *
	 *  If specified,
	 *  trade loops will not be shown in the results.
	 *  @param[in]	options	value to set
	 *  @return	reference to current object
	 */
	MathTrader & hideLoops( bool option = true );

	/*! @brief Hide non-traded items.
	 *
	 *  If specified,
	 *  items not being traded will not be shown in the results.
	 *  @param[in]	options	value to set
	 *  @return	reference to current object
	 */
	MathTrader & hideNonTrades( bool option = true );

	/*! @brief Hide trade stats.
	 *
	 *  If specified,
	 *  no other trade statistics will be shown in the results,
	 *  apart from trading items.
	 *  @param[in]	options	value to set
	 *  @return	reference to current object
	 */
	MathTrader & hideStats( bool option = true );

	/*! @brief Hide item summary.
	 *
	 *  If specified,
	 *  the items summary will not be shown in the results.
	 *  @param[in]	options	value to set
	 *  @return	reference to current object
	 */
	MathTrader & hideSummary( bool option = true );

	/*! @brief Sort items summary by item name instead of username.
	 *
	 *  If specified,
	 *  items shown in the item summary
	 *  will be sorted by item name, instead of username.
	 *  @note If @ref hideSummary() has been called, no item summary will be shown.
	 *  @param[in]	options	value of option to set
	 *  @return	reference to current object
	 */
	MathTrader & sortByItem( bool option = true );

	/*! @} */ // end of group


	/************************
	 * 	OUTPUT STATS	*
	 ************************/

	/*! @brief Number of trades.
	 *
	 *  Returns the number of trades between items.
	 *
	 *  @return number of trading items
	 */
	unsigned getNumTrades() const ;

	/**
	 * @brief Analyze the graph into strongly connected components.
	 * Decompose the input graph into strongly connected components
	 * and print their respective sizes twice:
	 * once including dummy items, once without.
	 */
	const MathTrader & writeStrongComponents( std::ostream & os = std::cout ) const ;

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
		_in_rank;		/**< rank of want	*/

	/**
	 * @brief Output Graph
	 * Type of Output Graph, member and maps.
	 */
	/**
	 * @brief Convert rank to cost.
	 * Receives the rank of an arc
	 * and generates the cost to be used
	 * for the minimum flow algorithm.
	 * @param rank The rank of the arc.
	 * @param dummy_source Indicates whether the source node is dummy.
	 * @return The corresponding cost.
	 */
	int64_t _getCost( int rank, bool dummy_source = false ) const ;

	/**
	 * @brief Export to .dot format.
	 * Exports a graph to a file compatible with
	 * graphviz, to visualize the graph.
	 */
	template < typename DGR >
	static void _exportToDot( std::ostream & os,
			const DGR & g,
			const std::string & title,
			const typename DGR::template NodeMap< std::string > & node_label );
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

#endif /* include guard */
