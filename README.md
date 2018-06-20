# mathtrader++

Math Trades are trades where multiple people participate at once
in order to trade items between them.
Each user defines multiple "want" relationships between
one or more items she offers and a items she would like to receive in return.
The goal of a Math Trading algorithm
is to maximize the number of items that will be eventually traded.

In this project we are approaching the real-world problem of
[Math Trades on BoardGameGeek (BGG)](https://www.boardgamegeek.com/wiki/page/Math_Trades)

## Introduction

### Background

Math Trades have been taking place on BGG since the early 2000's.
Various algorithms had been developed at the time,
most applying a brute-force search,
an approach which would only work for small datasets
due to their exponential asymptotic behavior.
[TradeGenie](https://www.boardgamegeek.com/wiki/page/TradeGenie),
developed by [B. Perry](https://www.boardgamegeek.com/user/Kayvon),
was a popular choice among the users.

In 2008, [Chris Okasaki](https://www.boardgamegeek.com/user/cokasaki)
developed an algorithm which would run in polynomial time,
subsequently releasing the JAVA source code, dubbed as
[TradeMaximizer](https://www.boardgamegeek.com/wiki/page/TradeMaximizer).
This software has been used since then as the _de facto_ algorithm
to resolve Math Trades on BGG.

Since then, B. Perry
implemented a compatible multithreaded C++ version of the software,
[TradeThing](https://sourceforge.net/projects/tradething/files/).
According to preliminary results,
the performance of _TradeThing_ is quite comparable to _TradeMaximizer_'s.

Subsequently, the **goal** of this project is not to be a yet-another-C++-adaption,
but to explore and evaluate the following alternatives:

1. Investigate whether the performance may be enhanced by utilizing
specialized, open source Graph Libraries,
that have been already optimized for performance.
Our main focus should be on designing effective math trading solutions
by applying standard, polynomial-time algorithms on the problem,
and not on optimizing graph management functions,
where we already have dedicated libraries at our disposal.
2. The original algorithm maximizes the number of traded items
and uses "number of trading users" as a metric, over multiple iterations,
to determine the best solution.
We would like to explore the possibilities of amending the original algorithm
itself in order to maximize the number of users that are trading at least one item,
rather than rely on metrics.
Therefore, we are also dealing with an algorithmic challenge.

### Formal problem

The Math Trading problem may be formally stated as:

> Given a weighted directed graph `G=(V,E)` with weights `w(E)`,
> find a set of vertex-disjoint cycles that maximizes
> the number of covered vertices
> and minimizes the total edge cost among the
> set of optimal solutions.

## Packages

### Required Packages

The following packages are required to build the library:

* ``g++`` compiler version ``4.9.2`` or newer
* ``cmake`` version ``3.0.2`` or newer
* The [LEMON Graph Library](http://lemon.cs.elte.hu/trac/lemon). See
<a href=https://github.com/gioannidis/mathtrader/blob/master/doc/LemonInstall.md>
Installing the LEMON library</a>.

### Optional Packages

#### Documentation

To generate the Doxygen-supported documentation, the following optional packages are required:

* ``doxygen``
* ``graphviz``

#### Unit Testing

The following package is required
to run the library unit tests under the Google Test framework:

* ``libgtest-dev``

## Compiling

* On Linux systems, execute from the top directory:

		mkdir build
		cd build
		cmake ..
		make

This creates the ``mathtrader++`` executable under ``build/app/mathtrader++``.

* Optionally, you may also run `make doc` to create the documentation
if ``doxygen`` has been installed.

## Running

_Note that this guide applies mostly on Linux systems._

Testcases from past trades may be found online
at the [Online Want List Generator (OLWLG)](http://bgg.activityclub.org/olwlg/).
The official wantslists of previous Math Trades are linked
under the respective `[WANT]` tag.

The ``mathtrader++`` executable is found under ``build/app/mathtrader++``.
You may see the full option list of `mathtrader++` by running `./mathtrader++ -h`.

### Using a remote want-list file

To run `mathtrader++` directly on a want-list file from OLWLG, run:

    ./mathtrader++ --input-url http://bgg.activityclub.org/olwlg/207635-officialwants.txt

### Using a local want-list file

Alternatively, you may download a want-list file using `wget` or `curl`, e.g.:

    wget http://bgg.activityclub.org/olwlg/207635-officialwants.txt

The testcase file may be provided either from the standard input
or as a file:

    ./mathtrader++ < 207635-officialwants.txt
    ./mathtrader++ --input-file 207635-officialwants.txt

### Saving results to local file.

The results will be printed by default to the standard output.
You may redirect the output to a file or use the `--output-file` option:

    ./mathtrader++ --input-url http://bgg.activityclub.org/olwlg/207635-officialwants.txt > 207635-results-official.txt
    ./mathtrader++ --input-url http://bgg.activityclub.org/olwlg/207635-officialwants.txt --output-file 207635-results-official.txt

## Documentation

This library has been documented using ``Doxygen``.
To build it, type ``make doc`` within the ``build/`` directory.

### ``html`` documentation

Open in a web browser the ``build/html/index.html`` file.

### ``LaTeX`` documentation

A ``refman.pdf`` file may be compiled with the entire documentation.
Execute the following steps from the ``build/`` directory:

1. ``cd latex``
2. ``make``
3. Open the ``refman.pdf`` file.

## Unit Testing

Unit tests under the Google Test framework are provided.
The Google Test framework package is required to build them.
The following testing executables are compiled:

* ``build/lib/iograph/testiograph`` : test the library that parses the want-list files
* ``build/lib/solver/testsolver`` : test the library that solves the math trades

Simply run the executables to test the libraries.

## Future Tasks

- [ ] Implement scaled priority schemes.
- [ ] Add _users-trading_ as a metric.
- [ ] Handle corner cases where there are no want lists at all.
- [ ] Parse EXPLICIT priorities; parse want lists formatted as ITEM=VALUE
- [ ] Add I/O checksums.
- [ ] Write own method in WantParser::parseLine to handle directives.
- [ ] If an OPTION fails to parse, do not skip the entire line, if multiple options are given in a single line.
- [ ] Report unknown items, i.e., items that appear in want lists but have not been previously defined.
