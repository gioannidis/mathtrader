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
#include "mathtrader.hpp"

#include <lemon/adaptors.h>
#include <lemon/connectivity.h>
#include <lemon/lgf_reader.h>
#include <stdexcept>
#include <unordered_map>

MathTrader::MathTrader() :
	_priority_scheme( NO_PRIORITIES ),
	_hide_no_trades( false ),
	_name( _input_graph ),
	_username( _input_graph ),
	_send( _input_graph ),
	_receive( _input_graph ),
	_dummy( _input_graph, false ),
	_trade( _input_graph, false ),
	_rank( _input_graph, 0 ),
	_chosen_arc( _input_graph, false )
{
}

MathTrader::~MathTrader() {
}

MathTrader &
MathTrader::graphReader( std::istream & is ) {

	digraphReader( _input_graph, is ).
		nodeMap( "item", _name ).
		nodeMap( "dummy", _dummy ).
		nodeMap( "username", _username ).
		arcMap( "rank", _rank ).
		run();

	/**
	 * Create username-to-item map
	 */
	const InputGraph & g = this->_input_graph;
	for ( InputGraph::NodeIt n(g); n != lemon::INVALID; ++ n ) {
		_username_to_item.emplace( _username[n], g.id(n) );
	}


	return *this;
}

MathTrader &
MathTrader::graphReader( const std::string & fn ) {
	return graphReader( fn.c_str() );
}

MathTrader &
MathTrader::graphReader( const char * fn ) {

	std::filebuf fb;
	fb.open(fn, std::ios::in);
	std::istream is(&fb);
	graphReader(is);
	fb.close();
	return *this;
}

MathTrader &
MathTrader::setPriorities( const std::string & priorities ) {

	typedef std::unordered_map< std::string, PriorityScheme > PrioMap_t;
	PrioMap_t prioMap;

	prioMap.emplace("LINEAR-PRIORITIES", LINEAR_PRIORITIES);
	prioMap.emplace("TRIANGLE-PRIORITIES", TRIANGLE_PRIORITIES);
	prioMap.emplace("SQUARE-PRIORITIES", SQUARE_PRIORITIES);
	prioMap.emplace("SCALED-PRIORITIES", SCALED_PRIORITIES);

	PrioMap_t::const_iterator it = prioMap.find( priorities );
	if ( it == prioMap.end() ) {
		throw std::runtime_error("Invalid priority scheme given");
	}

	_priority_scheme = it->second;

	return *this;
}

MathTrader &
MathTrader::clearPriorities() {
	_priority_scheme = NO_PRIORITIES;
	return *this;
}

MathTrader &
MathTrader::hideNoTrades() {
	_hide_no_trades = true;
	return *this;
}

MathTrader &
MathTrader::showNoTrades() {
	_hide_no_trades = false;
	return *this;
}

void
MathTrader::run() {

#if 1
	const InputGraph & input_graph = this->_input_graph;

	/**
	 * Find strongly connected components
	 */
	InputGraph::NodeMap< int > component_id( input_graph );
	int n_strong = lemon::stronglyConnectedComponents( input_graph, component_id );
	std::cout << "strongly: " << n_strong << std::endl;

	/**
	 * Iterate all strongly connected components.
	 * Create filter map.
	 * Run flow algorithm.
	 */
	std::vector< std::unique_ptr< BoolMap > > component_vector;
	component_vector.reserve( n_strong );

	/**
	 * Initialize the vector of bool maps
	 * per strongly connected component.
	 */
	for ( int i = 0; i < n_strong; i ++ ) {
		component_vector.push_back( std::unique_ptr< BoolMap >(new BoolMap(input_graph, false)) );
	}

	/**
	 * Iterate all nodes.
	 * For each node get the component_id.
	 * Get the corresponding component bool-vector: 0 .. n_strong - 1
	 * Set the node map to true.
	 */
	for ( InputGraph::NodeIt n(input_graph); n != lemon::INVALID; ++ n ) {
		BoolMap & filter = *(component_vector[ component_id[n] ]);
		filter[n] = true;
	}

	for ( unsigned i = 0; i < component_vector.size(); ++ i ) {
		const BoolMap & filter = *(component_vector[i]);
		this->_runFlowAlgorithm( filter );
	}
#else
	this->_runFlowAlgorithm();
#endif
}

MathTrader &
MathTrader::processResults() {

	/**
	 * Merge dummy nodes.
	 */
	this->_mergeDummies();

	return *this;
}

