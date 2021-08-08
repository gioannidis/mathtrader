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

# Build rules for internal modules that build an Assignment.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(
    default_visibility = ["//mathtrader/network:__pkg__"],
)

# Build rules for the ArcBuilder.
cc_library(
    name = "arc_builder",
    srcs = ["arc_builder.cc"],
    hdrs = ["arc_builder.h"],
    deps = [
        "//mathtrader/common:assignment_cc_proto",
        "//mathtrader/common:item_cc_proto",
        "//mathtrader/common:wantlist_cc_proto",
        "//mathtrader/parser:parser_result_cc_proto",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/strings",
        "@com_google_or_tools//ortools/base",
        "@com_google_or_tools//ortools/base:map_util",
    ],
)

cc_test(
    name = "arc_builder_test",
    size = "small",
    srcs = ["arc_builder_test.cc"],
    deps = [
        ":arc_builder",
        "//mathtrader/common:assignment_cc_proto",
        "//mathtrader/common:item_cc_proto",
        "//mathtrader/common:wantlist_cc_proto",
        "//mathtrader/parser:parser_result_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)
