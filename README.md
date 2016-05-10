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
developed am algorithm which would run in polynomial time,
subsequently releasing the JAVA source code, dubbed as
[TradeMaximizer](https://www.boardgamegeek.com/wiki/page/TradeMaximizer).
This software has been used since then as the _de facto_ algorithm
to resolve Math Trades on BGG.

Since then, B. Perry
implemented a compatiblen multithreadedd C++ version of the software,
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

> We are given a directed graph G=(V,E) whose arcs have costs.
> Find a set of vertex-disjoint cycles that maximizes
> the number of covered vertices
> and minimizes the total arc cost among the
> set of optimal solutions.


## Dependencies

* The [LEMON Graph Library](http://lemon.cs.elte.hu/trac/lemon). To
install the LEMON library, please refer to the respective
[installation guide](http://lemon.cs.elte.hu/trac/lemon/wiki/InstallGuide).
* A C++ compiler with support of the C++11 standard.
On Linux systems the latest `g++` version provides support of the standard, e.g., g++ 4.9.2.
Many IDE on Windows systems support the C++11 standard,
e.g., [Code::Blocks](http://www.codeblocks.org/).
You might have to appropriately configure your IDE to enable C++11 support.

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
The official wantlists of previous Math Trades are linked
under the respective `[WANT]` tag.

On a terminal, you may download it using `wget`, e.g.:

    wget http://bgg.activityclub.org/olwlg/207635-officialwants.txt

You may see the options of `mathtrader++` by running `./mathtrader++ -h`.
The testcase file may be provided either from the standard input
or as a file:

    ./mathtrader++ < 207635-officialwants.txt
    ./mathtrader++ -f 207635-officialwants.txt

The results will be printed to the standard output.
You may redirect the output to a file:

    ./mathtrader++ < 207635-officialwants.txt > 207635-results.txt
    ./mathtrader++ --input-file 207635-officialwants.txt --output-file 207635-results.txt

In the following example,
the input file is downloaded from OLWLG and saved to the standard output
and subsequently the wantlist is piped to `mathtrader++`, where we also request to hide non-trading items.

     wget http://bgg.activityclub.org/olwlg/207635-officialwants.txt -O - | ./mathtrader++ --hide-no-trades > 207635-results.txt

## Future Tasks

- [x] Investigate possible algorithm bug, as it does not maximize the trading items on larger inputs.
- [ ] Implement scaled priority schemes.
- [x] Implement BIG-STEP and SMALL-STEP.
- [x] Add Doxygen documentation.
- [ ] Add _users-trading_ as a metric.
- [x] Integrate the file conversion with the main program.
- [ ] Allow spaces around colons `:` and semicolons `;` in want lists.
