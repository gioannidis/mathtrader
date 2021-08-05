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

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/offered_item.pb.h"

namespace {
using ::mathtrader::parser::util::IsDummyItem;
using ::mathtrader::parser::util::UniquifyDummyItem;
using ::mathtrader::Item;
using ::mathtrader::OfferedItem;
using ::testing::AllOf;
using ::testing::EndsWith;
using ::testing::StartsWith;

TEST(IsDummyItemTest, TestStrings) {
  EXPECT_FALSE(IsDummyItem("0012-PANDE"));
  EXPECT_TRUE(IsDummyItem("%0012-PANDE"));

  EXPECT_FALSE(IsDummyItem("  \t  0012-PANDE"));
  EXPECT_TRUE(IsDummyItem("  \t  \t %0012-PANDE"));
}

TEST(IsDummyItemTest, TestItems) {
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

TEST(UniquifyDummyItemTest, TestNonDummy) {
  static constexpr char kItemId[] = R"(someItemId")";
  static constexpr char kUsername[] = "randomUser";

  // Builds the item.
  Item item;
  item.set_id(kItemId);
  item.SetExtension(OfferedItem::username, std::string(kUsername));

  // Does not change the original item, since it's not a dummy.
  UniquifyDummyItem(&item);
  EXPECT_EQ(item.id(), kItemId);
}

TEST(UniquifyDummyItemTest, TestDummy) {
  static constexpr char kItemId[] = R"(%someItemId")";
  static constexpr char kUsername[] = "randomUser";

  // Builds the item.
  Item item;
  item.set_id(kItemId);
  item.SetExtension(OfferedItem::username, std::string(kUsername));

  // Copies the item to verify that it will not change.
  Item item_copy;
  item_copy.CopyFrom(item);

  // Mutates the item id, since it's a dummy.
  UniquifyDummyItem(&item);
  EXPECT_THAT(item.id(), AllOf(StartsWith(kItemId), EndsWith(kUsername)));
}
}  // namespace
