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
#include "basemath.hpp"

#include <lemon/lgf_reader.h>
#include <stdexcept>
#include <unordered_map>


/************************************//*
 * 	PUBLIC METHODS - CONSTRUCTORS
 **************************************/

BaseMath::BaseMath() :
	/* input graph maps */
	_name( _input_graph ),
	_username( _input_graph ),
	_dummy( _input_graph, false ),
	_in_rank( _input_graph, 0 ),

	/* options */
	_priority_scheme( NO_PRIORITIES )	/**< Option: priorities */
{
}

BaseMath::~BaseMath() {
}


/************************************//*
 * 	PUBLIC METHODS - INPUT
 **************************************/

BaseMath &
BaseMath::graphReader( std::istream & is ) {

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

	return *this;
}

BaseMath &
BaseMath::graphReader( const std::string & fn ) {

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

BaseMath &
BaseMath::setPriorities( const std::string & priorities ) {

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

BaseMath &
BaseMath::clearPriorities() {
	_priority_scheme = NO_PRIORITIES;
	return *this;
}


/************************************//*
 * 	PUBLIC METHODS - RUNNABLE
 **************************************/


/************************************//*
 * 	PUBLIC METHODS - OUTPUT
 **************************************/


/************************************//*
 * 	PUBLIC METHODS - Utilities
 **************************************/

const BaseMath &
BaseMath::exportInputToDot( std::ostream & os ) const {

	_exportToDot< InputGraph >( os, _input_graph,
			"Input_Graph",
			_name );

	return *this;
}

const BaseMath &
BaseMath::exportInputToDot( const std::string & fn ) const {

	std::filebuf fb;
	fb.open(fn, std::ios::out);
	std::ostream os(&fb);
	exportInputToDot(os);
	fb.close();
	return *this;
}


/************************************//*
 * 	PRIVATE METHODS - Parameters
 **************************************/

int64_t
BaseMath::_getCost( int rank, bool dummy_source ) const {

	/**
	 * If the source is dummy, assign zero cost,
	 * no matter the scheme or the rank.
	 */
	if ( dummy_source ) {
		return 0;
	}

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
