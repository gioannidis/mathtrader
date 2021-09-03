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

# Defines internal BUILD rules for the "officialwants" file parser.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_proto_library", "cc_test")
load("@rules_proto//proto:defs.bzl", "proto_library")

package(
    default_visibility = ["//mathtrader:__subpackages__"],
)

# mathtrader request to solve a particular trade.
proto_library(
    name = "trade_request_proto",
    srcs = ["trade_request.proto"],
    deps = [
        "//mathtrader/common:item_proto",
        "//mathtrader/common:wantlist_proto",
    ],
)

cc_proto_library(
    name = "trade_request_cc_proto",
    deps = [":trade_request_proto"],
)

# Parser of "officialwants" files.
cc_library(
    name = "math_parser",
    srcs = ["math_parser.cc"],
    hdrs = ["math_parser.h"],
    deps = [
        ":trade_request_cc_proto",
        "//mathtrader/parser/internal:internal_parser",
        "@com_google_absl//absl/status:statusor",
    ],
)

# Unit tests for MathParser.
cc_test(
    name = "math_parser_test",
    size = "small",
    srcs = ["math_parser_test.cc"],
    deps = [
        ":math_parser",
        "//mathtrader/common:item_cc_proto",
        "//mathtrader/common:wantlist_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)

# Integration tests with OLWLG data.
cc_test(
    name = "math_parser_olwlg_test",
    size = "medium",
    srcs = ["math_parser_olwlg_test.cc"],
    data = ["//mathtrader/parser/test_data:olwlg_test_data"],
    deps = [
        ":math_parser",
        ":trade_request_cc_proto",
        "//mathtrader/common:item_cc_proto",
        "//mathtrader/common:wantlist_cc_proto",
        "@com_google_absl//absl/status:statusor",
        "@com_google_googletest//:gtest_main",
        "@com_google_or_tools//ortools/base:map_util",
    ],
)
