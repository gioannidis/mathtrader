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
#ifndef _ALGOWRAPPER_HPP_
#define _ALGOWRAPPER_HPP_

template< typename G >
class AlgoWrapper {

public:
	typedef typename G::template NodeMap< int64_t > NodeIntMap;
	typedef typename G::template ArcMap< int64_t > ArcIntMap;

	/**
	 * @brief Constructor.
	 * Details.
	 */
	AlgoWrapper( const G & graph,
			const NodeIntMap & supply,
			const ArcIntMap  & capacity,
			const ArcIntMap  & cost);

	/**
	 * @brief Destructor.
	 * Details.
	 */
	~AlgoWrapper();

private:
	const G & _graph;
	const NodeIntMap & _supply;
	const ArcIntMap & _capacity, & _cost;
};


/************************************//*
 * 	PUBLIC METHODS - CONSTRUCTORS
 **************************************/

template< typename G >
AlgoWrapper< G >::AlgoWrapper( const G & graph,
		const NodeIntMap & supply,
		const ArcIntMap & capacity,
		const ArcIntMap & cost) :
	_graph( graph ),
	_supply( supply ),
	_capacity( capacity ),
	_cost( cost )
{
}

template< typename G >
AlgoWrapper< G >::~AlgoWrapper() {
}

#endif /* _ALGOWRAPPER_HPP_ */