const MathTrader &
MathTrader::printResults( std::ostream & os ) const {

	/**
	 * FILTERS
	 * 1. Hide dummies.
	 * 2. Hide non-chosen arcs.
	 *    Graph becomes a forest of cycles.
	 * 3. Hide non-trading nodes. (optional)
	 */
	const InputGraph & result_graph = this->_input_graph;

	/**
	 * Hide all dummy nodes.
	 * Nodes between dummies should be already merged.
	 */
	auto const no_dummies = notMap(this->_dummy);
	auto const no_dummies_graph = filterNodes( result_graph, no_dummies );

	/**
	 * Hide all non-chosen arcs.
	 * This graph is now a forest of cycles.
	 */
	auto const cycle_forest = filterArcs( no_dummies_graph, this->_chosen_arc );

	/**
	 * Show/Hide no trades.
	 * If we hide trades, filter (show) only trading nodes.
	 * Else, filter all nodes.
	 */
	BoolMap filter( _input_graph, true );
	if ( this->_hide_no_trades ) {
		mapCopy( _input_graph, _trade, filter );
	}

	auto const output_graph = filterNodes( cycle_forest, filter );
	typedef decltype(output_graph) OutputGraph;

	/**
	 * TRADE LOOPS
	 */
	OutputGraph::NodeMap< int > component_id( output_graph );
	const int n_cycles = stronglyConnectedComponents( output_graph, component_id );
	int counted_cycles = 0;

	/**
	 * For each trade cycle: find a starting node.
	 */
	std::vector< int > cycle_start_vec( n_cycles, -1 );

	for ( OutputGraph::NodeIt n(output_graph); (n != lemon::INVALID) && (counted_cycles < n_cycles); ++ n ) {

		const int cycle_id = component_id[n];
		int & cycle_start = cycle_start_vec[ cycle_id ];

		if ( cycle_start < 0 ) {
			cycle_start = output_graph.id(n);
			++ counted_cycles;
		}
	}
	os << "n_cycles: " << n_cycles << " counted_cycles: " << counted_cycles
		<< std::endl;

	const int total_trades = countArcs( output_graph );
	os << "trades: " << total_trades << std::endl;

	/**
	 * Users trading
	 */
	std::unordered_map< std::string, int > users_trading;

	/**
	 * For each trade cycle: print it
	 * Start from @cycle_start and continue
	 * until the next node is the cycle start.
	 * This will also print no-trades (unconnected nodes)
	 * unless the moderator has chosen not to show no-trades.
	 */
	for ( int cycle_start : cycle_start_vec ) {

		const OutputGraph & g = output_graph;
		const auto start_node  = g.nodeFromId( cycle_start );

		if ( this->_trade[start_node] ) {

			auto cur_node = start_node;
			do {
				auto it = users_trading.emplace( _username[cur_node], 0 );
				int & count = it.first->second;
				++ count;

				auto const next_node = _receive[cur_node];

				os << "(" << _username[cur_node] << ") "
					<< _name[cur_node] << "\t\t"
					<< "receives (" << _username[next_node] << ") "
					<< _name[next_node]
					<< std::endl;

				cur_node = next_node;

			} while ( cur_node != start_node );
		} else {
			os << _name[start_node] << "\t\t"
				<< "does not trade"
				<< std::endl;
		}
		os << std::endl;
	}
	os << "Users trading: " << users_trading.size() << std::endl;

	/**
	 * Print item summary,
	 * sorted by username.
	 */
	os << "ITEM SUMMARY (" << total_trades << " total trades):" << std::endl;
	os << std::endl;

	for ( auto username_map : _username_to_item ) {

		const InputGraph::Node & n = _input_graph.nodeFromId(username_map.second);
		if ( !_dummy[n] ) {
			if ( _trade[n] ) {
				os << "(" << _username[n] << ") "
					<< _name[n]
					<< "\t"
					<< "receives "
					<< "(" << _username[ _receive[n] ] << ") "
					<< _name[ _receive[n] ]
					<< "\t"
					<< "and sends to "
					<< "(" << _username[ _send[n] ] << ") "
					<< _name[ _send[n] ]
					<< std::endl;
			} else if ( !_hide_no_trades ) {
				os << "(" << _username[n] << ") "
					<< _name[n]
					<< "\t"
					<< "does not trade"
					<< std::endl;
			}
		}
	}

	return *this;
}

const MathTrader &
MathTrader::printResults( const std::string & fn ) const {
	return printResults( fn.c_str() );
}

