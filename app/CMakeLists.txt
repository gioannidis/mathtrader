#  This file is part of MathTrader++.
#
#  Copyright (C) 2018 George Ioannidis
#
#  MathTrader++ is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  MathTrader++ is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with MathTrader++.  If not, see <http://www.gnu.org/licenses/>.

project(AppProject LANGUAGES CXX)

# Define the executable(s).
add_executable(mathtrader++
	mathtrader.cpp
	PracticalSocket.cpp
)
add_executable(routechecker
	routechecker.cpp
)

# Define the libraries the executables depend upon.
target_link_libraries(mathtrader++
	iograph
	solver
)
target_link_libraries(routechecker
	iograph
	solver
)
