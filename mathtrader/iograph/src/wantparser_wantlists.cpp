/* This file is part of MathTrader++.
 *
 * Copyright (C) 2018 George Ioannidis
 *
 * MathTrader++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MathTrader++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MathTrader++.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <iograph/wantparser.hpp>

void
WantParser::parseWantList_( const std::string & line ) {

	static const std::regex FPAT_want(
		R"(\([^\)]+\))"		// Group 1: parentheses
		"|"
		R"([^\s:;]+)"		// Group 2: any non-whitespace,
					// colon, or semicolon
		"|"
		R"(:)"
		"|"
		R"(;)"
	);

	/* Summary:
	 * 1. Tokenize the line.
	 * 2. Parse username.
	 * 3. Parse offering item name (source).
	 * 4. Parse colon.
	 * 5. Parse wanted items (targets).
	 *
	 * Full want list format:
	 * (user name) ITEM_A : ITEM_B ITEM_C ; ITEM_D %DUMMY1 %DUMMY2 ITEM_E
	 *
	 * REQUIRE-USERNAMES: "(user name)" are mandatories; optional otherwise.
	 * REQUIRE-COLONS: ":" are mandatories; optional otherwise.
	 * ALLOW-DUMMIES: not possible if REQUIRE-USERNAMES not set.
	 */

	/* Tokenize the line. */
	auto const match = split_( line, FPAT_want );
	if ( match.empty() ) {
		throw std::runtime_error("Bad format of want list");
	}

	/********************************
	 * 	PARSE USERNAME		*
	 ********************************/

	unsigned n_pos = 0;	/* current item that is being parsed */

	std::string username = extractUsername_(match.at(n_pos));

	/* Go to the next element if we have a valid username.
	 * If we are missing a required username stop here. */
	if ( !username.empty() ) {
		++ n_pos ;
	} else if (  this->bool_options_[ REQUIRE_USERNAMES ] ) {
		throw std::runtime_error("Missing username from want list");
	}

	/****************************************
	 *	OFFERED ITEM NAME (source)	*
	 ****************************************/

	/* Check whether we have reached the end of the line.
	 * If so, the wanted item name is missing. */
	if ( n_pos >= match.size() ) {
		throw std::runtime_error("Missing offered item from want list");
	}
	const std::string & original_source = match.at(n_pos);

	/* Convert item name. */
	const std::string source = convertItemName_( original_source, username );

	/* Add source item.
	 * Item name is also used as the 'official' name
	 */
	this->addSourceItem_( source, source, username );

	/* Finally, advance n_pos.
	 * We should always have an offering item. */
	++ n_pos;


	/********************************
	 *	CHECK COLONS		*
	 ********************************/

	/* Dependent scope to open local variables. */
	{
		const bool has_colon = (n_pos < match.size())
				&& (match.at(n_pos).compare(":") == 0);

		/* Advance n_pos if we have a colon. */
		if ( has_colon ) {
			++ n_pos;
		} else if ( this->bool_options_[REQUIRE_COLONS] ) {
			throw std::runtime_error("Missing colon from want list");
		}
	}


	/********************************
	 *	WANTED ITEMS (targets)	*
	 ********************************/

	/* Copy wanted items; first begins at n_pos */
	const auto wanted_items = std::vector< std::string >(
			match.begin() + n_pos,
			match.end());

	this->addTargetItems_( source, wanted_items );
}