const MathTrader &
MathTrader::printResults( const char * fn ) const {

	std::filebuf fb;
	fb.open(fn, std::ios::out);
	std::ostream os(&fb);
	printResults(os);
	fb.close();
	return *this;
}


void
MathTrader::_runFlowAlgorithm() {
	const InputGraph &g = _input_graph;
	BoolMap all_pass(g,true);
	_runFlowAlgorithm(all_pass);
}

void
MathTrader::_runFlowAlgorithm( const BoolMap & filter ) {
	static int n_strong = 0;
	n_strong ++ ;

	const InputGraph & input_graph = this->_input_graph;
	/**
	 * Graph -> Filter Nodes -> Split Direct
	 * 	-> Split Undirect -> Reverse bind edges.
	 * SplitNodes splits each node v to v-out and v-in.
	 * Each arc v -> u becomes v-out -> u-in.
	 *
	 * It also adds bind arcs: v-in -> v-out.
	 * We want to have bind arcs with the reverse direction: v-out -> v-in.
	 *
	 * Therefore, we make the graph undirected and create a direction map
	 * to reverse the direction of the bind edges;
	 * then we make it directed again.
	 *
	 * All node & arc maps are inter-compatible.
	 */

	/**
	 * Filter out the given strongly connected component.
	 */
	typedef lemon::FilterNodes< const InputGraph, const BoolMap > StronglyConnected;
	StronglyConnected strong_graph = filterNodes( input_graph, filter );

	/**
	 * Directed split graph.
	 * - Typedefs
	 * - Object
	 * - Maps
	 */
	typedef lemon::SplitNodes< StronglyConnected > SplitDirect;
	SplitDirect split_graph( strong_graph );

	SplitDirect::NodeMap< bigint_t > supply_map( split_graph );
	SplitDirect::ArcMap< bigint_t > capacity_map( split_graph, 1 ), cost_map( split_graph );

	/**
	 * Undirected split graph.
	 * - Typedefs
	 * - Object
	 * - Reverse map
	 */
	typedef lemon::Undirector< SplitDirect > SplitUndirect;
	SplitUndirect split_undirect( split_graph );

	typedef SplitUndirect::EdgeMap< bool > ReverseMap;
	ReverseMap reverse_map( split_undirect );

	/**
	 * Oriented (fixed) split graph.
	 * - Typedefs
	 * - Object
	 */
	typedef lemon::Orienter< const SplitUndirect, ReverseMap > SplitOrient;
	SplitOrient split_orient( split_undirect, reverse_map );

	/**
	 * Iterate nodes of SplitDirect.
	 * In-nodes have a supply of +1
	 * Out-nodes have a supply of -1
	 */
	for ( SplitDirect::NodeIt n( split_graph ); n != lemon::INVALID; ++ n ) {

		supply_map[n] = (split_graph.outNode(n)) ? +1 : -1;
	}

	/**
	 * Iterate arcs of SplitDirect.
	 * All arcs have a capacity of 1.
	 * Original capacity have a cost of 1.
	 * Bind (virtual/self) capacities have a cost of INF.
	 */
	for ( SplitDirect::ArcIt a( split_graph ); a != lemon::INVALID; ++ a ) {

		int rank = _rank[a];
		cost_map[a] = (split_graph.origArc(a)) ? _getCost(rank) : 1e9 ;
		reverse_map[a] = (split_graph.origArc(a)) ? true : false;
	}


	/**
	 * Define and apply the solver
	 */
	typedef lemon::NetworkSimplex< SplitOrient, bigint_t > FlowAlgorithm;
	FlowAlgorithm trade_solver( split_orient );
	FlowAlgorithm::ProblemType rv = trade_solver.
		upperMap( capacity_map ).
		costMap( cost_map ).
		supplyMap( supply_map ).
		run();

	/**
	 * Check if perfect match has been found.
	 */
	if ( rv != FlowAlgorithm::ProblemType::OPTIMAL ) {
		throw std::runtime_error("No optimal solution found");
	}

	/**
	 * Get the flow map
	 */
	SplitDirect::ArcMap< bigint_t > flow_map( split_graph );
	trade_solver.flowMap( flow_map );

	/**
	 * Map it back to the original graph.
	 */
	uint64_t total_cost = 0;
	int trades = 0;
	static int total_trades = 0;
	const StronglyConnected & g = strong_graph;
	for ( StronglyConnected::ArcIt a(g); a != lemon::INVALID; ++a ) {

		auto const & want_arc = split_graph.arc(a);
		const bool chosen = flow_map[ want_arc ];

		if ( chosen ) {

			if ( this->_chosen_arc[a] ) {
				throw std::runtime_error("Arc already chosen");
			}
			this->_chosen_arc[a] = chosen;

			total_cost += cost_map[ want_arc ];

			const InputGraph::Node &receiver = g.source(a), &sender = g.target(a);

			if ( ! _trade[receiver] ) {
				_trade[receiver] = true;
				_receive[ receiver ] = sender;
				_send[ sender ] = receiver;
			} else {
				throw std::runtime_error("Multiple trades for item "
						+ _name[ receiver ]);
			}
			if ( !_dummy[receiver] ) {
				trades ++ ;
			}
		}
	}
	int not_trading = 0;
	int trading = 0;
	int dummies_trading = 0;
	for ( StronglyConnected::NodeIt n(g); n != lemon::INVALID; ++ n ) {
		auto const & self_arc = split_graph.arc(n);
		const bool chosen = flow_map[ self_arc ];
		if ( chosen ) {
			not_trading ++ ;
			total_cost += cost_map[ self_arc ];
			if ( _trade[n] ) {
				throw std::runtime_error("Trading, but self arc chosen");
			}
		} else {
			if ( ! _trade[n] ) {
				throw std::runtime_error("Not trading, but self arc not chosen");
			}
			if ( !_dummy[n] ) {
				trading ++ ;
			} else {
				dummies_trading ++ ;
			}
		}
	}
	total_trades += trades;
	std::cout << "n_strong: " << n_strong
		<< " trades: " << trades
		<< " total_trades: " << total_trades
		<< " trading: " << trading
		<< " not_trading: " << not_trading
		<< " dummies_trading: " << dummies_trading
		<< " cost: " << total_cost
		<< std::endl;
	if ( n_strong == 218 && false ) {
		auto g = filterNodes(strong_graph,_trade);
		digraphWriter(g).
			nodeMap("trade", _trade).
			nodeMap("name", _name).
			nodeMap("receive", composeMap(_name,_receive)).
			nodeMap("send", composeMap(_name,_send)).
			arcMap("chosen", _chosen_arc).
			skipArcs().
			run();
	}
}

