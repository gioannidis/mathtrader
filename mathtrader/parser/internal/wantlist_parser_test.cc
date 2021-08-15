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

#include "mathtrader/parser/internal/wantlist_parser.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/wantlist.pb.h"
#include "mathtrader/parser/internal/internal_wantlist.pb.h"
#include "mathtrader/parser/util/item_util.h"

namespace {
using ::mathtrader::common::Wantlist;
using ::mathtrader::parser::internal::InternalWantlist;
using ::mathtrader::parser::internal::WantlistParser;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Each;
using ::testing::ElementsAre;
using ::testing::EndsWith;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::IsFalse;
using ::testing::IsTrue;
using ::testing::Property;
using ::testing::ResultOf;
using ::testing::SizeIs;
using ::testing::StartsWith;
using ::testing::StrCaseEq;
using ::testing::StrEq;

using WantedItem = ::mathtrader::common::Wantlist::WantedItem;

// Internal alias to resolve overloaded function in `ResultOf` matchers.
bool IsDummyItem(std::string_view item_id) {
  return mathtrader::parser::util::IsDummyItem(item_id);
}

// Matches the username, ignoring case.
MATCHER_P(UsernameStrCaseEq, username, "") {
  return ExplainMatchResult(StrCaseEq(username), arg.username(),
                            result_listener);
}

// Matches the priority value.
MATCHER_P(PriorityEq, priority, "") {
  return ExplainMatchResult(Eq(priority), arg.priority(), result_listener);
}

// Verifies that the id argument has been properly annotated with the username,
// if dummy.
MATCHER_P(IsModifiedIfDummy, username, "") {
  return ExplainMatchResult(AnyOf(ResultOf(IsDummyItem, IsFalse()),
                                  AllOf(StartsWith(arg), EndsWith(username))),
                            arg, result_listener);
}

// Matches a wantlist whose offered item or wanted items have been properly
// annotated with the username, if dummy.
MATCHER(HasModifiedDummyIds, "") {
  const std::string_view username =
      arg.GetExtension(InternalWantlist::username);
  return ExplainMatchResult(
      AllOf(Property("offered id", &Wantlist::offered,
                     IsModifiedIfDummy(username)),
            Property("wanted item", &Wantlist::wanted,
                     Each(Property("wanted id", &WantedItem::id,
                                   IsModifiedIfDummy(username))))),
      arg, result_listener);
}

// Tests an empty wantlist without username. Verifies that we ignore spaces.
TEST(WantlistParserTest, TestNoItems) {
  const WantlistParser parser;

  // Defines multiple empty wantlists.
  const std::vector<std::string> texts = {
      "0001-PANDE", "0001-PANDE:", "0001-PANDE :", "  0001-PANDE  :  "};

  for (std::string_view text : texts) {
    const auto wantlist = parser.ParseWantlist(text);

    ASSERT_TRUE(wantlist.ok())
        << text << " error: " << wantlist.status().message();

    // For each wantlist checks:
    // * Offered item id.
    // * No wanted items.
    // * No username.
    EXPECT_EQ(wantlist->offered(), "0001-PANDE");
    EXPECT_EQ(wantlist->wanted_size(), 0);
    EXPECT_FALSE(wantlist->HasExtension(InternalWantlist::username));
  }
}

// Tests an empty wantlist with username. Verifies that we ignore spaces.
TEST(WantlistParserTest, TestNoItemsWithUsername) {
  const WantlistParser parser;
  const std::vector<std::string> texts = {
      "(user) 0001-PANDE", "(user) 0001-PANDE:", "(user) 0001-PANDE :",
      "  (user)   0001-PANDE    :   "};

  for (std::string_view text : texts) {
    const auto wantlist = parser.ParseWantlist(text);

    ASSERT_TRUE(wantlist.ok())
        << wantlist.status().message() << " (" << text << ")";

    // For each wantlist checks:
    // * Offered item id.
    // * No wanted items.
    // * Username.
    EXPECT_EQ(wantlist->offered(), "0001-PANDE");
    EXPECT_EQ(wantlist->wanted_size(), 0);
    EXPECT_THAT(wantlist->GetExtension(InternalWantlist::username),
                StrCaseEq("user"));
  }
}

// Tests a small wantlist without colon or username.
TEST(WantlistParserTest, TestWantlist) {
  WantlistParser parser;
  const std::vector<std::string> items = {
      "0001-MKBG", "0002-TTAANSOC", "0003-PANDE", "0004-SCYTHE", "0005-PUERIC",
  };

  const auto wantlist = parser.ParseWantlist(absl::StrJoin(items, " "));
  ASSERT_TRUE(wantlist.ok());

  // Checks offered item id.
  EXPECT_EQ(wantlist->offered(), items[0]);
  EXPECT_FALSE(IsDummyItem(wantlist->offered()));

  // Verifies wantlist size and that all items are non-dummies.
  EXPECT_THAT(
      wantlist->wanted(),
      AllOf(SizeIs(Eq(4)),
            Each(Property(&WantedItem::id, ResultOf(IsDummyItem, IsFalse())))));

  // Verifies priorities and item names.
  EXPECT_THAT(
      wantlist->wanted(),
      ElementsAre(
          AllOf(PriorityEq(1), Property(&WantedItem::id, StrEq(items[1]))),
          AllOf(PriorityEq(2), Property(&WantedItem::id, StrEq(items[2]))),
          AllOf(PriorityEq(3), Property(&WantedItem::id, StrEq(items[3]))),
          AllOf(PriorityEq(4), Property(&WantedItem::id, StrEq(items[4])))));
}

// Tests a small wantlist with username and colon.
TEST(WantlistParserTest, TestWantlistWithUsernameAndColon) {
  WantlistParser parser;
  const std::vector<std::string> items = {
      "(username)", "0001-MKBG",   ":",           "0002-TTAANSOC",
      "0003-PANDE", "0004-SCYTHE", "0005-PUERIC",
  };

  const auto wantlist = parser.ParseWantlist(absl::StrJoin(items, " "));
  ASSERT_TRUE(wantlist.ok());

  // Checks offered item.
  EXPECT_EQ(wantlist->offered(), items[1]);
  EXPECT_FALSE(IsDummyItem(wantlist->offered()));
  EXPECT_THAT(wantlist->GetExtension(InternalWantlist::username),
              StrCaseEq("username"));

  // Verifies wantlist size and that all items are non-dummies.
  EXPECT_THAT(
      wantlist->wanted(),
      AllOf(SizeIs(Eq(4)),
            Each(Property(&WantedItem::id, ResultOf(IsDummyItem, IsFalse())))));

  // Verifies priorities and item names.
  EXPECT_THAT(
      wantlist->wanted(),
      ElementsAre(
          AllOf(PriorityEq(1), Property(&WantedItem::id, StrEq(items[3]))),
          AllOf(PriorityEq(2), Property(&WantedItem::id, StrEq(items[4]))),
          AllOf(PriorityEq(3), Property(&WantedItem::id, StrEq(items[5]))),
          AllOf(PriorityEq(4), Property(&WantedItem::id, StrEq(items[6])))));
}

// Test suite: dummy items

// Dummy offered item.
TEST(WantlistParserDummyItemsTest, TestDummyOfferedItem) {
  WantlistParser parser;
  const std::vector<std::string> items = {
      "(username)", "%0001-MKBG",  "0002-TTAANSOC",
      "0003-PANDE", "0004-SCYTHE", "0005-PUERIC",
  };

  const auto wantlist = parser.ParseWantlist(absl::StrJoin(items, " "));
  ASSERT_TRUE(wantlist.ok()) << wantlist.status().message();

  // Checks offered item.
  EXPECT_TRUE(IsDummyItem(wantlist->offered()));

  // Verifies wantlist size and that all items are non-dummies.
  EXPECT_THAT(
      wantlist->wanted(),
      AllOf(SizeIs(Eq(4)),
            Each(Property(&WantedItem::id, ResultOf(IsDummyItem, IsFalse())))));

  // Checks username annotation.
  EXPECT_THAT(*wantlist, HasModifiedDummyIds());
}

// Dummy wanted items: all.
TEST(WantlistParserDummyItemsTest, TesAllDummyWantedItems) {
  WantlistParser parser;
  const std::vector<std::string> items = {
      "(username)",  "0001-MKBG",    "%0002-TTAANSOC",
      "%0003-PANDE", "%0004-SCYTHE", "%0005-PUERIC",
  };

  const auto wantlist = parser.ParseWantlist(absl::StrJoin(items, " "));
  ASSERT_TRUE(wantlist.ok());

  // Checks offered item.
  EXPECT_FALSE(IsDummyItem(wantlist->offered()));

  // Verifies wantlist size and that all items are non-dummies.
  EXPECT_THAT(
      wantlist->wanted(),
      AllOf(SizeIs(Eq(4)),
            Each(Property(&WantedItem::id, ResultOf(IsDummyItem, IsTrue())))));

  // Checks username annotation.
  EXPECT_THAT(*wantlist, HasModifiedDummyIds());
}

// Dummy wanted items: some.
TEST(WantlistParserDummyItemsTest, TesSomeDummyWantedItems) {
  WantlistParser parser;
  const std::vector<std::string> items = {
      "(username)", "0001-MKBG",   "%0002-TTAANSOC",
      "0003-PANDE", "0004-SCYTHE", "%0005-PUERIC",
  };

  const auto wantlist = parser.ParseWantlist(absl::StrJoin(items, " "));
  ASSERT_TRUE(wantlist.ok());

  // Checks offered item.
  EXPECT_FALSE(IsDummyItem(wantlist->offered()));

  // Verifies that no wanted item is dummy.
  EXPECT_THAT(
      wantlist->wanted(),
      ElementsAre(Property(&WantedItem::id, ResultOf(IsDummyItem, IsTrue())),
                  Property(&WantedItem::id, ResultOf(IsDummyItem, IsFalse())),
                  Property(&WantedItem::id, ResultOf(IsDummyItem, IsFalse())),
                  Property(&WantedItem::id, ResultOf(IsDummyItem, IsTrue()))));

  // Checks username annotation.
  EXPECT_THAT(*wantlist, HasModifiedDummyIds());
}

// Test suite: negative tests

// Disabled because the ')' is regarded as an item.
TEST(WantlistParserNegativeTest, TestUsernameMissingParenthesisLeft) {
  WantlistParser parser;
  const std::string text = "user) 0001-A : 0002-B";
  EXPECT_TRUE(absl::IsInvalidArgument(parser.ParseWantlist(text).status()));
}

TEST(WantlistParserNegativeTest, TestUsernameMissingParenthesisRight) {
  WantlistParser parser;
  const std::string text = "(user 0001-A : 0002-B";
  EXPECT_TRUE(absl::IsInvalidArgument(parser.ParseWantlist(text).status()));
}

TEST(WantlistParserNegativeTest, TestExtraLeftParenthesis) {
  WantlistParser parser;
  const std::string text = "0001-A : (0002-B";
  EXPECT_TRUE(absl::IsInvalidArgument(parser.ParseWantlist(text).status()));
}

TEST(WantlistParserNegativeTest, TestExtraLeftParenthesisWithUsername) {
  WantlistParser parser;
  const std::string text = "(user) 0001-A : (0002-B";
  EXPECT_TRUE(absl::IsInvalidArgument(parser.ParseWantlist(text).status()));
}

TEST(WantlistParserNegativeTest, TestExtraRightParenthesis) {
  WantlistParser parser;
  const std::string text = "0001-A : 0002-B)";
  EXPECT_TRUE(absl::IsInvalidArgument(parser.ParseWantlist(text).status()));
}

TEST(WantlistParserNegativeTest, TestExtraRightParenthesisWithUsername) {
  WantlistParser parser;
  const std::string text = "(user) 0001-A : 0002-B)";
  EXPECT_TRUE(absl::IsInvalidArgument(parser.ParseWantlist(text).status()));
}

TEST(WantlistParserNegativeTest, TestExtraLeftRightParenthesis) {
  WantlistParser parser;
  const std::string text = "0001-A : (0002-B)";
  EXPECT_TRUE(absl::IsInvalidArgument(parser.ParseWantlist(text).status()));
}

TEST(WantlistParserNegativeTest, TestExtraLeftRightParenthesisWithUsername) {
  WantlistParser parser;
  const std::string text = "(user) 0001-A : (0002-B)";
  EXPECT_TRUE(absl::IsInvalidArgument(parser.ParseWantlist(text).status()));
}

TEST(WantlistParserNegativeTest, TestExtraColon) {
  WantlistParser parser;
  const std::string text = "(user) 0001-A : 0002-B : 0003-C";
  EXPECT_TRUE(absl::IsInvalidArgument(parser.ParseWantlist(text).status()));
}

// No colons should be specified after the first wanted item.
TEST(WantlistParserNegativeTest, TestColonAfterWantedItem) {
  WantlistParser parser;
  const std::string text = "0001-A 0002-B : 0003-C";
  EXPECT_TRUE(absl::IsInvalidArgument(parser.ParseWantlist(text).status()));
}

// No colons should be specified after the first wanted item.
TEST(WantlistParserNegativeTest, TestColonAfterWantedItemWithUsername) {
  WantlistParser parser;
  const std::string text = "(user) 0001-A 0002-B : 0003-C";
  EXPECT_TRUE(absl::IsInvalidArgument(parser.ParseWantlist(text).status()));
}

// Invalid characters in usernames: most special characters are allowed, except
// parentheses and colons.
TEST(WantlistParserNegativeTest, TestUsernameInvalidChars) {
  WantlistParser parser;
  // Invalid characters. Defining them as string_view to avoid the '\0'.
  static constexpr std::string_view kInvalidChars = R"tag(():)tag";

  for (char c : kInvalidChars) {
    std::string username = "user";
    username += c;
    const std::string text = absl::StrCat("(", username, ") 0001-A : 0002-B");
    EXPECT_TRUE(absl::IsInvalidArgument(parser.ParseWantlist(text).status()))
        << text;
  }

  // Verifies that the following characters are valid in usernames. This
  // includes the '\0' character.
  static constexpr char kValidChars[] = R"tag(`~!@#$%^&*+-=[]{}\|;'"<>?,./)tag";

