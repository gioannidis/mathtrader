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

# Uses the `graphviz` package to render the .dot files that visualize the test
# examples. The output files are located in:
#
# $(bazel info bazel-bin)/mathtrader/solver/test_data/run_graphviz.runfiles/gioannidis_mathtrader/*.pdf

# Renders the .dot file.
function RenderDotFile {

  if [[ $# -ne 1 ]]; then
      >&2 echo "Missing filename. Usage: $0 <filename>"
      exit 2;
  fi

  fullfilename=$1
  filenamewithoutpath=$(basename -- "$fullfilename")  # strips path.
  filename="${filenamewithoutpath%.*}"
  circo -Tpdf ${fullfilename} > ${filename}.pdf
}

### Main script

required_binaries="circo"

# Checks if required binaries have been installed.
for required_binary in ${required_binaries[@]}; do
  if ! command -v ${required_binary} &> /dev/null; then
    >&2 echo "${required_binary} is required to run $0, but was not found."
    exit 1
  fi
done

# Parses each individual file.
for dot_file in mathtrader/solver/test_data/*.dot; do
  RenderDotFile ${dot_file}
done
