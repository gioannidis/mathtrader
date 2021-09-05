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

# Defines BUILD rules internal solver modules.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_proto_library", "cc_test")
load("@rules_proto//proto:defs.bzl", "proto_library")

package(
    default_visibility = ["//mathtrader/solver:__pkg__"],
)

cc_library(
    name = "trade_model",
    srcs = ["trade_model.cc"],
    hdrs = ["trade_model.h"],
    deps = [
        "//mathtrader/common:trade_response_cc_proto",
        "//mathtrader/util:str_indexer",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/types:span",
        "@com_google_or_tools//ortools/base:map_util",
        "@com_google_or_tools//ortools/sat:cp_model",
    ],
)

cc_test(
    name = "trade_model_test",
    size = "small",
    srcs = ["trade_model_test.cc"],
    deps = [
        ":trade_model",
        "@com_google_googletest//:gtest_main",
    ],
)

# Rules to build the internal TradeRequest extensions.
proto_library(
    name = "trade_request_extensions_proto",
    srcs = ["trade_request_extensions.proto"],
    deps = ["//mathtrader/parser:trade_request_proto"],
)

cc_proto_library(
    name = "trade_request_extensions_cc_proto",
    deps = [":trade_request_extensions_proto"],
)
