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

#include <unordered_map>

void
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
}

void
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
}

/********************************
 * 	PUBLIC - OUTPUT OPTIONS	*
 ********************************/

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
