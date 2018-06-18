/* This file is part of MathTrader++, a C++ utility
 * for finding, on a directed graph whose arcs have costs,
 * a set of vertex-disjoint cycles that maximizes the number
 * of covered vertices as a first priority
 * and minimizes the total cost as a second priority.
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
#ifndef _ALGOWRAPPER_HPP_
#define _ALGOWRAPPER_HPP_

#include "algoabstract.hpp"

template< typename A, typename G >
class AlgoWrapper : public AlgoAbstract< G > {

public:
	typedef typename G::template NodeMap< int64_t > NodeIntMap;
	typedef typename AlgoAbstract< G >::ArcIntMap ArcIntMap;

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

	/**
	 * @brief Runnable.
	 * Run the minimum cost flow algorithm
	 */
	void run();

	/**
	 * @brief Has an optimal solution been found?
	 * Checks whether the algorithm has found
	 * an optimal solution for the MCF problem.
	 * run() must be called beforehand.
	 * @return true if an optimal solution has been found.
	 */
	bool optimalSolution() const ;

	/**
	 * @brief Get Flow Map.
	 * Returns the flow map produced by run().
	 * @param flow_map The flow map to be set.
	 * @return *this
	 */
	const AlgoWrapper & flowMap( ArcIntMap & flow_map ) const ;

private:
	const G & _graph;
	const NodeIntMap & _supply;
	const ArcIntMap & _capacity, & _cost;
	A _algorithm;

	typedef typename A::ProblemType ProblemType;
	ProblemType _rv;
};


/************************************//*
 * 	PUBLIC METHODS - CONSTRUCTORS
 **************************************/

template< typename A, typename G >
AlgoWrapper< A, G >::AlgoWrapper( const G & graph,
		const NodeIntMap & supply,
		const ArcIntMap & capacity,
		const ArcIntMap & cost) :
	AlgoAbstract< G >(),
	_graph( graph ),
	_supply( supply ),
	_capacity( capacity ),
	_cost( cost ),
	_algorithm( _graph )
{
	/**
	 * Set the parameters
	 */
	_algorithm.
		supplyMap( supply ).
		upperMap( capacity ).
		costMap( cost );
}

template< typename A, typename G >
AlgoWrapper< A, G >::~AlgoWrapper() {
}

template< typename A, typename G >
void
AlgoWrapper< A, G >::run() {
	_rv = _algorithm.run();
}

template< typename A, typename G >
bool
AlgoWrapper< A, G >::optimalSolution() const {
	return ( _rv == ProblemType::OPTIMAL );
}

template< typename A, typename G >
const AlgoWrapper< A, G > &
AlgoWrapper< A, G >::flowMap( ArcIntMap & flow_map ) const {
	_algorithm.flowMap( flow_map );
	return *this;
}

#endif /* _ALGOWRAPPER_HPP_ */
