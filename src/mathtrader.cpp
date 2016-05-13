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
#include "algowrapper.hpp"

#include <iomanip>
#include <lemon/adaptors.h>
#include <lemon/connectivity.h>
#include <lemon/lgf_reader.h>
#include <list>
#include <stdexcept>
#include <unordered_map>

/* Algorithms */
#include <lemon/capacity_scaling.h>
#include <lemon/cost_scaling.h>
#include <lemon/cycle_canceling.h>
#include <lemon/network_simplex.h>


/************************************//*
 * 	PUBLIC METHODS - CONSTRUCTORS
 **************************************/

MathTrader::MathTrader() :
	/* options */
	_priority_scheme( NO_PRIORITIES ),	/**< Option: priorities */
	_mcfa( NETWORK_SIMPLEX ),		/**< Option: algorithm 	*/
	_hide_non_trades( false ),		/**< Option: hide non-trades */

	/* input graph maps */
	_name( _input_graph ),
	_username( _input_graph ),
	_dummy( _input_graph, false ),
	_in_rank( _input_graph, 0 ),

	/* input-output (cross) references */
	_node_in2out( _input_graph ),
	_arc_in2out( _input_graph ),
	_node_out2in( _output_graph ),
	_arc_out2in( _output_graph ),

	/* output graph maps */
	_send( _output_graph ),
	_receive( _output_graph ),
	_trade( _output_graph, false ),
	_out_rank( _output_graph ),
	_chosen_arc( _output_graph, false )
{
}

MathTrader::~MathTrader() {
}


/************************************//*
 * 	PUBLIC METHODS - INPUT
 **************************************/

