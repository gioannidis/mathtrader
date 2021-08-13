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

#include <cstdint>
#include <string>
#include <vector>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "glog/logging.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/wantlist.pb.h"
#include "mathtrader/parser/internal/internal_wantlist.pb.h"
#include "mathtrader/parser/util/item_util.h"
#include "mathtrader/util/str_toupper.h"

namespace mathtrader::parser::internal {
namespace {
using ::mathtrader::common::Item;
using ::mathtrader::common::Wantlist;
using ::mathtrader::util::StrToUpper;

// Internal regex that matches wantlist prefix, capturing offered item id and
// optionally the username.
constexpr absl::string_view kWantlistPrefixRegexStr
    // Prefix matching at the beginning of the text.
    = "^"

      // Captures optional username. Expected format:
      //    (USERNAME)
      R"regex(\s*(?:\((.+)\))?\s*)regex"

      // Captures item id. Excludes ':' characters and whitespaces.
      // Also filters trailing whitespaces.
      R"regex(([^:\s\(\)]+)\s*)regex"

      // Captures optional colon character.
      R"regex((:)?\s*)regex";

// Conditionally updates a status ok with an error message if the maximum number
// of characters has been exceeded.
void UpdateOnError(char c, int count, int max_count, absl::Status* status) {
  CHECK_NOTNULL(status);
  if (count > max_count) {
    status->Update(absl::InvalidArgumentError(absl::StrFormat(
        "Found %d '%c' characters in wantlist. Maximum allowed: %d", count, c,
        max_count)));
  }
}

// Checks a wantlist text for integrity errors and forbidden characters.
// @text  Entire wantlist, including username.
// @wantlist  Suffix of @text, which excludes the username.
ABSL_MUST_USE_RESULT absl::Status CheckWantlist(absl::string_view text,
                                                absl::string_view wantlist) {
  static constexpr int kMaxColonCount = 1;
  static constexpr int kMaxParenthesisCount = 1;
  static constexpr absl::string_view kForbiddenChars =
      R"tag(`~!@#$^&*=+(){}[]\|;'",.<>/?)tag";

  {
    auto pos = wantlist.find_first_of(kForbiddenChars);
    if (pos != std::string::npos) {
      return absl::InvalidArgumentError(
          absl::StrFormat("Character '%c' is not allowed in wantlists. "
                          "List of forbidden characters: %s",
                          text[pos], kForbiddenChars));
    }
  }

  int colon_count = 0;
  int left_parenthesis_count = 0;
  int right_parenthesis_count = 0;

  // Counts the number of max allowed characters.
  for (char c : text) {
    switch (c) {
      case ':': {
        ++colon_count;
        break;
      }
      case '(': {
        ++left_parenthesis_count;
        break;
      }
      case ')': {
        ++right_parenthesis_count;
        break;
      }
      default: {
        break;
      }
    }
  }

  auto ret = absl::OkStatus();
  UpdateOnError(':', colon_count, kMaxColonCount, &ret);
  UpdateOnError('(', left_parenthesis_count, kMaxParenthesisCount, &ret);
  UpdateOnError(')', right_parenthesis_count, kMaxParenthesisCount, &ret);

  // Checks number of matching parentheses.
  if (left_parenthesis_count != right_parenthesis_count) {
    ret.Update(absl::InvalidArgumentError(absl::StrFormat(
        "Number of '(' (%d) and ')' (%d) characters in wantlist must match.",
        left_parenthesis_count, right_parenthesis_count)));
  }
  return ret;
}
}  // namespace

// Constructs the parser and dies if the regular expression was not created
// properly.
WantlistParser::WantlistParser(int32_t small_step, int32_t big_step)
    : kSmallStep(small_step),
      kBigStep(big_step),
      kWantlistPrefixRegex(kWantlistPrefixRegexStr) {
  CHECK(kWantlistPrefixRegex.ok()) << "Could not create regular expression: "
                                   << kWantlistPrefixRegex.error();
}

