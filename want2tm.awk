#!/usr/bin/gawk

# This file is part of MathTrader++, a C++ utility
# for finding, on a directed graph whose arcs have costs,
# a set of vertex-disjoint cycles that maximizes the number
# of covered vertices as a first priority
# and minimizes the total cost as a second priority.
#
# Copyright (C) 2016 George Ioannidis
#
# MathTrader++ is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# MathTrader++ is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with MathTrader++.  If not, see <http://www.gnu.org/licenses/>.

#
# TODO:
# - Usernames with quotation marks or parentheses
#   may be parsed incorrectly.
# - Do not ignore lines with options, starting with "#!"
# - Official game names with quotation marks are parsed incorrectly;
#   e.g."Pathfinder Adventure Card Game: Wrath of the Righteous – "Valais Durant" Promo"
#   in trade #199349
#

BEGIN {
	##
	# Fields:
	# - Anything that is not blank
	# - Left "(", anything not a ")", and a final ")"
	# - Note: parentheses have to be escaped TWICE
	##
	FPAT = "([^[:blank:]]*)|(\"[^\"]+\")|(\\([^\\)]+\\))"

	official_nodes = 1
	n_arcs = 0

	###
	# Start the @nodes section
	###
	print "@nodes"
	print "label" "\t\t" "item" "\t\t" "dummy" "\t" "username" "\t" "official_name"
}
#
# Ignore white spaces and comments
#
!/^($|#)/{
	if ( official_nodes == 1 ) {

		switch ( $1 ) {
			case "!BEGIN-OFFICIAL-NAMES":
				# do nothing
				break
			case "!END-OFFICIAL-NAMES":
				official_nodes = 0
				break
			default:
				###
				# Username is found in "(from XXX)".
				# Isolate username.
				# Define "(from " as a delimeter,
				# as some usernames may contain whitespaces.
				# Remove the last ")" from the username.
				###
				user = $4
				split( user, elem, "(\\(from )")
				user = elem[2]
				user = substr(user, 1, length(user) - 1)

				###
				# Print node
				# 1. label
				# 2. item (same as label)
				# 3. dummy
				# 4. username
				# 5. official name of item
				###
				item = toupper($1)
				print item "\t" \
				      item "\t" \
				      "0" "\t" \
				      quote(user) "\t\t" \
				      $3
				break
		}
	} else {

		###
		# This is an arc.
		# OLWLG format of an arc:
		#	(USERNAME) NODE1 : NODE2 NODE3 NODE4 ...
		#
		# Append the arc to an array to print it
		# after any dummy nodes have been processed.
		###
		arcs[n_arcs] = $0
		++ n_arcs

		###
		# Check whether this item is a dummy.
		###
		item = toupper($2)
		if ( item ~ /^%/ ) {

			###
			# Username is found in "(XXX)"
			# Isolate username from parentheses.
			# Append username to item.
			###
			user = $1
			user = substr(user, 2, length(user) - 2)
			item = dummyCheck( item, user )

			###
			# Print the dummy node
			# label-item-dummy-username-"DUMMY" (as official_name)
			###
			print item "\t" item "\t" "1" "\t" quote(user) "\t\t" "DUMMY"
		}

	}

}
END {
	###
	# Print all the arcs that
	# have been previously stored
	###
	print "@arcs"
	print "\t\t\t\t" "rank" "\t" "username"

	for ( i = 0; i < n_arcs; i ++ ) {

		###
		# Split the arc
		# The FPAT will be used
		###
		n = patsplit( arcs[i], elem )

		###
		# Get user and source
		# If source is dummy, append username
		###
		user = elem[1]
		user = substr(user, 2, length(user) - 2) #!< remove parentheses

		###
		# Source node: append username if dummy
		###
		n_source = toupper(elem[2])
		n_source = dummyCheck(n_source, user)

		rank = 0
		for ( j = 4; j <= n; j ++ ) {
			++ rank

			###
			# Target node: append username if dummy
			###
			n_target = toupper(elem[j])
			n_target = dummyCheck(n_target, user)

			###
			# Missing officials:
			# Check if n_target is missing;
			# skip if so.
			###
			if ( n_target !~ /^MISSING/ && n_target !~ /^\"MISSING/ ) {

				###
				# Print arc
				# 1. Source node
				# 2. Target node
				# 3. Rank
				# 4. Username
				###
				print n_source "\t" \
				      n_target "\t" \
				      rank "\t" \
				      quote( user )
			}
		}
	}
}


###
# @brief Append username to dummy item
# Checks whether a given node is a dummy
# and appends the username if so.
###
function dummyCheck( node, user ) {
	if ( node ~ /^%/ ) {
		return "\"" node "-(" user ")\""
	} else {
		return "\"" node "\""
	}
}


###
# @brief Quote
# Appends quotation marks
# if not already beginning
###
function quote( str ) {
	if ( str !~ /^\"/ ) {
		return "\"" str "\""
	} else {
		return str
	}
}