template < typename DGR >
void
MathTrader::exportToDot( const DGR & g,
		const typename DGR::template NodeMap< std::string > & name ) {

	std::cout << "digraph G {" << std::endl;
	for ( typename DGR::template NodeIt n(g); n != lemon::INVALID; ++n ) {
		std::cout << "\t"
			<< "n" << g.id(n)
			<< " [label=\"" << name[n] << "\"];"
			<< std::endl;
	}
	for ( typename DGR::template ArcIt a(g); a != lemon::INVALID; ++a ) {
		std::cout << "\t"
			<< "n" << g.id( g.source(a) )
			<< " -> " << "n" << g.id(g.target(a))
			<< std::endl;
	}
	std::cout << "}" << std::endl;
}

/**
 * Source: https://www.boardgamegeek.com/wiki/page/TradeMaximizer#toc4
 */
MathTrader::bigint_t
MathTrader::_getCost( int rank ) const {

	switch ( this->_priority_scheme ) {
		case NO_PRIORITIES:
			return 1;
			break;
		case LINEAR_PRIORITIES:
			return rank;
			break;
		case TRIANGLE_PRIORITIES:
			return (rank*(rank+1))/2;
			break;
		case SQUARE_PRIORITIES:
			return (rank*rank);
		case SCALED_PRIORITIES:
		default:
			throw std::logic_error("No implementation of chosen priority scheme");
			break;
	}
	return -1;
}

MathTrader &
MathTrader::_mergeDummies() {

	InputGraph & g = this->_input_graph;
	for ( InputGraph::NodeIt n(g); n != lemon::INVALID; ++ n ) {

		if ( _dummy[n] ) {

			/**
			 * Update _send & _receive maps, if needed.
			 */
			if ( _trade[n] ) {
				const InputGraph::Node &receiver = _send[n], &sender = _receive[n];
				_receive[ receiver ] = sender;
				_send[ sender ] = receiver;

				/**
				 * Add corresponding arc.
				 * The rank value is irrelevant.
				 */
				auto arc = g.addArc( receiver, sender );
				this->_rank[arc] = 0;
				this->_chosen_arc[arc] = true;
			}
		}
	}

	return *this;
}
