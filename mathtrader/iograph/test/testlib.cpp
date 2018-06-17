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
#include <chrono>
#include <fstream>
#include <thread>	// Google Test runs on threads
#include <vector>
#include <iostream>

#include <gtest/gtest.h>
#include <iograph/wantparser.hpp>

TEST( WantParserTest, UrlFetch ) {
	WantParser want_parser;
	want_parser.parseUrl("http://bgg.activityclub.org/olwlg/207635-officialwants.txt");
}

int main( int argc, char ** argv ) {

	testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}