void
WantParser::addSourceItem_( const std::string & source,
		const std::string & official_name,
		const std::string & username) {

	/* Check if the source item is present in node_map. */
	auto it = node_map_.find( source );

	if ( it == node_map_.end() ) {

		/* Source item is NOT in node_map. */
		switch ( this->status_ ) {
			case PARSE_NAMES:
			case PARSE_WANTS_NONAMES: {

				/* We are either:
				 * 1. currently reading official names; OR
				 * 2. currently reading want-lists without
				 *    official names.
				 *
				 * These should be no entries in these cases.
				 * Break here, add later.
				 */
				break;
			}
			case PARSE_WANTS_WITHNAMES: {

				/* We are currently reading want-lists
				 * WITH official names previously given.
				 * There SHOULD be an entry
				 * with a corresponding official name,
				 * unless it's a dummy item.
				 * This check usually catches spelling errors.
				 *
				 * Therefore, raise an error only if it's
				 * a non-dummy item.
				 * Otherwise, proceed to add.
				 */
				if ( !isDummy_(source) ) {

					throw std::runtime_error("Non-dummy item "
							+ source
							+ " has no official name."
							" Hint: spelling error?");
				}
				break;
			}
			default: {
				throw std::logic_error("Unknown handler for internal status "
						+ std::to_string( this->status_ )
						+ "; source item not found in node map.");
				break;
			}
		}

		/* Insert the item in the node_map. */
		auto const pair = this->node_map_.emplace(
				source,
				Node_t_(source, official_name, username)
				);

		/* Insert should have succeeded. */
		if ( !pair.second ) {
			throw std::logic_error("Could not insert node in node_map_.");
		}

	} else {
		/* Source item IS in node_map. */
		switch ( this->status_ ) {
			case PARSE_NAMES: {

				/* We are currently reading official names.
				 * Ignore any item attempted to be inserted twice.
				 */
				throw std::runtime_error("Existing entry for item "
						+ source);
				break;
			}
			case PARSE_WANTS_WITHNAMES:
			case PARSE_WANTS_NONAMES: {

				/* We are currently reading want-lists either:
				 * 1. without official names; OR
				 * 2. with official names.
				 *
				 * If a want-list exists: ignore.
				 * Otherwise:
				 * - If we have official names, do nothing. We had previously inserted the item
				 *   in parseOfficialName_.
				 * - If we do not have official names, it's a logic error.
				 */
				const bool source_has_wantlist =
					(this->arc_map_.find(source) != this->arc_map_.end());

				if ( source_has_wantlist ) {
					/* Condition must be true. */
					throw std::runtime_error("Ignoring multiple wantlist for item "
							+ source);
				} else if ( this->status_ == PARSE_WANTS_NONAMES ) {
					/* Sanity check. If no official names are being read
					 * an existing item MUST have a want-list. */
					throw std::logic_error("Existing item found in node list map "
							"without a want-list, but official names "
							"have not been given.");
				}
				break;
			}
			default: {
				throw std::logic_error("Unknown handler for internal status "
						+ std::to_string( this->status_ )
						+ "; source item already found in node map.");
				break;
			}
		}
	}
}

void
WantParser::addTargetItems_( const std::string & source, const std::vector< std::string > & wanted_items ) {

	/* Check if want list already exists.
	 * This may happen if a user has defined multiple want lists
	 * or another line was split over two lines.
	 */
	if ( arc_map_.find(source) != arc_map_.end() ) {
		throw std::runtime_error("Multiple want lists for item "
				+ source
				+ ". Hint: check if an item want-list line has been split"
				+ " over two lines.");
	}

	/* Initialize rank. */
	int rank = 1;

	/* Initialize list with want-lists to be added.
	 * All parsed want lists are added to this map
	 * and they will be eventually added
	 * to the official graph
	 * if *no errors* whatsoever are detected.
	 * On errors, the whole line is discarded.
	 */
	std::list< Arc_t_ > arcs_to_add;

	/********************************
	 *	WANTED ITEMS ITERATOR	*
	 ********************************/

	for ( const auto & target : wanted_items ) {

		/* Small and big steps. */
		const auto register & small_step = int_options_[SMALL_STEP];
		const auto register & big_step   = int_options_[BIG_STEP];

		/* Cases:
		 * 1. Semicolon:
		 * 	"increase the rank of the next item by the big-step value"
		 * 	NOTE: the small-step of the previous item will also be applied.
		 * 2. Colon: raise an error. There should be no colon here.
		 * 3. Actual wanted item.
		 */
		if ( target.compare(";") == 0 ) {
			rank += big_step;
		} else if ( target.compare(":") == 0 ) {
			throw std::runtime_error("Invalid colon occurence.");
		} else {

			/* Parse the item name (dummy, uppercase, etc). */
			const auto & username = this->node_map_.at( source ).username;
			const auto converted_target_name = convertItemName_( target, username );

			/* Push (item-target) arc to map. */
			arcs_to_add.push_back(Arc_t_( source, converted_target_name, rank ));
		}

		/* Advance always the rank by small-step. */
		rank += small_step;
	}

	/* Create ArcMap entry for item;
	 * in C++11 we can directly move the items from the original list
	 * to the vector; we don't have to copy them!
	 */
	auto pair = arc_map_.emplace(
			source,
			std::vector< Arc_t_ > {
				std::make_move_iterator(std::begin(arcs_to_add)),
				std::make_move_iterator(std::end(arcs_to_add)) }
			);

	/* Insertion should have succeeded. */
	if ( !pair.second ) {
		throw std::logic_error("Could not insert arcs in arc_map_.");
	}
}
