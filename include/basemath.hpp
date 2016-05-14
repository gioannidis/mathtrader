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
#ifndef _BASEMATH_HPP_
#define _BASEMATH_HPP_

#include <lemon/smart_graph.h>

class BaseMath {

public:
	/**
	 * @brief Constructor.
	 * Details.
	 */
	BaseMath();

	/**
	 * @brief Destructor.
	 * Details.
	 */
	virtual ~BaseMath();

	/**
	 * @brief Read graph from istream.
	 * Constructs the input trade graph,
	 * from the given input stream.
	 * @param is input stream (default: stdin)
	 * @return *this
	 */
	BaseMath & graphReader( std::istream & is = std::cin );

	/**
	 * @brief Read graph from file.
	 * Constructs the input trade graph,
	 * from the given file.
	 * @param fn file name
	 * @return *this
	 */
	BaseMath & graphReader( const std::string & fn );

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
	BaseMath & setPriorities( const std::string & priorities );

	/**
	 * @brief Clear priorities.
	 * Clears any previously given priorities.
	 * No priorities will be used.
	 * @return *this
	 */
	BaseMath & clearPriorities();

	/**
	 * @brief Run the BaseMath.
	 * Calls the runnable method.
	 * This is implementation-dependent
	 * of the child classes.
	 */
	virtual void run() = 0;

	/**
	 * @brief Export input graph to dot format.
	 * Writes the input graph
	 * to the given output stream as .dot format.
	 * @param os output stream (default: stdout)
	 * @return *this
	 */
	const BaseMath & exportInputToDot( std::ostream & os = std::cout ) const ;

	/**
	 * @brief Export input graph to dot format.
	 * Writes the input graph
	 * to the given file as .dot format.
	 * @param fn file name to write to
	 * @return *this
	 */
	const BaseMath & exportInputToDot( const std::string & fn ) const ;

protected:
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
			const typename DGR::template NodeMap< std::string > & name );

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
};


/************************************//*
 * 	PRIVATE TEMPLATES - Utilities
 **************************************/

template < typename DGR >
void
BaseMath::_exportToDot( std::ostream & os,
		const DGR & g,
		const std::string & title,
		const typename DGR::template NodeMap< std::string > & name ) {

	os << "digraph "
		<< title
		<< " {"
		<< std::endl;

	for ( typename DGR::NodeIt n(g); n != lemon::INVALID; ++n ) {
		os << "\t"
			<< "n" << g.id(n)
			<< " [label=\"" << name[n] << "\"];"
			<< std::endl;
	}
	for ( typename DGR::ArcIt a(g); a != lemon::INVALID; ++a ) {
		os << "\t"
			<< "n" << g.id( g.source(a) )
			<< " -> " << "n" << g.id(g.target(a))
			<< std::endl;
	}
	os << "}" << std::endl;
}

#endif /* _BASEMATH_HPP_ */
