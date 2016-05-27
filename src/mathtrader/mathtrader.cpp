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

/* STL libraries */
#include <fstream>
#include <iomanip>
#include <list>
#include <map>
#include <stdexcept>
#include <unordered_map>

/* Lemon base libraries */
#include <lemon/adaptors.h>
#include <lemon/connectivity.h>

/* Lemon Algorithms */
#include <lemon/capacity_scaling.h>
#include <lemon/cost_scaling.h>
#include <lemon/cycle_canceling.h>
#include <lemon/network_simplex.h>


/************************************//*
 * 	PUBLIC METHODS - CONSTRUCTORS
 **************************************/

MathTrader::MathTrader() :
	/* Base class */
	BaseMath(),

	/* options */
	_mcfa( NETWORK_SIMPLEX ),		/**< Option: algorithm 	*/
	_COST_NONTRADE( 1 << 29 ),
	_COST_MORETRADES( 1 << 20 ),

	_hide_loops( false ),
	_hide_non_trades( false ),
	_hide_stats( false ),
	_hide_summary( false ),
	_sort_by_item( false ),

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
 * 	PUBLIC METHODS - INPUT OPTIONS
 **************************************/

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


/************************************//*
 * 	PUBLIC METHODS - OUTPUT OPTIONS
 **************************************/

MathTrader &
MathTrader::hideLoops( bool v ) {
	_hide_loops = v;
	return *this;
}

MathTrader &
MathTrader::hideNonTrades( bool v ) {
	_hide_non_trades = v;
	return *this;
}

MathTrader &
MathTrader::hideStats( bool v ) {
	_hide_stats = v;
	return *this;
}

MathTrader &
MathTrader::hideSummary( bool v ) {
	_hide_summary = v;
	return *this;
}

