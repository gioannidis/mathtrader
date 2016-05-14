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

		std::cout << "Item: " << item << std::endl;
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

			/**
			 * DFS to find a route.
			 */
			typedef lemon::Dfs< RouteGraph > DType;
			DType::DistMap d(g);
			DType::PredMap p(g);

			auto const & s = g.nodeFromId(prev_id);
			auto const & t = n;


			/**
			 * Run the dfs from s to t.
			 * Sanity check if a path has been found.
			 */
			DType dfs(g);
			bool rv = dfs.run(s,t);

			if ( !rv ) {
				throw std::runtime_error("No path between items "
						+ _name[s]
						+ " and "
						+ _name[t]);
			}

			/**
			 * Get the path to @t.
			 */
			lemon::Path< RouteGraph > pp = dfs.path(t);
			if ( pp.empty() ) {
				throw std::logic_error("Path between items "
						+ _name[s]
						+ " and "
						+ _name[t]
						+ " is empty, "
						"but DFS found a solution");
			}


			/**
			 * Get the first arc only.
			 * Add its cost.
			 */
			auto const & arc = pp.front();
			std::cout
				<< "source: " << _name[g.source(arc)]
				<< " target: " << _name[g.target(arc)]
				<< std::endl;
			_total_cost += _getCost( _in_rank[arc] );
#if 0
			std::cout << "rank of item "
				<< _name[s] << " is "
				<< _in_rank[arc]
				<< " and its cost is "
				<< _getCost( _in_rank[arc] )
				<< std::endl;
#endif

			/**
			 * Flag a new loop if needed.
			 */
			if ( cur_id == start_id ) {
				new_loop = true;
				std::cout << "----" << std::endl;
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