  for (char c : kValidChars) {
    std::string username = "user";
    username += c;
    const std::string text = absl::StrCat("(", username, ") 0001-A : 0002-B");
    EXPECT_TRUE(parser.ParseWantlist(text).ok()) << text;
  }
}

TEST(WantlistParserNegativeTest, TestInvalidCharsInSuffix) {
  WantlistParser parser;
  const std::string invalid_chars = R"tag(`~!@#$^&*+={}[]\|;'",.<>/?)tag";

  for (char c : invalid_chars) {
    std::string text = "0001-A : 0002-B";
    text += c;
    const auto status = parser.ParseWantlist(text).status();
    const std::string_view message = status.message();

    EXPECT_TRUE(absl::IsInvalidArgument(status)) << text;

    // Checks that the forbidden character is included in the error message.
    EXPECT_THAT(message, HasSubstr(std::string(1, c))) << message;
  }
}

// A wantlist cannot contain a dummy offered and wanted item.
// The test must have a username, otherwise a `NotFoundError` will be raised.
// TODO(gioannidis) Determine whether this is a valid case.
TEST(WantlistParserNegativeTest, DISABLED_TestDummyOfferedAndWantedItems) {
  WantlistParser parser;
  const std::vector<std::string> items = {
      "(username) %0001-MKBG", "0002-TTAANSOC", "0003-PANDE",
      "%0004-SCYTHE",          "0005-PUERIC",
  };

  const auto wantlist = parser.ParseWantlist(absl::StrJoin(items, " "));
  EXPECT_TRUE(absl::IsInvalidArgument(wantlist.status()))
      << wantlist.status().message();
  EXPECT_FALSE(wantlist.ok());
}

