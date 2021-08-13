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
#include <string_view>

#include "absl/status/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mathtrader/common/item.pb.h"

namespace {
using ::mathtrader::common::Item;
using ::mathtrader::parser::util::IsDummyItem;
using ::mathtrader::parser::util::MakeItem;
using ::mathtrader::parser::util::ProcessIfDummy;
using ::testing::AllOf;
using ::testing::EndsWith;
using ::testing::StartsWith;

// Test suite: `IsDummyItem` function.

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

// Test suite: `MakeItem` function.

TEST(MakeItemTest, NonDummyItem) {
  static constexpr char kItemId[] = R"(someItemId")";

  const Item item = MakeItem(kItemId);
  EXPECT_EQ(item.id(), kItemId);
  EXPECT_FALSE(item.is_dummy());
  EXPECT_FALSE(item.has_unmodified_id());
  EXPECT_FALSE(item.has_username());
}

TEST(MakeItemTest, NonDummyItemWithUsername) {
  static constexpr char kItemId[] = R"(someItemId")";
  static constexpr char kUsername[] = "randomUser";

  const Item item = MakeItem(kItemId, kUsername);
  EXPECT_EQ(item.id(), kItemId);
  EXPECT_FALSE(item.is_dummy());
  EXPECT_FALSE(item.has_unmodified_id());
  EXPECT_EQ(item.username(), kUsername);
}

TEST(MakeItemTest, DummyItemWithUsername) {
  static constexpr char kItemId[] = R"(%someItemId")";
  static constexpr char kUsername[] = "randomUser";

  const Item item = MakeItem(kItemId, kUsername);
  EXPECT_THAT(item.id(), AllOf(StartsWith(kItemId), EndsWith(kUsername)));
  EXPECT_TRUE(item.is_dummy());
  EXPECT_EQ(item.unmodified_id(), kItemId);
  EXPECT_EQ(item.username(), kUsername);
}

// Test suite: `ProcessIfDummy` function.

TEST(ProcessIfDummyTest, TestNonDummyId) {
  static constexpr char kItemId[] = R"(someItemId")";
  static constexpr char kUsername[] = "randomUser";

  std::string item_id = kItemId;

  // Does not change the original item id, since it's not a dummy.
  ASSERT_TRUE(ProcessIfDummy(kUsername, &item_id).ok());
  EXPECT_EQ(item_id, kItemId);
}

TEST(ProcessIfDummyTest, TestNonDummyItem) {
  static constexpr char kItemId[] = R"(someItemId")";
  static constexpr char kUsername[] = "randomUser";

  // Builds the item.
  Item item;
  item.set_id(kItemId);
  item.set_username(std::string(kUsername));

  // Does not change the original item, since it's not a dummy.
  ASSERT_TRUE(ProcessIfDummy(&item).ok());
  EXPECT_EQ(item.id(), kItemId);
  EXPECT_FALSE(item.has_unmodified_id());
}

TEST(ProcessIfDummyTest, TestDummyId) {
  static constexpr char kItemId[] = R"(%someItemId")";
  static constexpr char kUsername[] = "randomUser";

  std::string item_id = kItemId;

  // Mutates the original item id, since it's a dummy.
  ASSERT_TRUE(ProcessIfDummy(kUsername, &item_id).ok());
  EXPECT_THAT(item_id, AllOf(StartsWith(kItemId), EndsWith(kUsername)));
}

// As above, but tests consecutive processing.
TEST(ProcessIfDummyTest, TestDummyIdMultipleProcessing) {
  static constexpr char kItemId[] = R"(%someItemId")";
  static constexpr char kUsername[] = "randomUser";

  std::string item_id = kItemId;

  // Mutates the original item id, since it's a dummy.
  ASSERT_TRUE(ProcessIfDummy(kUsername, &item_id).ok());
  EXPECT_THAT(item_id, AllOf(StartsWith(kItemId), EndsWith(kUsername)));

  // Copies the item_id before subsequent processing.
  const std::string item_id_copy = item_id;
  ASSERT_TRUE(ProcessIfDummy(kUsername, &item_id).ok());
  EXPECT_EQ(item_id, item_id_copy);

  ASSERT_TRUE(ProcessIfDummy(kUsername, &item_id).ok());
  EXPECT_EQ(item_id, item_id_copy);
}

TEST(ProcessIfDummyTest, TestDummyIdIdenticalToUsername) {
  static constexpr std::string_view kItemId = R"(%someItemId")";
  const std::string_view kUsername = kItemId;

  auto item_id = std::string(kItemId);

  // Mutates the original item id, since it's a dummy.
  ASSERT_TRUE(ProcessIfDummy(kUsername, &item_id).ok());
  EXPECT_THAT(item_id, AllOf(StartsWith(kItemId), EndsWith(kUsername)));

  // Verifies that the item_id was actually mutated.
  EXPECT_GT(item_id.size(), kItemId.size());
}

TEST(ProcessIfDummyTest, TestDummy) {
  static constexpr char kItemId[] = R"(%someItemId")";
  static constexpr char kUsername[] = "randomUser";

  // Builds the item.
  Item item;
  item.set_id(kItemId);
  item.set_username(std::string(kUsername));

  // Mutates the item id, since it's a dummy.
  ASSERT_TRUE(ProcessIfDummy(&item).ok());
  EXPECT_THAT(item.id(), AllOf(StartsWith(kItemId), EndsWith(kUsername)));
  EXPECT_TRUE(item.is_dummy());
  EXPECT_EQ(item.unmodified_id(), kItemId);
}

TEST(ProcessIfDummyTest, DummyItemIdWithoutUsername) {
  Item item;
  item.set_id(R"(%dummyId)");
  EXPECT_TRUE(absl::IsNotFound(ProcessIfDummy(&item)));
}

TEST(ProcessIfDummyTest, DummyItemPropertyWithoutUsername) {
  Item item;
  item.set_is_dummy(true);
  EXPECT_TRUE(absl::IsNotFound(ProcessIfDummy(&item)));
}

TEST(ProcessIfDummyDeathTest, NullItem) {
  EXPECT_DEATH(ProcessIfDummy(nullptr).IgnoreError(), "Must be non NULL");
}

TEST(ProcessIfDummyDeathTest, NullItemWithUsername) {
  Item* const item = nullptr;
  EXPECT_DEATH(ProcessIfDummy("someUsername", item).IgnoreError(),
               "Must be non NULL");
}

TEST(ProcessIfDummyDeathTest, NullUsername) {
  std::string* const item_id = nullptr;
  EXPECT_DEATH(ProcessIfDummy("someUsername", item_id).IgnoreError(),
               "Must be non NULL");
}
}  // namespace