MathTrader &
MathTrader::sortByItem( bool v ) {
	_sort_by_item = v;
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

	//this->_runMaximizeTrades();
	this->_runMaximizeUsers();
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

#if 0
				/**
				 * First, calculate the rank
				 * between receiver -> D1 -> D2 -> ... -> DN -> sender.
				 */
				int rank = -1;
				const OutputGraph::Node
					*prev = receiver,
					*next = prev;

				/**
				 * Get the minimum rank across the whole
				 * want chain between A -> D1 -> D2 -> B
				 */
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
#else
				/**
				 * Get the rank of A -> D1 only
				 * across the chain between A -> D1 -> D2 -> B
				 */
				auto const & next = _receive[*receiver];
				auto const & arc = arc_lookup( *receiver, next );
				if ( arc == lemon::INVALID ) {
					throw std::runtime_error("Arc not found "
							"between items "
							+ _name[_node_out2in[*receiver]]
							+ " and "
							+ _name[_node_out2in[next]]);
				}

				const int rank = _out_rank[arc];
#endif
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
		auto const & arc = g.addArc( *(new_arc.s), *(new_arc.t) );
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
MathTrader::writeResults( std::ostream & os ) const {

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
	 * Calculate statistics
	 */
	FinalGraph::NodeMap< int > component_id( final_graph );
	const int n_cycles = stronglyConnectedComponents( final_graph, component_id );
	const int total_trades = countArcs( final_graph );
	std::vector< int > cycle_size;
	cycle_size.reserve( n_cycles );
	int n_groups = 0;	/**< Trading cycles */


	/***********************************//*
	 * 	TRADE LOOPS
	 ************************************/

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

	/**
	 * Show in output?
	 */
	if ( !_hide_loops ) {
		os << "TRADE LOOPS (" << total_trades << " total trades):" << std::endl;
	}

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
	 * TODO: replace with DFS.
	 */
	for ( int cycle_start : cycle_start_vec ) {

		auto const & g = final_graph;
		const auto start_node  = g.nodeFromId( cycle_start );

		int cur_size = 0;

		if ( this->_trade[start_node] ) {

			/**
			 * Found a trading group/cycle.
			 * Count it.
			 */
			++ n_groups;

			auto cur_node = start_node;
			do {
				/**
				 * Statitisc:
				 * increase current cycle size.
				 */
				++ cur_size;

				/**
				 * Statistics:
				 * track trading user.
				 */
				auto it = users_trading.emplace( _username[_node_out2in[cur_node]], 0 );
				int & count = it.first->second;
				++ count;

				auto const next_node = _receive[cur_node];

				/**
				 * Show in output?
				 */
				if ( !_hide_loops ) {
					os << std::left
						<< std::setw(TABWIDTH)
						<< "(" + _username[_node_out2in[cur_node]] + ") "
						+ _name[_node_out2in[cur_node]]
						<< "receives (" + _username[_node_out2in[next_node]] + ") "
						+ _name[_node_out2in[next_node]]
						<< std::endl;
				}

				cur_node = next_node;

			} while ( cur_node != start_node );

			/**
			 * Show in output?
			 */
			if ( !_hide_loops ) {
				os << std::endl;
			}

			/**
			 * Statistics: track total cycle size
			 */
			cycle_size.push_back( cur_size );

		} /* trading item */

	} /* iterate cycles */


	/***********************************//*
	 * 	ITEM SUMMARY
	 ************************************/
	if ( !_hide_summary ) {

		os << "ITEM SUMMARY (" << total_trades << " total trades):" << std::endl;
		os << std::endl;

		/**
		 * Structure to summarize an item
		 */
		typedef struct Summary_s {
			const std::string
				user,
				item_name,
				receive_user,
				receive_item,
				send_user,
				send_item;

			Summary_s(
					const std::string & user_,
					const std::string & item_,
					const std::string & ruser_ = "",
					const std::string & ritem_ = "",
					const std::string & suser_ = "",
					const std::string & sitem_ = ""
					) :
				user( user_ ),
				item_name( item_ ),
				receive_user( ruser_ ),
				receive_item( ritem_ ),
				send_user( suser_ ),
				send_item( sitem_ )
			{}

		} Summary_t;

		std::multimap< std::string, Summary_t > summary_multimap;

		for ( FinalGraph::NodeIt n(final_graph); n != lemon::INVALID; ++ n ){

			/**
			 * Usernames and item names are contained in
			 * a map tied to the input graph.
			 * Get the corresponding input graph node.
			 */
			const InputGraph::Node & ni = _node_out2in[n];

			/**
			 * Key
			 */
			const std::string
				/* This user */
				& user	= _username[ni],
				& item	= _name[ni],
				& key	= (!_sort_by_item) ? user : item;

			if ( _trade[n] ) {

				const std::string
					/* User she receives from */
					& ruser	= _username[ _node_out2in[_receive[n]] ],
					& ritem	=     _name[ _node_out2in[_receive[n]] ],

					/* User she sends to */
					& suser	= _username[ _node_out2in[_send[n]] ],
					& sitem	=     _name[ _node_out2in[_send[n]] ];

				summary_multimap.emplace(key,
						Summary_t(user,item,
							ruser,ritem,
							suser,sitem));
			} else if ( !_hide_non_trades ) {

				summary_multimap.emplace(key,
						Summary_t(user,item));
			}
		}

		for ( auto const & item_set : summary_multimap ) {
			auto const & item = item_set.second;

			if ( item.receive_item.length() > 0 ) {

				/**
				 * Trading item summary.
				 */
				os << std::left
					<< std::setw(TABWIDTH)
					<< "(" + item.user + ") "
					+ item.item_name
					<< std::setw(TABWIDTH)
					<< "receives ("
					+ item.receive_user + ") "
					+ item.receive_item
					<< "and sends to ("
					+ item.send_user + ") "
					+ item.send_item
					<< std::endl;

			} else if ( !_hide_non_trades ) {

				/**
				 * Non-trading item summary.
				 * Show only if we're not hiding non-trades.
				 */
				os << std::left
					<< std::setw(TABWIDTH)
					<< "(" + item.user + ") "
					+ item.item_name
					<< "does not trade"
					<< std::endl;
			}
		}
		/**
		 * Final endline to conclude the summary
		 */
		os << std::endl;
	}


	/***********************************//*
	 * 	STATISTICS
	 ************************************/
	if ( !_hide_stats ) {

		int64_t total_cost = 0;
		for ( FinalGraph::ArcIt a(final_graph); a != lemon::INVALID; ++ a ) {
			total_cost += _getCost(_out_rank[a], _dummy[_node_out2in[final_graph.source(a)]]);
		}

		/**
		 * Calculate statistics
		 * TODO users_trading
		 * TODO group sizes
		 */
		const int n_trades = countArcs( cycle_forest );
		const int n_items = countNodes( result_graph );

		os << "TRADE STATISTICS"
			<< std::endl
			<< std::endl	/**< Twice */
			<< "Num trades  = "
			<< n_trades << " of " << n_items << " items"
			<< " ("
			<< std::setprecision(3)
			<< (100.0 * static_cast< double >(n_trades) / n_items )
			<< std::fixed
			<< "%)"
			<< std::endl
			<< "Total cost  = " << total_cost
			<< std::endl
			<< "Num groups  = " << n_groups
			<< std::endl
			<< "Group sizes =";

		for ( auto const & group_size : cycle_size ) {
			os << " " << group_size;
		}

		os << std::endl
			<< "Users trading = " << users_trading.size()
			<< std::endl;
	}

	return *this;
#undef TABWIDTH
}


/************************************//*
 * 	PUBLIC METHODS - Utilities
 **************************************/

const MathTrader &
MathTrader::exportOutputToDot( std::ostream & os ) const {

	/**
	 * TODO show non-trading items if specified.
	 */
	auto const & result_graph = this->_output_graph;
	auto const & trading_graph = filterArcs( result_graph, _chosen_arc );
	auto const cycle_forest = filterNodes( trading_graph, _trade );
	typedef decltype(cycle_forest) CycleForest;

	/**
	 * Create item_name map for template method.
	 * TODO workaround until fixed.
	 */
	CycleForest::NodeMap< std::string > item_name(cycle_forest);
	mapCopy( cycle_forest,
			composeMap(_name, _node_out2in),
			item_name);

	_exportToDot< CycleForest >( os, cycle_forest,
			"Output_Graph", item_name);

	return *this;
}

const MathTrader &
MathTrader::exportOutputToDot( const std::string & fn ) const {

	std::filebuf fb;
	fb.open(fn, std::ios::out);
	std::ostream os(&fb);
	exportOutputToDot(os);
	fb.close();
	return *this;
}


/************************************//*
 * 	PRIVATE METHODS - Trade Graphs
 **************************************/

void
MathTrader::_runMaximizeTrades() {

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

	SplitOrient::NodeMap< int64_t >   supply_map( split_orient, 0 );
	SplitOrient::ArcMap < int64_t > capacity_map( split_orient, 1 ),
		cost_map( split_orient, 0 );

	/**
	 * Iterate nodes of original graph.
	 * Get corresponding bind-arcs.
	 * Cost: c >> 1, but zero if it's a dummy node.
	 * so as to inherently prefer a dummy self-arc over a real item's self-arc.
	 * Mark self-arc to reverse its direction.
	 */
	for ( StartGraph::NodeIt n(start_graph); n != lemon::INVALID; ++ n ) {

		auto const & self_arc = split_graph.arc(n);
		cost_map[ self_arc ] = ( _dummy[_node_out2in[n]] ) ? 0 : 1e9;
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

		cost_map[ match_arc ] = _getCost(rank, _dummy[_node_out2in[start_graph.source(a)]]);
		reverse_map[ match_arc ] = true;
	}

	/**
	 * Iterate nodes of SplitDirect.
	 * In-nodes have a supply of +1
	 * Out-nodes have a supply of -1
	 */
	for ( SplitOrient::NodeIt n( split_orient ); n != lemon::INVALID; ++ n ) {

		supply_map[n] = (split_graph.outNode(n)) ? +1 : -1;
	}

	/**
	 * Flow map; the solver will populate it.
	 */
	SplitOrient::ArcMap< int64_t > flow_map( split_orient );

	/**
	 * Run flow algorithm.
	 */
	this->_runFlowAlgorithm( split_orient,
			supply_map, capacity_map, cost_map, flow_map );

	/**
	 * Map the flow map back to the original graph.
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

void
MathTrader::_runMaximizeUsers() {

	/**
	 * TradeGraph will be eventually constructed
	 * from StartGraph.
	 */
	typedef lemon::SmartDigraph TradeGraph;
	TradeGraph trade_graph;

	/**
	 * Node references and cross references.
	 */
	typedef OutputGraph StartGraph;
	const StartGraph & start_graph = this->_output_graph;


	/********************************************//**
	 *	CREATE TRADE GRAPH
	 ***********************************************/

	/**
	 * Graph -> Split	[split the nodes]
	 * 	 -> NoBind	[filter out bind-arcs]
	 *
	 * SplitNodes splits each node v to v-out and v-in.
	 * Each arc v -> u becomes v-out -> u-in.
	 *
	 * It also adds bind arcs: v-in -> v-out.
	 * These will be filtered out.
	 *
	 *
	 * All node & arc maps are inter-compatible.
	 */

	/**
	 * Split the start graph.
	 * This will have bind arcs.
	 */
	typedef lemon::SplitNodes< StartGraph > SplitBindGraph;
	SplitBindGraph split_bind_graph( start_graph );

	/**
	 * Create filter to filter out
	 * bind-arcs.
	 * Iterate all nodes of start graph,
	 * get the bind-arcs and filter them.
	 */
	SplitBindGraph::ArcMap< bool > nonbind_map( split_bind_graph, true );

	for ( StartGraph::NodeIt n(start_graph); n != lemon::INVALID; ++ n ) {

		auto const & bind_arc = split_bind_graph.arc(n);
		nonbind_map[ bind_arc ] = false;
	}

	/**
	 * Apply the filter
	 * split_graph has no bind arcs.
	 */
	auto split_graph  = filterArcs( split_bind_graph, nonbind_map );
	typedef decltype(split_graph) SplitGraph;

	/**
	 * Copy to trade_graph
	 */

	/* references */
	SplitGraph::NodeMap< TradeGraph::Node > node_split2trade( split_graph );
	SplitGraph::ArcMap < TradeGraph::Arc  >  arc_split2trade( split_graph );
	/* cross-references */
	TradeGraph::NodeMap< SplitGraph::Node > node_trade2split( trade_graph );
	TradeGraph::ArcMap < SplitGraph::Arc  >  arc_trade2split( trade_graph );

	digraphCopy( split_graph, trade_graph ).
		nodeRef( node_split2trade ).
		arcRef(   arc_split2trade ).
		nodeCrossRef( node_trade2split ).
		arcCrossRef(   arc_trade2split ).
		run();


	/********************************************//**
	 *	INITIALIZE SOURCE-CAPACITY-COST MAPS
	 ***********************************************/

	/**
	 * Create supply, capacity & cost maps
	 * Default supply: zero
	 * Default capacity: unit
	 * Default cost: zero
	 */
	TradeGraph::NodeMap< int64_t > supply_map( trade_graph, 0 );
	TradeGraph::ArcMap < int64_t >
		capacity_map( trade_graph, 1 ),
		cost_map( trade_graph, 0 );


	/********************************************//**
	 *	SETUP COSTS FROM PRIORITIES
	 ***********************************************/

	for ( StartGraph::ArcIt a(start_graph); a != lemon::INVALID; ++ a ) {

		/**
		 * Get match_arc at split graph.
		 * Use the reference map to get the corresponding trade arc.
		 */
		auto const & match_arc = split_bind_graph.arc(a);
		auto const & trade_arc = arc_split2trade[ match_arc ];
		const int rank = _out_rank[a];

		cost_map[ trade_arc ] = _getCost(rank, _dummy[_node_out2in[start_graph.source(a)]]);
	}


	/********************************************//**
	 *	EXPAND GRAPH WITH PARENT NODES
	 ***********************************************/

	/**
	 * Structure per USERNAME;
	 * keeps the parent (source) node
	 * and the non-trade (pseudo-sink) node.
	 */
	typedef struct Parent_s {
		TradeGraph::Node parent, notrade;
		Parent_s ( TradeGraph::Node _p, TradeGraph::Node _no ) :
			parent( _p ), notrade( _no ) {}
	} Parent_t;

	/**
	 * Unordered map: usernames are mapped to parent nodes
	 */
	std::unordered_map< std::string, Parent_t >
		parent_map;

	/**
	 * Iterate all nodes of start_graph:
	 * - Dummy item: make nodes sources/sinks, add zero-cost bind-arc.
	 * - Non-dummy item: link item_in to parent_node, item_out to notrade_node.
	 */
	for ( StartGraph::NodeIt n(start_graph); n != lemon::INVALID; ++ n ) {

		/**
		 * In and out nodes of split_graph
		 */
		auto const & out_node = split_bind_graph.outNode(n);
		auto const &  in_node = split_bind_graph.inNode(n);

		/**
		 * In and out nodes of trade_graph
		 */
		auto const & trade_out = node_split2trade[ out_node ];
		auto const & trade_in  = node_split2trade[  in_node ];

		/**
		 * In-nodes should be always sinks.
		 */
		supply_map[ trade_in ] = -1;

		/**
		 * Is it a dummy item?
		 */
		if ( _dummy[_node_out2in[n]] ) {

			/**
			 * Dummy item;
			 *
			 * We will not bother grouping a dummy item
			 * under a user, as we do not want to count them
			 * as "trading items". It's fine not to trade a dummy item.
			 *
			 * Add a zero-cost bind-arc between the in-out nodes.
			 * Make out nodes sources.
			 */
			auto const & bind_arc = trade_graph.addArc( trade_out, trade_in );
			capacity_map[ bind_arc ] = 1;
			cost_map[ bind_arc ] = 0;

			supply_map[ trade_out ] = +1;

		} else {

			/**
			 * Non-dummy item;
			 * look up its parent-node based on the username.
			 */
			const std::string & username = _username[_node_out2in[n]];
			auto it = parent_map.find( username );

			if ( it == parent_map.end() ) {

				/**
				 * Parent node NOT found.
				 * This should be the first item from this username.
				 * Create new parent node.
				 * Initial supplies are zero.
				 * We will set the parent supply maps later.
				 */
				auto const &  parent_node = trade_graph.addNode();
				auto const & notrade_node = trade_graph.addNode();

				supply_map[  parent_node ] = 0;
				supply_map[ notrade_node ] = 0;

				/**
				 * Insert parent node to parent_map
				 */
				auto pair = parent_map.emplace(username,
						Parent_t(parent_node, notrade_node));

				/**
				 * Sanity check
				 */
				if ( !pair.second ) {
					throw std::logic_error("Could not emplace"
							" parent node");
				}

				/**
				 * Point iterator to new item
				 */
				it = pair.first;
			}

			/**
			 * Retrieve parent node.
			 */
			auto const &  parent_node = it->second.parent;
			auto const & notrade_node = it->second.notrade;

			/**
			 * Add arc from parent  node to item_out.
			 * Add arc from notrade node to item_in.
			 */
			auto const & trade_arc = trade_graph.addArc(parent_node, trade_out),
			     & notrade_arc = trade_graph.addArc(notrade_node, trade_in);

			/**
			 * Cost: always zero.
			 * Capacity: always unit.
			 */
			capacity_map[   trade_arc ] = 1;
			capacity_map[ notrade_arc ] = 1;
			cost_map[   trade_arc ] = 0;
			cost_map[ notrade_arc ] = 0;
		}
	}

	/**
	 * Iterate all parent nodes
	 */
	for ( auto const & pair : parent_map ) {

		auto const &  parent_node = pair.second.parent;
		auto const & notrade_node = pair.second.notrade;

		/**
		 * Count out arcs of parent_node.
		 * Count out arcs of notrade_node.
		 */
		const int parent_out  = countOutArcs( trade_graph,  parent_node );
		const int notrade_out = countOutArcs( trade_graph, notrade_node );

		if ( parent_out != notrade_out ) {
			throw std::logic_error("Outgoing arcs from parent node"
					" (" + std::to_string(parent_out) + ")"
					" not equal to outgoing arcs at notrade node"
					" (" + std::to_string(notrade_out) + ")");
		}

		/**
		 * Number of trading items: outgoing arcs from parent.
		 * That's the supply of the parent node.
		 */
		const int n_items = parent_out;
		supply_map[ parent_node ] = n_items;

		/**
		 * Add arcs from parent to notrade.
		 * Orange arc:
		 * - Capacity: n_items - 1
		 * - Cost: lower
		 * Red arc:
		 * - Capacity: unit
		 * - Cost: higher
		 */
		auto const & red_arc = trade_graph.addArc( parent_node, notrade_node );
		capacity_map[ red_arc ] = 1;
		cost_map[ red_arc ] = _COST_NONTRADE;

		/**
		 * Add orange arc only if necessary,
		 * i.e., user is trading more than one item.
		 */
		if ( n_items > 1 ) {
			auto const & orange_arc = trade_graph.addArc( parent_node, notrade_node );
			capacity_map[ orange_arc ] = (n_items - 1);
			cost_map[ orange_arc ] = _COST_NONTRADE; //_COST_MORETRADES
		}
	}


	/********************************************//**
	 *	RUN FLOW ALGORITHM
	 ***********************************************/

	/**
	 * Flow map; the solver will populate it.
	 */
	TradeGraph::ArcMap< int64_t > flow_map( trade_graph );

	/**
	 * Run the solver.
	 */
	this->_runFlowAlgorithm( trade_graph,
			supply_map, capacity_map, cost_map, flow_map );


	/********************************************//**
	 *	MAP THE FLOW RESULTS TO START-GRAPH
	 ***********************************************/

	/**
	 * TODO we are using much of the same code as in runMaximizeItems.
	 * Put it in a private method.
	 */
	for ( StartGraph::ArcIt a(start_graph); a != lemon::INVALID; ++a ) {

		/**
		 * Match start arc -> split_graph arc -> trade_graph arc
		 */
		auto const & want_split_arc = split_bind_graph.arc(a);
		auto const & want_arc = arc_split2trade[ want_split_arc ];
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


	/********************************************//**
	 *	EXPORT PRODUCED TRADE GRAPH TO DOT
	 ***********************************************/

#if 1
	TradeGraph::NodeMap< std::string > label( trade_graph );

	/**
	 * All nodes except parent nodes
	 */
	for ( StartGraph::NodeIt n(start_graph); n != lemon::INVALID; ++ n ) {

		auto const & out_node = split_bind_graph.outNode(n);
		auto const &  in_node = split_bind_graph.inNode(n);
		auto const & trade_out = node_split2trade[ out_node ];
		auto const & trade_in  = node_split2trade[  in_node ];

		label[trade_out] = _name[_node_out2in[n]] + "+";
		label[trade_in]  = _name[_node_out2in[n]] + "-";
	}

	/**
	 * Parent nodes
	 */
	for ( auto const & pair : parent_map ) {

		auto const &  parent_node = pair.second.parent;
		auto const & notrade_node = pair.second.notrade;
		auto const & username = pair.first;

		label[parent_node] = username;
		label[notrade_node] = "no";
	}

	_exportToDot( "trade.dot", trade_graph, "Trade_Graph", label );
#endif
}


/************************************//*
 * 	PRIVATE METHODS - SOLVER
 **************************************/

template < typename DGR >
void
MathTrader::_runFlowAlgorithm( const DGR & g,
		const typename DGR::template NodeMap< int64_t > & supply_map,
		const typename DGR::template  ArcMap< int64_t > & capacity_map,
		const typename DGR::template  ArcMap< int64_t > & cost_map,
		      typename DGR::template  ArcMap< int64_t > & flow_map ) {

	/**
	 * Define and apply the solver
	 */
	std::unique_ptr< AlgoAbstract< DGR > > trade_ptr;

	switch ( _mcfa ) {
		case NETWORK_SIMPLEX: {
			typedef lemon::NetworkSimplex< DGR, int64_t > FlowAlgorithm;
			trade_ptr.reset(new AlgoWrapper< FlowAlgorithm, DGR >
				(g, supply_map, capacity_map, cost_map));
			break;
		}

		case COST_SCALING: {
			typedef lemon::CostScaling< DGR, int64_t > FlowAlgorithm;
			trade_ptr.reset(new AlgoWrapper< FlowAlgorithm, DGR >
				(g, supply_map, capacity_map, cost_map));
			break;
		}

		case CAPACITY_SCALING: {
			typedef lemon::CapacityScaling< DGR, int64_t > FlowAlgorithm;
			trade_ptr.reset(new AlgoWrapper< FlowAlgorithm, DGR >
				(g, supply_map, capacity_map, cost_map));
			break;
		}

		case CYCLE_CANCELING: {
			typedef lemon::CycleCanceling< DGR, int64_t > FlowAlgorithm;
			trade_ptr.reset(new AlgoWrapper< FlowAlgorithm, DGR >
				(g, supply_map, capacity_map, cost_map));
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
	trade_ptr->flowMap( flow_map );
}


/************************************//*
 * 	PRIVATE METHODS - Parameters
 **************************************/


/************************************//*
 * 	PRIVATE METHODS - Utilities
 **************************************/
