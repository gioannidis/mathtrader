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
#include <solver/mathtrader.hpp>

#include <lemon/adaptors.h>	// filterArcs, filterNodes
#include <lemon/lgf_reader.h>
#include "lemon_io.hpp"

/*****************************
 * 	PUBLIC - GRAPH INPUT *
 *****************************/

void
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
}

void
MathTrader::graphReader( const std::string & fn ) {

	std::filebuf fb;
	fb.open(fn, std::ios::in);
	std::istream is(&fb);
	try {
		graphReader(is);
	} catch ( const std::exception & e ) {
		fb.close();
		throw;
	}
	fb.close();
}

/*****************************
 * 	PUBLIC - GRAPH EXPORT *
 *****************************/

void
MathTrader::exportInputToDot( std::ostream & os ) const {

	LemonIo::getInstance().exportToDot< InputGraph >( os, _input_graph,
			_name, "Input_Graph" );
}

void
MathTrader::exportInputToDot( const std::string & fn ) const {

	LemonIo::getInstance().exportToDot< InputGraph >( fn, _input_graph,
			_name,
			"Input_Graph" );
}

void
MathTrader::exportOutputToDot( std::ostream & os ) const {

	// TODO show non-trading items if specified.
	auto const & result_graph = this->_output_graph;
	auto const & trading_graph = filterArcs( result_graph, _chosen_arc );
	auto const cycle_forest = filterNodes( trading_graph, _trade );
	typedef decltype(cycle_forest) CycleForest;

	// Create item_name map for template method.
	// TODO workaround until fixed.
	CycleForest::NodeMap< std::string > item_name(cycle_forest);
	mapCopy( cycle_forest,
			composeMap(_name, _node_out2in),
			item_name);

	LemonIo::getInstance().exportToDot< CycleForest >( os, cycle_forest,
			item_name, "Output_Graph");
}

void
MathTrader::exportOutputToDot( const std::string & fn ) const {

	std::filebuf fb;
	fb.open(fn, std::ios::out);
	std::ostream os(&fb);
	try {
		exportOutputToDot(os);
	} catch ( const std::exception & e ) {
		fb.close();
		throw;
	}
	fb.close();
}
