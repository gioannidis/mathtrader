# mathtrader

_Note: `mathtrader` is currently migrating to version `2`, refactoring the code
base and switching optimization libraries._

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

## Installing

### Required Packages

The following packages are required to build the library:

* C++ compiler with C++17 support.
  * ``g++`` compiler: version ``8`` or newer. Recommended: ``10.2.1`` or newer.
* [``bazel``](https://bazel.build) version ``4`` or newer. Recommended:
  ``4.1.0`` or newer.

### External Dependencies:

`mathtrader` uses the following external libraries, which should be
automatically integrated by the build system.

* [Abseil](https://abseil.io) common libraries.
  libraries, used to solve the math trade problem.
* [Google Logging](https://github.com/google/glog) Library.
* [GoogleTest](http://google.github.io/googletest/) testing and mocking
  framework.
* [OR-Tools](https://developers.google.com/optimization/) optimization
* [RE2](https://github.com/google/re2) regular expression library.

## Compiling

To be added.

## Running

To be added.
