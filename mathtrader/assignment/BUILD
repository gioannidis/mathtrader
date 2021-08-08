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

# Build rules the NetworkBuilder.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(
    default_visibility = ["//mathtrader:__subpackages__"],
)

cc_library(
    name = "network_builder",
    srcs = ["network_builder.cc"],
    hdrs = ["network_builder.h"],
    deps = [
        "//mathtrader/assignment/internal:arc_builder",
        "//mathtrader/common:assignment_cc_proto",
        "//mathtrader/parser:parser_result_cc_proto",
    ],
)

cc_test(
    name = "network_builder_test",
    size = "small",
    srcs = ["network_builder_test.cc"],
    deps = [
        ":network_builder",
        "//mathtrader/common:assignment_cc_proto",
        "//mathtrader/common:item_cc_proto",
        "//mathtrader/common:wantlist_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)
