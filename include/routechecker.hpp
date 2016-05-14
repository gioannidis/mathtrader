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
#ifndef _ROUTECHECKER_HPP_
#define _ROUTECHECKER_HPP_

#include "basemath.hpp"

#include <list>

class RouteChecker : public BaseMath {

public:
	/**
	 * @brief Constructor.
	 * Details.
	 */
	RouteChecker();

	/**
	 * @brief Destructor.
	 * Details.
	 */
	~RouteChecker();

	/**
	 * @brief Read loops.
	 * Reads loops from the given input stream.
	 * Expected multiple loops in the following format:
	 * 	A B C D E A K L M N K ...
	 * @param is Input stream to read from
	 * @return *this
	 */
	RouteChecker & loopReader( std::istream & is );

	/**
	 * @brief Run the RouteChecker.
	 * Checks the validity of the routes:
	 * - No node shall be passed twice.
	 * - An arc A->B must either exist directly
	 *   or go through a dummy node.
	 * - Calculate the total cost.
	 */
	void run();

	/**
	 * @brief Print RouteChecker results and stats.
	 */
	const RouteChecker & printResults() const ;

private:
	std::list< std::string > _loop_list;
};

#endif /* _ROUTECHECKER_HPP_ */
