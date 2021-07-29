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

#include "mathtrader/common/item.pb.h"
#include "mathtrader/parser/internal/item_parser.h"

#include <utility>

#include "absl/strings/string_view.h"
#include "gtest/gtest.h"

namespace {

using ::mathtrader::internal_parser::ItemParser;
using ::mathtrader::OfferedItem;

constexpr char kPandemicId[] = "0012-PANDE";  // case-insensitive
constexpr char kPandemicName[] = "Pandemic";
constexpr char kPandemicUser[] = "USER";  // case-insensitive

// Extracts the item from the given text and compares it with the given
// arguments. Empty arguments are not compared.
void ExpectItem(absl::string_view text, absl::string_view id,
                absl::string_view username = "",
                absl::string_view official_name = "", int64_t copy_id = 0,
                int64_t num_copies = 0) {
  ItemParser item_parser;
  const auto item = item_parser.ParseItem(text);

  ASSERT_TRUE(item.ok()) << item.status().message();
  EXPECT_EQ(item->id(), id);
  EXPECT_FALSE(item->is_dummy());

  // Checks whether at least one official field has been given.
  bool has_official_data = !username.empty() || !official_name.empty()
                           || copy_id || num_copies;

  if (!username.empty()) {
    EXPECT_EQ(item->GetExtension(OfferedItem::username), username);
  } else if (has_official_data) {
    EXPECT_FALSE(item->HasExtension(OfferedItem::username));
  }
  if (!official_name.empty()) {
    EXPECT_EQ(item->GetExtension(OfferedItem::official_name), official_name);
  } else if (has_official_data) {
    EXPECT_FALSE(item->HasExtension(OfferedItem::official_name));
  }
  if (copy_id) {
    EXPECT_EQ(item->GetExtension(OfferedItem::copy_id), copy_id);
  } else if (has_official_data) {
    EXPECT_FALSE(item->HasExtension(OfferedItem::copy_id));
  }
  if (num_copies) {
    EXPECT_EQ(item->GetExtension(OfferedItem::num_copies), num_copies);
  } else if (has_official_data) {
    EXPECT_FALSE(item->HasExtension(OfferedItem::num_copies));
  }
}

// Expects Pandemic with only the item id.
void ExpectPandemic(absl::string_view text) {
  ExpectItem(text, kPandemicId);
}

// Expects Pandemic with username.
void ExpectPandemicWithUsername(absl::string_view text) {
  ExpectItem(text, kPandemicId, kPandemicUser);
}

// Expects Pandemic with official name.
void ExpectPandemicWithName(absl::string_view text) {
  ExpectItem(text, kPandemicId, /*username=*/"", kPandemicName);
}

// Expects Pandemic with both username and official name.
void ExpectPandemicWithNameAndUsername(absl::string_view text) {
  ExpectItem(text, kPandemicId, kPandemicUser, kPandemicName);
}

// Item id only.
TEST(ItemParserTest, TestIdOnly) {
  static constexpr absl::string_view text = R"(0012-PANDE)";
  ExpectPandemic(text);
}

// Item id only with spaces.
TEST(ItemParserTest, TestIdWithSpaces) {
  static constexpr absl::string_view text = R"(   0012-PANDE  )";
  ExpectPandemic(text);
}

// Id and Username only.
TEST(ItemParserTest, TestIdAndUsername) {
  static constexpr absl::string_view text = R"(0012-PaNdE ==> (from User))";
  ExpectPandemicWithUsername(text);
}

// Id and official name only.
TEST(ItemParserTest, TestIdAndOfficialName) {
  static constexpr absl::string_view text = R"(0012-pande ==> "Pandemic")";
  ExpectPandemicWithName(text);
}

// Username and official name.
TEST(ItemParserTest, TestIdAndOfficialNameAndUsername) {
  static constexpr absl::string_view text = R"(0012-Pande ==> "Pandemic" (from UsEr))";
  ExpectPandemicWithNameAndUsername(text);
}

// Tests an item where multiple copies are given.
TEST(ItemParserTest, TestMultipleCopies) {
  static constexpr absl::string_view text
      = R"(9999-5GIFT-COPY10 ==> "Alt Name: $7 Gift Certificate" (from dummyUserName) [copy 17 of 64])";

  ExpectItem(text, "9999-5GIFT-COPY10", "DUMMYUSERNAME",
             "Alt Name: $7 Gift Certificate", /*copy_id=*/17,
             /*num_copies=*/64);
}

// Tests an item separated by an optional colon, without separating spaces.
TEST(ItemParserTest, TestWithColon) {
  static constexpr absl::string_view text
    = R"(0012-PANDE:"Pandemic" (from user))";
  ExpectPandemicWithNameAndUsername(text);
}

// Tests an item with leading whitespaces (spaces and tabs).
TEST(ItemParserTest, TestWithSpaces) {
  static constexpr absl::string_view text = "  0012-PANDE   'some name'";
  ExpectPandemic(text);
}

// Tests an item with leading whitespaces.
TEST(ItemParserTest, TestWithWhitespaces) {
  static constexpr absl::string_view text
    = " \t    \t   0012-PANDE    'some name'";
  ExpectPandemic(text);
}

// Tests an item with leading spaces and colon.
TEST(ItemParserTest, TestWithWhitespacesAndColon) {
  static constexpr absl::string_view text = " \t   0012-PANDE: 'some name'";
  ExpectPandemic(text);
}

// Tests an item with additional number before official name.
TEST(ItemParserTest, TestWithExtraId) {
  static constexpr absl::string_view text = R"(0012-PANDE ==> 42. "Pandemic")";
  ExpectPandemicWithName(text);
}

// Test suite: non-strict item ids. These do not conform to the format
// <numeric>-<alphanumeric> or <numeric>-<alphanumeric>-COPY<numeric>, but are
// still accepted.

// Tests a non-strict item id, which has an alphanumeric prefix "NNNN".
TEST(ItemParserNonStrictItemIdTest, TestAlphanumericPrefix) {
  static constexpr absl::string_view text
    = R"(1A-ID ==> "OfficialName" (from Username))";
  ExpectItem(text, "1A-ID", "USERNAME", "OfficialName");
}

// Tests a non-strict item id, which a lowercase suffix.
TEST(ItemParserNonStrictItemIdTest, TestLowercaseSuffix) {
  static constexpr absl::string_view text
    = R"(1-id ==> "OfficialName" (from Username))";
  ExpectItem(text, "1-ID", "USERNAME", "OfficialName");
}

// Tests a non-strict item id, missing the numeric prefix "NNNN".
TEST(ItemParserNonStrictItemIdTest, TestMissingAlphanumeric) {
  static constexpr absl::string_view text
    = R"(1- ==> "OfficialName" (from Username))";
  ExpectItem(text, "1-", "USERNAME", "OfficialName");
}

// Tests a non-strict item id, missing the dash character in "NNNN-AAA".
TEST(ItemParserNonStrictItemIdTest, TestMissingDash) {
  static constexpr absl::string_view text
    = R"(1ID ==> "OfficialName" (from Username))";
  ExpectItem(text, "1ID", "USERNAME", "OfficialName");
}

// Tests a non-strict item id, missing the prefix numberical "NNNN".
TEST(ItemParserNonStrictItemIdTest, TestMissingNumeric) {
  static constexpr absl::string_view text
    = R"(-ID ==> "OfficialName" (from Username))";
  ExpectItem(text, "-ID", "USERNAME", "OfficialName");
}

// Tests a non-strict item id, having a negative prefix numerical "NNNN".
TEST(ItemParserNonStrictItemIdTest, TestNegativeNumeric) {
  static constexpr absl::string_view text
    = R"(-1-ID ==> "OfficialName" (from Username))";
  ExpectItem(text, "-1-ID", "USERNAME", "OfficialName");
}

// Test suite: capturing copy ids.

TEST(ItemParserCopiesTest, TestCopies) {
  static constexpr absl::string_view text
    = R"(1A-ID ==> "OfficialName" (from Username) [copy 5 of 42])";
  ExpectItem(text, "1A-ID", "USERNAME", "OfficialName", 5, 42);
}

TEST(ItemParserCopiesTest, TestCopiesWithoutUsername) {
  static constexpr absl::string_view text
    = R"(1A-ID ==> (from Username) [copy 10 of 10000])";
  ExpectItem(text, "1A-ID", "USERNAME", "", 10, 10000);
}

TEST(ItemParserCopiesTest, TestCopiesWithoutOfficialName) {
  static constexpr absl::string_view text
    = R"(1A-ID ==> "OfficialName" [copy 10 of 10000])";
  ExpectItem(text, "1A-ID", "", "OfficialName", 10, 10000);
}

TEST(ItemParserCopiesTest, TestCopiesWithoutOfficialNameOrUsername) {
  static constexpr absl::string_view text
    = R"(1A-ID ==> [copy 10 of 10000])";
  ExpectItem(text, "1A-ID", "", "", 10, 10000);
}

// Test suite: negative tests

TEST(ItemParserNegativeTest, TestEmpty) {
  ItemParser item_parser;
  static constexpr absl::string_view text = "";
  const auto item = item_parser.ParseItem(text);
  EXPECT_TRUE(absl::IsInvalidArgument(item.status()));
}

TEST(ItemParserNegativeTest, TestWithSpaces) {
  ItemParser item_parser;
  static constexpr absl::string_view text = "     ";
  const auto item = item_parser.ParseItem(text);
  EXPECT_TRUE(absl::IsInvalidArgument(item.status()));
}

TEST(ItemParserNegativeTest, TestWithWhitespaces) {
  ItemParser item_parser;
  static constexpr absl::string_view text = "  \t  \t    \t  \t    ";
  const auto item = item_parser.ParseItem(text);
  EXPECT_TRUE(absl::IsInvalidArgument(item.status()));
}

TEST(ItemParserNegativeTest, TestWithWhitespacesAndColon) {
  ItemParser item_parser;
  static constexpr absl::string_view text = "\t   \t   \t : \t \t   ";
  const auto item = item_parser.ParseItem(text);
  EXPECT_TRUE(absl::IsInvalidArgument(item.status()));
}

TEST(ItemParserNegativeTest, TestDummyItem) {
  ItemParser item_parser;
  static constexpr absl::string_view text = R"(%DummyItem)";
  const auto item = item_parser.ParseItem(text);
  EXPECT_TRUE(absl::IsInvalidArgument(item.status()));
}

TEST(ItemParserNegativeTest, TestDummyItemWithSpaces) {
  ItemParser item_parser;
  static constexpr absl::string_view text = R"(  %DummyItem   )";
  const auto item = item_parser.ParseItem(text);
  EXPECT_TRUE(absl::IsInvalidArgument(item.status()));
}

}  // namespace
