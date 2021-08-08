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

# Build rules to generate an assignment out of wantlists.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_proto_library", "cc_test")
load("@rules_proto//proto:defs.bzl", "proto_library")

package(
    default_visibility = ["//mathtrader:__subpackages__"],
)

# Build rules for an Assignment graph.
proto_library(
    name = "assignment_proto",
    srcs = ["assignment.proto"],
    deps = ["//mathtrader/common:item_proto"],
)

cc_proto_library(
    name = "assignment_cc_proto",
    deps = [":assignment_proto"],
)

# Build rules for AssignmentBuilder.
cc_library(
    name = "network_builder",
    srcs = ["network_builder.cc"],
    hdrs = ["network_builder.h"],
    deps = [
        ":assignment_cc_proto",
        "//mathtrader/assignment/internal:arc_builder",
        "//mathtrader/parser:parser_result_cc_proto",
    ],
)

cc_test(
    name = "network_builder_test",
    size = "small",
    srcs = ["network_builder_test.cc"],
    deps = [
        ":assignment_cc_proto",
        ":network_builder",
        "//mathtrader/common:item_cc_proto",
        "//mathtrader/common:wantlist_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)
