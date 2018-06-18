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

#include <algorithm>
#include <sstream>
#include <thread>	// Google Test runs on threads

#include <gtest/gtest.h>
#include <solver/mathtrader.hpp>
#include <iograph/wantparser.hpp>

void testUsecase( unsigned trade_num, unsigned num_trades ) {
	const std::string input = "http://bgg.activityclub.org/olwlg/"
		+ std::to_string(trade_num)
		+ "-officialwants.txt";

	WantParser want_parser;
	std::stringstream graph;
	want_parser.parseUrl(input);
	want_parser.print(graph);

	MathTrader trade_solver;
	trade_solver.graphReader(graph);
	trade_solver.run();
	trade_solver.mergeDummyItems();
	EXPECT_EQ(num_trades, trade_solver.getNumTrades());
}

TEST( WantParserTest, 2016_April_GR_url ) {
	testUsecase( 207635, 268 );
}

TEST( WantParserTest, 2018_Origins_url ) {
	testUsecase( 240154, 1349 );
}

TEST( WantParserTest, 2018_June_UK_url ) {
	testUsecase( 241767, 241 );
}

int main( int argc, char ** argv ) {

	testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}
