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

#include <fstream>


/***************************************
 * 	PUBLIC METHODS - OUTPUT
 **************************************/

void
WantParser::print( const std::string & fn ) const {

	/* Open the file. */
	std::filebuf fb;
	auto fb_ptr = fb.open(fn, std::ios::out);

	/* Check if failed */
	if ( fb_ptr == NULL ) {
		throw std::runtime_error("Failed to open "
				+ fn);
	}

	/* Print the want-file. */
	std::ostream os(&fb);
	try {
		this->print(os);
	} catch ( const std::exception & e ) {
		/* If any exception is caught, close the file first.
		 * Then re-throw. */
		fb.close();
		throw;
	}

	/* Close the file. */
	fb.close();
}

void
WantParser::print( std::ostream &os ) const {

	// Print Nodes in LGF file
	os << "@nodes"
		<< std::endl
		<< "label" << "\t"
		<< "item" << "\t"
		<< "official_name" << "\t"
		<< "username" << "\t"
		<< "dummy"
		<< std::endl;

	for ( auto const & node : node_map_ ) {

		/* Get references to item details. */
		const std::string
			&item = node.second.item,
			&official_name = node.second.official_name,
			&username = node.second.username;

		/* Check if it has a want-list (present in the arc map). */
		const bool has_wantlist =
			(this->arc_map_.find(item) != this->arc_map_.end());

		/* Skip if it has no want-list. */
		if ( has_wantlist ) {

			const bool dummy = isDummy_(item);

			os << '"' << item << '"'	/* item also used as label */
				<< '\t'
				<< '"' << item << '"'
				<< '\t'
				<< '"' << official_name << '"'
				<< '\t'
				<< '"' << username << '"'
				<< '\t'
				<< dummy
				<< std::endl;
		}
	}

	// Print Arcs in LGF file
	os << "@arcs"
		<< std:: endl
		<< "\t" << "\t"
		<< "rank" << "\t"
		<< std::endl;

	for ( auto const & it : arc_map_ ) {

		auto const & arc_vector = it.second;

		for ( auto const & arc : arc_vector ) {

			auto const & target = arc.item_t;

			/* Valid if:
			 * 1) Target is in node map.
			 * 2) Target has a want-list.
			 */
			const bool valid =
				(node_map_.find(target) != node_map_.end())	/* valid target node */
				&& (arc_map_.find(target)  != arc_map_.end());	/* target node has a want-list too */

			if ( valid ) {
				os << '"' << arc.item_s << '"'
					<< '\t'
					<< '"' << arc.item_t << '"'
					<< '\t'
					<< arc.rank
					<< std::endl;
			}
		}
	}
}

void
WantParser::printOptions( std::ostream & os ) const {

	os << "Options: ";
	for ( auto const option : given_options_ ) {
		os << option << " ";
	}
	os << std::endl;
}

void
WantParser::printMissing( std::ostream & os ) const {

	unsigned count = 0;
	std::stringstream ss;

	for ( auto const & node_pair : node_map_ ) {

		const std::string & item = node_pair.first;
		auto const & node = node_pair.second;

		const bool has_wantlist =
			(this->arc_map_.find(item) != this->arc_map_.end());

		/* Report if want-list is empty and is NOT a dummy item. */
		if ( !this->isDummy_(item) && !has_wantlist ) {
			++ count;
			ss << "**** Missing want list for item "
				<< "\"" << item << "\""
				<< std::endl;
		}
	}

	if ( count > 0 ) {
		os << "MISSING ITEMS: "
			<< "(" << count
			<< " occurrence"
			<< ((count > 1)?"s":"")
			<< ")"
			<< std::endl
			<< ss.rdbuf()
			<< std::endl;
	}
}

void
WantParser::printErrors( std::ostream & os ) const {

	/* Print the preliminary line 'ERRORS'
	 * only if there are any actual errors to report. */
	if ( ! this->errors_.empty() ) {
		os << "ERRORS" << std::endl;
		for ( auto const & err : this->errors_ ) {
			os << "**** " << err << std::endl;
		}
	}
}

/********************************************************
 *	PUBLIC METHODS - EXTERNAL OPTIONS OUTPUT	*
 ********************************************************/

std::string
WantParser::getPriorityScheme() const {
	return priority_scheme_;
}

bool
WantParser::hideErrors() const {
	return this->bool_options_[HIDE_ERRORS];
}

bool
WantParser::hideLoops() const {
	return this->bool_options_[HIDE_LOOPS];
}

bool
WantParser::hideNonTrades() const {
	return this->bool_options_[HIDE_NONTRADES];
}

bool
WantParser::hideStats() const {
	return this->bool_options_[HIDE_STATS];
}

bool
WantParser::hideSummary() const {
	return this->bool_options_[HIDE_SUMMARY];
}

bool
WantParser::showElapsedTime() const {
	return this->bool_options_[SHOW_ELAPSED_TIME];
}

bool
WantParser::showMissing() const {
	return this->bool_options_[SHOW_MISSING];
}

bool
WantParser::sortByItem() const {
	return this->bool_options_[SORT_BY_ITEM];
}

/****************************************
 *	PUBLIC METHODS - STATS OUTPUT	*
 ****************************************/

unsigned
WantParser::getNumItems() const {
	/* Count all non-dummy items. */
	return std::count_if(
			this->node_map_.begin(),
			this->node_map_.end(),
			[]( const decltype(this->node_map_)::value_type & pair ) {
					/* Const reference to item being checked. */
					const auto & item = pair.second.item;
					/* True if item is not dummy. */
					return !isDummy_(item);
				}
			);
}

unsigned
WantParser::getNumMissingItems() const {
	/* Arc map to check for missing want-lists. */
	const auto & arc_map = this->arc_map_;

	/* Count items with missing wantlists. */
	return std::count_if(
			this->node_map_.begin(),
			this->node_map_.end(),
			[& arc_map]( const decltype(this->node_map_)::value_type & pair ) {
					/* Const reference to item being checked. */
					const auto & item = pair.second.item;
					/* Item has a want-list. */
					const bool has_wantlist = (arc_map.find(item)
						!= arc_map.end());
					/* True if want-list is missing. */
					return !has_wantlist;
				}
			);
}
