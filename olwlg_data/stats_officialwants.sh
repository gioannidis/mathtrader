#!/bin/bash

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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with mathtrader. If not, see <http://www.gnu.org/licenses/>.

# Library used to retrieve quick statistics about a OLWLG-generated
# "officialwants" file. It is not meant to be deployed for accurate parsing, but
# merely as a straightforward way to retrieve statistics for test development.
# Therefore, it is intended to be run in a Linux environment
# Run as:
#
#   bazel run //olwlg_data:stats_officialwants

# Retrieves statistics from a given official wants file.
function GetStatsFromOfficialwantsFile {

  if [[ $# -ne 1 ]]; then
      >&2 echo "Missing filename. Usage: $0 <filename>"
      exit 2;
  fi

  # Merely echos the file being parsed.
  echo "-----"
  echo "Parsing file: $1"

  # Retrieves the users from the OFFICIAL-NAMES section, e.g., "(from username)".
  echo -n "Users: "
  pcregrep -e "\(from (.+)\)" -o1 $1 | sort | uniq | wc -l

  # Retrieves the number of official items by looking up the pattern "==>".
  echo -n "Official items: "
  egrep -c "==>" $1

  # Retrieves the number of wantlists, looking up lines beginning with a "(".
  echo -n "Wantlists: "
  egrep -c "^\(" $1

  # Retrieves the number of dummy wantlists.
  echo -n "Dummy wantlists: "
  egrep -c "^\(.*\)\s*%" $1

  # Retrieves the number of items in all wantlists, including the offered items.
  # This is indicative of the number of arcs that will be generated. Subtracts
  # the username and the colon from the number of fields.
  echo -n "Total items in all wantlists: "
  grep "^(" $1 | awk 'BEGIN{sum = 0}{sum += NF-2}END{print sum}'

  # As above, but retrieves the longest wantlist.
  echo -n "Longest wantlist (#wanted items): "
  grep "^(" $1 \
      | awk 'BEGIN{max = 0}{n = NF-3; if(n > max){max = n}}END{print max}'

  # Retrieves the number of items named "missing-official". "sed" adds a line
  # break after each occurence, because "grep" cannot count multiple matches
  # within a single line. Finally, filters out comment lines beginning with '#'.
  missing="missing-official"
  echo -n "${missing} items: "
  sed "s/${missing}/${missing}\\n/gi" $1 | egrep -i "missing-official" \
      | egrep -c "^[^#]"
}

### Main script

required_binaries="egrep pcregrep sed"

# Checks if required binaries have been installed.
for required_binary in ${required_binaries[@]}; do
  if ! command -v ${required_binary} &> /dev/null; then
    >&2 echo "${required_binary} is required to run $0, but was not found."
    exit 1
  fi
done

# Parses each individual file.
for officialwants_file in olwlg_data/*-officialwants.txt; do
  GetStatsFromOfficialwantsFile ${officialwants_file}
done