// Usernames are required for dummy items (offered or wanted).
TEST(WantlistParserNegativeTest, TestDummyOfferedWithoutUsername) {
  WantlistParser parser;
  const std::vector<std::string> items = {
      "%0001-MKBG", "0002-TTAANSOC", "0003-PANDE", "0004-SCYTHE", "0005-PUERIC",
  };

  const auto wantlist = parser.ParseWantlist(absl::StrJoin(items, " "));
  EXPECT_TRUE(absl::IsNotFound(wantlist.status()))
      << wantlist.status().message();
}

// Usernames are required for dummy items (offered or wanted).
TEST(WantlistParserNegativeTest, TestDummyWantedWithoutUsername) {
  WantlistParser parser;
  const std::vector<std::string> items = {
      "0001-MKBG", "0002-TTAANSOC", "0003-PANDE", "%0004-SCYTHE", "0005-PUERIC",
  };

  const auto wantlist = parser.ParseWantlist(absl::StrJoin(items, " "));
  EXPECT_TRUE(absl::IsNotFound(wantlist.status()))
      << wantlist.status().message();
}

// Test suite: the underlying regex string.

// Creates a regex to match a wantlist prefix. Dies if it fails to compile.
// Note that this creates the same regex for every test, but we prefer it to
// test fixtures. See: https://abseil.io/tips/122
std::unique_ptr<re2::RE2> MakeRegex() {
  auto re = absl::make_unique<re2::RE2>(WantlistParser::get_regex_str());
  EXPECT_TRUE(re->ok()) << re->error();
  return re;
}

