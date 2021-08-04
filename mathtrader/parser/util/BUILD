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

# Build rules for parser-related utility functions.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(
    default_visibility = ["//mathtrader/parser:__subpackages__"],
)

cc_library(
    name = "item_util",
    hdrs = ["item_util.h"],
    deps = [
        "//mathtrader/common:item_cc_proto",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "item_util_test",
    size = "small",
    srcs = ["item_util_test.cc"],
    deps = [
        ":item_util",
        "//mathtrader/common:item_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)