MathTrader &
MathTrader::graphReader( std::istream & is ) {

	/**
	 * The only instance where we are allowed to modify
	 * the input graph.
	 */
	digraphReader( const_cast< InputGraph & >(_input_graph), is ).
		nodeMap( "item", _name ).
		nodeMap( "dummy", _dummy ).
		nodeMap( "username", _username ).
		arcMap( "rank", _in_rank ).
		run();

	/**
	 * Create username-to-item map
	 */
	_username_to_item.clear();
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


/************************************//*
 * 	PUBLIC METHODS - OPTIONS
 **************************************/

MathTrader &
MathTrader::setPriorities( const std::string & priorities ) {

	typedef std::unordered_map< std::string, PriorityScheme > PrioMap_t;
	static const PrioMap_t prioMap = {
		{"LINEAR-PRIORITIES", LINEAR_PRIORITIES},
		{"TRIANGLE-PRIORITIES", TRIANGLE_PRIORITIES},
		{"SQUARE-PRIORITIES", SQUARE_PRIORITIES},
		{"SCALED-PRIORITIES", SCALED_PRIORITIES},
	};

	auto const & it = prioMap.find( priorities );
	if ( it == prioMap.end() ) {
		throw std::runtime_error("Invalid priority scheme given: "
				+ priorities);
	}

	_priority_scheme = it->second;

	return *this;
}

MathTrader &
MathTrader::setAlgorithm( const std::string & algorithm ) {

	typedef std::unordered_map< std::string, MCFA > AlgoMap_t;
	static const AlgoMap_t algoMap = {
		{"NETWORK-SIMPLEX", NETWORK_SIMPLEX},
		{"COST-SCALING", COST_SCALING},
		{"CAPACITY-SCALING", CAPACITY_SCALING},
		{"CYCLE-CANCELING", CYCLE_CANCELING},
	};

	auto const & it = algoMap.find( algorithm );
	if ( it == algoMap.end() ) {
		throw std::runtime_error("Invalid algorithm given: "
				+ algorithm);
	}

	_mcfa = it->second;

	return *this;
}

MathTrader &
MathTrader::clearPriorities() {
	_priority_scheme = NO_PRIORITIES;
	return *this;
}

MathTrader &
MathTrader::hideNonTrades() {
	_hide_non_trades = true;
	return *this;
}

MathTrader &
MathTrader::showNonTrades() {
	_hide_non_trades = false;
	return *this;
}


/************************************//*
 * 	PUBLIC METHODS - RUNNABLE
 **************************************/

void
MathTrader::run() {

	/**
	 * Copy input to output,
	 * as we will never modify the input.
	 */
	digraphCopy( _input_graph, _output_graph).
		nodeRef( _node_in2out ).
		arcRef( _arc_in2out ).
		nodeCrossRef( _node_out2in ).
		arcCrossRef( _arc_out2in ).
		run();

	mapCopy( _output_graph,
			composeMap(_in_rank, _arc_out2in),
			_out_rank);

	this->_runFlowAlgorithm();
}


/************************************//*
 * 	PUBLIC METHODS - OUTPUT
 **************************************/

MathTrader &
MathTrader::mergeDummyItems() {

	OutputGraph & g = this->_output_graph;
	OutputGraph::NodeMap< bool > iterated(g,false);

	/**
	 * List to mark new arcs to add
	 */
	typedef struct NewArc_s {
		const OutputGraph::Node *s, *t;
		int rank;

		/**
		 * Constructor
		 */
		NewArc_s( const OutputGraph::Node *s_,
				const OutputGraph::Node *t_,
				int rank_ ) :
			s( s_ ),
			t( t_ ),
			rank( rank_ ) {}

	} NewArc_t;
	std::list< NewArc_t > arcs_to_add;

	/**
	 * List to mark nodes for deletion.
	 */
	std::list< int > id_to_delete;

	/**
	 * Create static arc lookup to quickly find the arcs
	 * between two nodes.
	 * Node/Arc operations will be applied afterwards,
	 * therefore we can use the static lookup.
	 */
	lemon::ArcLookUp< OutputGraph > arc_lookup(g);

	/**
	 * Iterate all nodes and check if they are dummies.
	 */
	for ( OutputGraph::NodeIt n(g); n != lemon::INVALID; ++ n ) {

		/**
		 * Node of input graph.
		 * The _dummy map uses INPUT nodes.
		 */
		auto const & n_i = _node_out2in[ n ];

		/**
		 * Dummy? Mark for deletion in any case, trading or not.
		 * All nodes will be parsed up to this point.
		 */
		if ( _dummy[n_i] ) {
			id_to_delete.push_back( g.id(n) );
		}

		/**
		 * Parse a dummy if it's trading and it hasn't been iterated yet.
		 * A dummy may have been already iterated if it's part of a larger dummy chain.
		 * Some users may specify multiple dummies in a chain, like A -> D1 -> D2 -> B.
		 */
		if ( _dummy[n_i] && _trade[n] && !iterated[n] ) {

			iterated[n] = true;

			/**
			 * Pointers to sender and receiver node.
			 * We will go forwards/backwards
			 * until a non-dummy is found
			 * or we detect a cycle.
			 * Cycle begins from @start.
			 */
			const OutputGraph::Node *receiver = &n, *sender = &n;
			const int start = g.id(n);

			/**
			 * Move to next receiver/sender until a non-dummy is found
			 * or until we detect a cycle of dummies;
			 * some users define dummy cycles like D1 -> D2 -> D1
			 * and the algorithm might have chosen them.
			 * Mark the iterated nodes as "iterated" in the process,
			 * so that the running time of this method is O(V).
			 */
			/* Do for receiver node */
			do {
				receiver = &( _send[*receiver] );
				iterated[*receiver] = true;
			}
			while (( receiver != NULL ) && ( _dummy[_node_out2in[*receiver]] )
					&& ( g.id(*receiver) != start ));

			/* Do for sender node */
			do {
				sender = &( _receive[*sender] );
				iterated[*sender] = true;
			}
			while (( sender != NULL ) && ( _dummy[_node_out2in[*sender]] )
					&& ( g.id(*sender) != start ));

			/**
			 * Found a cycle of dummies? Ignore it.
			 */
			if (( sender != NULL ) && ( receiver != NULL )
					&& ( g.id(*sender) != start ) && ( g.id(*receiver) != start )) {

				/**
				 * First, calculate the rank
				 * between receiver -> D1 -> D2 -> ... -> DN -> sender.
				 */
				int rank = -1;
				const OutputGraph::Node
					*prev = receiver,
					*next = prev;

				/**
				 * Begin from @receiver.
				 * Continue until reaching @sender.
				 * Keep track of @prev node and @next node
				 * to quickly find the arcs in between.
				 */
				while ( next != sender ) {
					next = &(_receive[ *next ]);
					auto const & arc = arc_lookup( *prev, *next );

					if ( arc == lemon::INVALID ) {
						throw std::runtime_error("Arc not found "
								"between items "
								+ _name[_node_out2in[*prev]]
								+ " and "
								+ _name[_node_out2in[*next]]);
					}

					int cur_rank = _out_rank[arc];
					if ( rank < 0 || cur_rank < rank ) {
						rank = cur_rank;
					}

					prev = next;
				}

				/**
				 * We have found the real items of this chain.
				 * Updated send/receive maps.
				 */
				_receive[ *receiver ] = *sender;
				_send[ *sender ] = *receiver;

				/**
				 * Schedule corresponding arc to be added.
				 */
				arcs_to_add.push_back(NewArc_t(receiver,sender,rank));
			}
		}
	}

	/**
	 * Add new arcs
	 */
	for ( auto const & new_arc : arcs_to_add ) {
		auto arc = g.addArc( *(new_arc.s), *(new_arc.t) );
		this->_out_rank[arc] = new_arc.rank;
		this->_chosen_arc[arc] = true;
	}

	/**
	 * Delete scheduled nodes.
	 */
	for ( int id : id_to_delete ) {
		g.erase(g.nodeFromId(id));
	}

	return *this;
}


const MathTrader &
MathTrader::tradeLoops( std::ostream & os ) const {

#define TABWIDTH 50

	/**
	 * FILTERS
	 * 1. Hide dummies.
	 * 2. Hide non-chosen arcs.
	 *    Graph becomes a forest of cycles.
	 * 3. Hide non-trading nodes. (optional)
	 */
	auto const & result_graph = this->_output_graph;

	/**
	 * Hide all non-chosen arcs.
	 * This graph is now a forest of cycles.
	 */
	auto const cycle_forest = filterArcs( result_graph, this->_chosen_arc );

	/**
	 * Show/Hide no trades.
	 * If we hide trades, filter (show) only trading nodes.
	 * Else, filter all nodes.
	 */
	decltype(_trade) filter( result_graph, true );
	if ( this->_hide_non_trades ) {
		mapCopy( result_graph, _trade, filter );
	}

	auto const final_graph = filterNodes( cycle_forest, filter );
	typedef decltype(final_graph) FinalGraph;

	/**
	 * TRADE LOOPS
	 */
	FinalGraph::NodeMap< int > component_id( final_graph );
	const int n_cycles = stronglyConnectedComponents( final_graph, component_id );
	int counted_cycles = 0;

	/**
	 * For each trade cycle: find a starting node.
	 */
	std::vector< int > cycle_start_vec( n_cycles, -1 );

	for ( FinalGraph::NodeIt n(final_graph); (n != lemon::INVALID) && (counted_cycles < n_cycles); ++ n ) {

		const int cycle_id = component_id[n];
		int & cycle_start = cycle_start_vec[ cycle_id ];

		if ( cycle_start < 0 ) {
			cycle_start = result_graph.id(n);
			++ counted_cycles;
		}
	}
	os << "n_cycles: " << n_cycles << " counted_cycles: " << counted_cycles
		<< std::endl;

	const int total_trades = countArcs( final_graph );
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

		auto const & g = final_graph;
		const auto start_node  = g.nodeFromId( cycle_start );

		if ( this->_trade[start_node] ) {

			auto cur_node = start_node;
			do {
				auto it = users_trading.emplace( _username[_node_out2in[cur_node]], 0 );
				int & count = it.first->second;
				++ count;

				auto const next_node = _receive[cur_node];

				os << std::left
					<< std::setw(TABWIDTH)
					<< "(" + _username[_node_out2in[cur_node]] + ") "
					+ _name[_node_out2in[cur_node]]
					<< "receives (" + _username[_node_out2in[next_node]] + ") "
					+ _name[_node_out2in[next_node]]
					<< std::endl;

				cur_node = next_node;

			} while ( cur_node != start_node );
		} else {
			os << std::left
				<< std::setw(TABWIDTH)
				<< "(" + _username[_node_out2in[start_node]] + ") "
				+ _name[_node_out2in[start_node]]
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

		/**
		 * NOTE: username_map contains the INPUT graph ids.
		 */
		const InputGraph::Node & ni = _input_graph.nodeFromId(username_map.second);
		const FinalGraph::Node & n  = _node_in2out[ni];

		if ( _trade[n] ) {

			os << std::left
				<< std::setw(TABWIDTH)
				<< "(" + _username[ni] + ") "
				+ _name[ni]
				<< std::setw(TABWIDTH)
				<< "receives ("
				+ _username[ _node_out2in[_receive[n]] ] + ") "
				+ _name[ _node_out2in[_receive[n]] ]
				<< "and sends to ("
				+ _username[ _node_out2in[_send[n]] ] + ") "
				+ _name[ _node_out2in[_send[n]] ]
				<< std::endl;

		} else if ( !_hide_non_trades ) {

			os << std::left
				<< std::setw(TABWIDTH)
				<< "(" + _username[ni] + ") "
				+ _name[ni]
				<< "does not trade"
				<< std::endl;
		}
	}

	int64_t total_cost = 0;
	for ( FinalGraph::ArcIt a(final_graph); a != lemon::INVALID; ++ a ) {
		total_cost += _getCost(_out_rank[a]);
	}
	os << "Total cost = " << total_cost << std::endl;

	return *this;
#undef TABWIDTH
}


/************************************//*
 * 	PRIVATE METHODS - Flows
 **************************************/

void
MathTrader::_runFlowAlgorithm() {

	typedef OutputGraph StartGraph;
	const StartGraph & start_graph = this->_output_graph;

	/**
	 * Graph -> Split Direct	[split the nodes]
	 * 	 -> Split Undirect	[make graph undirect]
	 * 	 -> Reverse bind edges	[make graph direct again; reverse self-edges]
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
	 * Directed split graph.
	 * - Typedefs
	 * - Object
	 * - Maps
	 */
	typedef lemon::SplitNodes< StartGraph > SplitDirect;
	SplitDirect split_graph( start_graph );

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

	SplitOrient::NodeMap< int64_t > supply_map( split_orient );
	SplitOrient::ArcMap< int64_t > capacity_map( split_orient, 1 ), cost_map( split_orient );

	/**
	 * Iterate nodes of original graph.
	 * Get corresponding bind-arcs.
	 * Cost: c >> 1, but lower if it's a dummy node, by 1-2 orders of magnitude,
	 * so as to inherently prefer a dummy self-arc over a real item's self-arc.
	 * Mark self-arc to reverse its direction.
	 */
	for ( StartGraph::NodeIt n(start_graph); n != lemon::INVALID; ++ n ) {

		auto const & self_arc = split_graph.arc(n);
		cost_map[ self_arc ] = ( _dummy[_node_out2in[n]] ) ? 1e7 : 1e9;
		reverse_map[ self_arc ] = false;
	}

	/**
	 * Iterate arcs of original graph.
	 * Get corresponding match-arcs.
	 * Cost: depends on rank and priority scheme.
	 * Mark match-arc to keep its direction.
	 */
	for ( StartGraph::ArcIt a(start_graph); a != lemon::INVALID; ++ a ) {

		auto const & match_arc = split_graph.arc(a);
		const int rank = _out_rank[a];

		cost_map[ match_arc ] = _getCost(rank);
		reverse_map[ match_arc ] = true;
	}

	/**
	 * Iterate nodes of SplitDirect.
	 * In-nodes have a supply of +1
	 * Out-nodes have a supply of -1
	 */
	for ( SplitDirect::NodeIt n( split_graph ); n != lemon::INVALID; ++ n ) {

		supply_map[n] = (split_graph.outNode(n)) ? +1 : -1;
	}


	/**
	 * Define and apply the solver
	 */
	std::unique_ptr< AlgoAbstract< SplitOrient > > trade_ptr;

	switch ( _mcfa ) {
		case NETWORK_SIMPLEX: {
			typedef lemon::NetworkSimplex< SplitOrient, int64_t > FlowAlgorithm;
			trade_ptr.reset(new AlgoWrapper< FlowAlgorithm, SplitOrient >
				(split_orient, supply_map, capacity_map, cost_map));
			break;
		}

		case COST_SCALING: {
			typedef lemon::CostScaling< SplitOrient, int64_t > FlowAlgorithm;
			trade_ptr.reset(new AlgoWrapper< FlowAlgorithm, SplitOrient >
				(split_orient, supply_map, capacity_map, cost_map));
			break;
		}

		case CAPACITY_SCALING: {
			typedef lemon::CapacityScaling< SplitOrient, int64_t > FlowAlgorithm;
			trade_ptr.reset(new AlgoWrapper< FlowAlgorithm, SplitOrient >
				(split_orient, supply_map, capacity_map, cost_map));
			break;
		}

		case CYCLE_CANCELING: {
			typedef lemon::CycleCanceling< SplitOrient, int64_t > FlowAlgorithm;
			trade_ptr.reset(new AlgoWrapper< FlowAlgorithm, SplitOrient >
				(split_orient, supply_map, capacity_map, cost_map));
			break;
		}

		default: {
			throw std::logic_error("No implementation for "
					"minimum cost flow algorithm "
					+ std::to_string(_mcfa));
			break;
		}
	}

	/**
	 * Run and get the Problem Type
	 */
	trade_ptr->run();

	/**
	 * Check if perfect match has been found.
	 */
	if ( !trade_ptr->optimalSolution() ) {
		throw std::runtime_error("No optimal solution found");
	}

	/**
	 * Get the flow map
	 */
	SplitOrient::ArcMap< int64_t > flow_map( split_orient );
	trade_ptr->flowMap( flow_map );

	/**
	 * Map it back to the original graph.
	 */
	for ( StartGraph::ArcIt a(start_graph); a != lemon::INVALID; ++a ) {

		auto const & want_arc = split_graph.arc(a);
		const bool chosen = flow_map[ want_arc ];

		if ( chosen ) {

			/**
			 * Receiver & Sender nodes: source/target
			 * of original chosen arc.
			 */
			const OutputGraph::Node
				&receiver = start_graph.source(a),
				&sender = start_graph.target(a);

			/**
			 * Mark original arc as chosen.
			 * This should be the first and only time
			 * when the chosen arc is marked as "trading".
			 */
			if ( this->_chosen_arc[a] ) {
				throw std::runtime_error("Arc from "
						+ _name[ _node_out2in[receiver] ]
						+ " to "
						+ _name[ _node_out2in[sender] ]
						+ " has been already chosen");
			}
			this->_chosen_arc[a] = chosen;

			/**
			 * By convention, mark only the receiver as trading.
			 * The sender will be marked
			 * by its own chosen "want" arc.
			 * This should be the first and only time
			 * when the receiver node is marked as "trading".
			 */
			if ( this->_trade[receiver] ) {
				throw std::runtime_error("Multiple trades for item "
						+ _name[ _node_out2in[receiver] ]);
			}
			_trade[receiver] = true;

			/**
			 * Set the receiver & sender
			 * reciprocal maps.
			 */
			_receive[ receiver ] = sender;
			_send[ sender ] = receiver;
		}
	}
}


/************************************//*
 * 	PRIVATE METHODS - Utilities
 **************************************/

#if 0
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
#endif


/************************************//*
 * 	PRIVATE METHODS - Parameters
 **************************************/

int64_t
MathTrader::_getCost( int rank ) const {

	/**
	 * Source: https://www.boardgamegeek.com/wiki/page/TradeMaximizer#toc4
	 */
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
