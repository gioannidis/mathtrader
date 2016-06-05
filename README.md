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

## Formal problem

The Math Trading problem may be formally stated as:

> Given a weighted directed graph `G=(V,E)` with weights `w(E)`,
> find a set of vertex-disjoint cycles that maximizes
> the number of covered vertices
> and minimizes the total edge cost among the
> set of optimal solutions.


## Dependencies

* The [LEMON Graph Library](http://lemon.cs.elte.hu/trac/lemon). To
install the LEMON library, please refer to the respective
[installation guide](http://lemon.cs.elte.hu/trac/lemon/wiki/InstallGuide).
  * ``cmake 3.0.2`` or newer is recommended.
* ``g++ 4.9`` compiler or newer.
  * On a Windows IDE, you might have to appropriately configure your IDE to enable C++11 support.

## Compiling

* On Linux systems, running `make` on the top directory will compile the project.
* Optionally, you may also run `make html` to create the html documentation.
This requires the `doxygen` package.
* On Windows systems, you will have to pass the compiling options
found in the `Makefile` to your IDE's compiler.

## Running

_Note that this guide applies mostly on Linux systems_

Testcases from past trades may be found online
at the [Online Want List Generator (OLWLG)](http://bgg.activityclub.org/olwlg/).
The official wantslists of previous Math Trades are linked
under the respective `[WANT]` tag.

Note, that you may see the full option list of `mathtrader++` by running `./mathtrader++ -h`.

### Using a remote want-list file

To run `mathtrader++` directly on a want-list file from OLWLG, run:

    ./mathtrader++ --input-url http://bgg.activityclub.org/olwlg/207635-officialwants.txt

### Using a local want-list file

Alternatively, you may download a want-list file using `wget` or `curl`, e.g.:

    wget http://bgg.activityclub.org/olwlg/207635-officialwants.txt

The testcase file may be provided either from the standard input
or as a file:

    ./mathtrader++ < 207635-officialwants.txt
    ./mathtrader++ -f 207635-officialwants.txt

### Saving results to local file.

The results will be printed by default to the standard output.
You may redirect the output to a file or use the `--output-file` option:

    ./mathtrader++ --input-url http://bgg.activityclub.org/olwlg/207635-officialwants.txt > 207635-results-official.txt
    ./mathtrader++ --input-url http://bgg.activityclub.org/olwlg/207635-officialwants.txt --output-file 207635-results-official.txt

## Future Tasks

- [x] Resolve cost discrepancy vs other applications.
- [ ] Implement scaled priority schemes.
- [ ] Add _users-trading_ as a metric.
- [x] Catch user spelling errors by comparing items in official names with the given ones.
- [ ] Handle corner cases where there are no want lists at all.
- [x] Allow multiple options in one line.
- [ ] Parse EXPLICIT priorities; parse want lists formatted as ITEM=VALUE
- [x] Resolve cost of items protected by duplicate protection.
- [ ] Add I/O checksums.
- [x] Echo overridden options to results.
- [ ] Integrate ``libcurl`` to download remote want files.
