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

# Build rules for utility functions.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "strings",
    srcs = ["str_toupper.cc"],
    hdrs = ["str_toupper.h"],
)

cc_test(
    name = "str_toupper_test",
    size = "small",
    srcs = ["str_toupper_test.cc"],
    deps = [
        ":strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "str_indexer",
    srcs = ["str_indexer.cc"],
    hdrs = ["str_indexer.h"],
    deps = [
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/types:span",
        "@com_google_or_tools//ortools/base:map_util",
    ],
)

cc_test(
    name = "str_indexer_test",
    size = "small",
    srcs = ["str_indexer_test.cc"],
    deps = [
        ":str_indexer",
        "@com_google_googletest//:gtest_main",
    ],
)