// Tests simple use-cases.
TEST(WantlistParserRegexTest, SimpleTest) {
  auto re = MakeRegex();
  const std::vector<std::string> wantlists = {
      "1-A",
      "1-A :",
      "1-A 2-B 3-C 4-D",
      "1-A : 2-B 3-C 4-D",
      "(user) 1-A : 2-B 3-C 4-D",
  };
  for (std::string_view wantlist : wantlists) {
    EXPECT_TRUE(re2::RE2::PartialMatch(wantlist, *re))
        << "Failed to match"
        << " wantlist: '" << wantlist << "'";
  }
}

// As in `SimpleTests`, but also captures groups.
TEST(WantlistParserRegexTest, SimpleCapturingTest) {
  auto re = MakeRegex();
  const std::vector<std::string> wantlists = {
      "1-A",
      "1-A :",
      "1-A 2-B 3-C 4-D",
      "1-A : 2-B 3-C 4-D",
      "(user) 1-A",
      "(user) 1-A :",
      "(user) 1-A : 2-B 3-C 4-D",
      "  (user)   1-A   :   2-B   3-C   4-D  ",  // extra whitespaces
  };

  // The capture groups.
  std::string username;
  std::string offered_item;
  std::string colon;

  // The expected capture groups.
  const std::vector<std::string> expected_username = {
      "", "", "", "", "user", "user", "user", "user"};
  const std::string expected_offered_item = "1-A";
  const std::vector<std::string> expected_colon = {"", ":", "",  ":",
                                                   "", ":", ":", ":"};

  // Size of expected results must be the same as the size of the tests.
  EXPECT_EQ(expected_username.size(), wantlists.size());
  EXPECT_EQ(expected_colon.size(), wantlists.size());

  int i = 0;
  for (std::string_view wantlist : wantlists) {
    EXPECT_TRUE(
        re2::RE2::PartialMatch(wantlist, *re, &username, &offered_item, &colon))
        << "Failed to match wantlist: '" << wantlist << "'";

    EXPECT_EQ(username, expected_username[i])
        << "Failed username match on wantlist: '" << wantlist << "'";
    EXPECT_EQ(offered_item, expected_offered_item)
        << "Failed offered_item match on wantlist: '" << wantlist << "'";
    EXPECT_EQ(colon, expected_colon[i])
        << "Failed colon match on wantlist: '" << wantlist << "'";
    ++i;
  }
}

// Missing offered item test.
TEST(WantlistParserRegexTest, MissingOfferedItemTest) {
  auto re = MakeRegex();
  static constexpr char wantlist[] = ": 1-A";
  EXPECT_FALSE(re2::RE2::PartialMatch(wantlist, *re));
}

}  // namespace