absl::StatusOr<Wantlist> WantlistParser::ParseWantlist(
    absl::string_view text) const {
  // Creates the wantlist to return, currently empty.
  Wantlist wantlist;

  // Converts string_view to StringPiece, so that RE2 can consume its prefix.
  re2::StringPiece text_piece = text;
  Item* offered_item = nullptr;
  {
    std::string item_id;
    std::string colon;     // optional
    std::string username;  // optional

    // Matches and consumes the wantlist prefix containing the username
    // (optional), the offered item and a separating colon (optional).
    if (!re2::RE2::Consume(&text_piece, kWantlistPrefixRegex, &username,
                           &item_id, &colon)) {
      return absl::InvalidArgumentError(
          "Could not retrieve the username and/or the offered item. The "
          "wantlist must begin with '(username) 0001-ITEM' or '0001-ITEM'. "
          "(Hint: usernames must be at least 4 characters long, begin with "
          "an alpha character and only contain alphanumeric characters or "
          "underscores '_'.)");
    }

    // Checks the integrity of the wantlist, excluding the username. Must be
    // called before converting the username to case-insensitive, if applicable.
    const absl::string_view text_without_username =
        text.substr(text.find(username) + username.size() + 1);
    if (absl::Status status = CheckWantlist(text, text_without_username);
        !status.ok()) {
      return status;
    }

    // Makes the id and username case-insensitive.
    item_id = StrToUpper(item_id);
    username = StrToUpper(username);

    // Checks if colons are specified after wanted items.
    if (text_piece.find(':') != std::string::npos) {
      return absl::InvalidArgumentError(
          "Specifying a colon ':' after the first wanted item is not allowed.");
    }

    // Sets the offered item id.
    offered_item = wantlist.mutable_offered_item();
    offered_item->set_id(item_id);
    wantlist.set_offered(std::move(item_id));

    // Sets the wantlist username, if specified. Should be done before
    // processing a potentially dummy item.
    if (!username.empty()) {
      offered_item->set_username(username);
      wantlist.SetExtension(InternalWantlist::username, std::move(username));
    } else {
      // TODO(gioannidis) return if usernames are required.
    }

    // Processes the item id if dummy.
    if (const absl::Status status = util::ProcessIfDummy(offered_item);
        !status.ok()) {
      return status;
    }
    if (const absl::Status status = util::ProcessIfDummy(username, &item_id);
        !status.ok()) {
      return status;
    }
  }
  CHECK_NOTNULL(offered_item);

  // Splits the rest of text using spaces as delimiter, skipping empty strings
  // and strings containing only whitespaces. The rest of the text should
  // contain the wanted items or the `kBigStepChar`.
  const std::string wanted_items(text_piece);
  std::vector<absl::string_view> tokens =
      absl::StrSplit(wanted_items, ' ', absl::SkipWhitespace());

  // Empty wantlists, without wanted items, are allowed.
  if (tokens.empty()) {
    return wantlist;
  }

  // The rank of the first wanted item is by default `1`.
  int32_t rank = 1;

  // Parses the tokens and adds a wanted item, except if a `kBigStepChar` is
  // seen, where it further increases the next item's rank.
  for (absl::string_view token : tokens) {
    if (token.size() == 1 && token[0] == kBigStepChar) {
      // Not a wanted item, but a "big step" character.
      rank += kBigStep;
      continue;
    }
    Item* wanted_item = wantlist.add_wanted_item();
    Wantlist::WantedItem* wanted = wantlist.add_wanted();

    // Makes the wanted item id case-insensitive.
    {
      auto wanted_item_id = static_cast<std::string>(token);
      wanted_item->set_id(StrToUpper(std::move(wanted_item_id)));
    }
    wanted->set_id(std::move(std::string(token)));
    StrToUpper(wanted->mutable_id());

    // Processes the item id if dummy, returning on error.
    if (const absl::Status status = util::ProcessIfDummy(
            /*username=*/wantlist.GetExtension(InternalWantlist::username),
            /*item_id=*/wanted->mutable_id());
        !status.ok()) {
      return status;
    }
    if (const absl::Status status =
            util::ProcessIfDummy(offered_item->username(), wanted_item);
        !status.ok()) {
      return status;
    }

    const int32_t priority = ComputePriority(rank);
    wanted_item->set_priority(priority);
    wanted->set_priority(priority);
    rank += kSmallStep;
  }
  return wantlist;
}

int32_t WantlistParser::ComputePriority(int32_t rank) const { return rank; }

// Returns the internal string that is used to build the regex that parses the
// wantlists.
absl::string_view WantlistParser::get_regex_str() {
  return kWantlistPrefixRegexStr;
}
}  // namespace mathtrader::parser::internal
