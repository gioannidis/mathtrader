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

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "solver_result_proto",
    srcs = ["solver_result.proto"],
    deps = [
        "//mathtrader/common:item_proto",
        "//mathtrader/common:wantlist_proto",
    ],
)

cc_proto_library(
    name = "solver_result_cc_proto",
    deps = [":solver_result_proto"],
)

cc_library(
    name = "solver",
    srcs = ["solver.cc"],
    hdrs = ["solver.h"],
    deps = [
        ":solver_result_cc_proto",
        "//mathtrader/assignment:assignment_cc_proto",
        "@com_google_absl//absl/status:statusor",
    ],
)
