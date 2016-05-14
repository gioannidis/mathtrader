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
#include "routechecker.hpp"

#include <lemon/dfs.h>
#include <lemon/maps.h>
#include <lemon/path.h>
#include <stdexcept>


/************************************//*
 * 	PUBLIC METHODS - CONSTRUCTORS
 **************************************/

RouteChecker::RouteChecker() :
	BaseMath()
{
}

RouteChecker::~RouteChecker() {
}


/************************************//*
 * 	PUBLIC METHODS - INPUT
 **************************************/

RouteChecker &
RouteChecker::loopReader( std::istream & is ) {

	_loop_list.clear();

	/**
	 * Read buffer
	 */
	const size_t BUFSIZE = (1<<10);
	std::string buffer;
	buffer.reserve(BUFSIZE);

	while (std::getline( is, buffer )) {

		_loop_list.push_back( buffer );
	}

	return *this;
}


/************************************//*
 * 	PUBLIC METHODS - RUNNABLE
 **************************************/

void
RouteChecker::run() {

	/**
	 * New loop flag.
	 */
	bool new_loop = true;

	/**
	 * Graph to be used.
	 */
	auto const & g = this->_input_graph;
	using RouteGraph = InputGraph;

	/**
	 * Frequency map
	 */
	RouteGraph::NodeMap< int > frequency_map(g, 0);
	int start_id = 0, prev_id = 0;

	/**
	 * Initialize total cost
	 */
	this->_total_cost = 0;

	/**
	 * Parse all items.
	 */
	for ( auto const & item : _loop_list ) {

		/**
		 * Look up node.
		 */
		auto const & n = mapFind( g, _name, item );
		const int cur_id = g.id(n);

		/**
		 * Sanity check: node has been found
		 */
		if ( n == lemon::INVALID ) {
			// TODO: don't use exceptions
			throw std::runtime_error("Could not find item "
					+ item);
		}

		if ( new_loop ) {
			new_loop = false;
			start_id = cur_id;
		} else {

			bool found = false;

			auto const & s = g.nodeFromId(prev_id);
			auto const & t = n;
			int64_t cost = 0;

			/**
			 * Iterate all outgoing arcs of @s.
			 */
			for ( RouteGraph::OutArcIt a(g,s); a != lemon::INVALID; ++a ) {

				auto const & next = g.target(a);

				if ( next == t ) {

					/**
					 * Target node is directly
					 * accessible.
					 */
					found = true;
					cost = _in_rank[a];
					break;

				} else if ( _dummy[next] ) {

					/**
					 * Check if target can be found via a dummy node.
					 * DFS to find a route.
					 * NOTE: we can never be sure which dummy path was chosen!
					 * FIXME: if complex dummy wants are specified,
					 * it's difficult to verify the cost.
					 */
					typedef lemon::Dfs< RouteGraph > DType;
					auto const & dummy = next;

					/**
					 * Run the dfs from s to t.
					 * Sanity check if a path has been found.
					 */
					DType dfs(g);
					bool rv = dfs.run(dummy,t);

					/* Found a path */
					if ( rv ) {
						found = true;
						cost = _in_rank[a];
						break;
					}
				}
			}

			if ( !found ) {
				throw std::runtime_error("No path between items "
					+ _name[s]
					+ " and "
					+ _name[t]);
			}

			_total_cost += cost;

			/**
			 * Flag a new loop if needed.
			 */
			if ( cur_id == start_id ) {
				new_loop = true;
			}
		}
		prev_id = cur_id;

	} /* end for loop */
}


/************************************//*
 * 	PUBLIC METHODS - OUTPUT
 **************************************/

const RouteChecker &
RouteChecker::printResults( std::ostream & os ) const {

	os << "Total cost: " << _total_cost << std::endl;
	return *this;
}


/************************************//*
 * 	PUBLIC METHODS - Utilities
 **************************************/

