# This file is part of mathtrader.
#
# Copyright (C) 2021 George Ioannidis
#
# mathtrader is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# mathtrader is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with mathtrader. If not, see <http://www.gnu.org/licenses/>.

# Defines filegroup for OLWLG test files.

filegroup(
    name = "olwlg_test_data",
    srcs = glob(["*-officialwants.txt"]),
    visibility = ["//visibility:public"],
)

sh_binary(
    name = "stats_officialwants",
    srcs = ["stats_officialwants.sh"],
    data = [":olwlg_test_data"],
)
