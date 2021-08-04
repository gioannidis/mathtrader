// This file is part of mathtrader.
//
// Copyright (C) 2021 George Ioannidis
//
// mathtrader is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// mathtrader is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with mathtrader. If not, see <http://www.gnu.org/licenses/>.

#include "mathtrader/parser/util/item_util.h"

#include "gtest/gtest.h"

#include "mathtrader/common/item.pb.h"

namespace {
using ::mathtrader::parser::util::IsDummyItem;
using ::mathtrader::Item;

TEST(ItemUtilTest, TestStrings) {
  EXPECT_FALSE(IsDummyItem("0012-PANDE"));
  EXPECT_TRUE(IsDummyItem("%0012-PANDE"));

  EXPECT_FALSE(IsDummyItem("  \t  0012-PANDE"));
  EXPECT_TRUE(IsDummyItem("  \t  \t %0012-PANDE"));
}

TEST(ItemUtilTest, TestItems) {
  {
    Item non_dummy;
    non_dummy.set_id("0001-MKBG");
    EXPECT_FALSE(IsDummyItem(non_dummy));
    EXPECT_FALSE(IsDummyItem(&non_dummy));
  }

  {
    Item dummy;
    dummy.set_id("%0001-MKBG");
    EXPECT_TRUE(IsDummyItem(dummy));
    EXPECT_TRUE(IsDummyItem(&dummy));
  }
}
}  // namespace
