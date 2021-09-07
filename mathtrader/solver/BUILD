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

# Defines BUILD rules for the module that solves the math trade.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(
    default_visibility = ["//mathtrader:__subpackages__"],
)

# Build rules for the mathtrader solver.
cc_library(
    name = "solver",
    srcs = ["solver.cc"],
    hdrs = ["solver.h"],
    deps = [
        "//mathtrader/common:item_cc_proto",
        "//mathtrader/common:trade_request_cc_proto",
        "//mathtrader/common:trade_response_cc_proto",
        "//mathtrader/common:wantlist_cc_proto",
        "//mathtrader/solver/internal:trade_model",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_or_tools//ortools/sat:cp_model",
    ],
)

cc_test(
    name = "solver_test",
    size = "small",
    srcs = ["solver_test.cc"],
    data = ["//mathtrader/solver/test_data:trade_request_textproto"],
    deps = [
        ":solver",
        "//mathtrader/common:item_cc_proto",
        "//mathtrader/common:trade_request_cc_proto",
        "//mathtrader/common:trade_response_cc_proto",
        "//mathtrader/solver/internal:trade_request_extensions_cc_proto",
        "@com_google_absl//absl/types:span",
        "@com_google_googletest//:gtest_main",
        "@com_google_or_tools//ortools/base:file",
        "@com_google_or_tools//ortools/base:map_util",
    ],
)

# Integration tests with OLWLG data.
cc_test(
    name = "solver_olwlg_test",
    size = "small",
    timeout = "eternal",
    srcs = ["solver_olwlg_test.cc"],
    data = ["//olwlg_data:olwlg_test_data"],
    deps = [
        ":solver",
        "//mathtrader/common:item_cc_proto",
        "//mathtrader/common:trade_request_cc_proto",
        "//mathtrader/common:wantlist_cc_proto",
        "//mathtrader/parser:math_parser",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/status",
        "@com_google_googletest//:gtest_main",
        "@com_google_or_tools//ortools/base",
        "@com_google_or_tools//ortools/sat:cp_model",
    ],
)
