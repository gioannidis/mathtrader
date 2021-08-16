# mathtrader

_Note: `mathtrader` is currently migrating to version `2`, refactoring the code
base and switching optimization libraries._

Math Trades are trades where multiple people participate at once in order to
trade items between them. Each user defines multiple "want" relationships
between one or more items she offers and a items she would like to receive in
return. The goal of a Math Trading algorithm is to maximize the number of items
that will be eventually traded.

In this project we are approaching the real-world problem of
[Math Trades on BoardGameGeek (BGG)][bgg ref].

## Introduction

### Background

Math Trades have been taking place on BGG since the early 2000's. Various
algorithms had been developed at the time, most applying a brute-force search,
an approach which would only work for small datasets due to their exponential
asymptotic behavior. [TradeGenie][tradegenie ref], developed by
[B. Perry][user:perry ref], was a popular choice among the users.

In 2008, [Chris Okasaki][user:okasaki ref] developed an algorithm which would
run in polynomial time, subsequently releasing the JAVA source code, dubbed as
[TradeMaximizer][trademaximizer ref]. This software has been used since then as
the _de facto_ algorithm to resolve Math Trades on BGG.

Since then, B. Perry implemented a compatible multithreaded C++ version of the
software, [TradeThing][tradething ref]. According to preliminary results, the
performance of _TradeThing_ is quite comparable to _TradeMaximizer_'s.

Subsequently, the **goal** of this project is not to be a
yet-another-C++-adaption, but to explore and evaluate the following
alternatives:

1. Investigate whether the performance may be enhanced by utilizing specialized,
   open source Graph Libraries, that have been already optimized for
   performance. Our main focus should be on designing effective math trading
   solutions by applying standard, polynomial-time algorithms on the problem,
   and not on optimizing graph management functions, where we already have
   dedicated libraries at our disposal.
1. The original algorithm maximizes the number of traded items and uses "number
   of trading users" as a metric, over multiple iterations, to determine the
   best solution. We would like to explore the possibilities of amending the
   original algorithm itself in order to maximize the number of users that are
   trading at least one item, rather than rely on metrics. Therefore, we are
   also dealing with an algorithmic challenge.

### Formal problem

The Math Trading problem may be formally stated as:

> Given a weighted directed graph `G=(V,E)` with weights `w(E)`, find a set of
> vertex-disjoint cycles that maximizes the number of covered vertices and
> minimizes the total edge cost among the set of optimal solutions.

## Installing

### Required Packages

The following packages are required to build the library:

- C++ compiler with C++17 support.
  - `g++` compiler: version `8` or newer. Recommended: `10.2.1` or newer.
- [`bazel`][bazel ref] version `4` or newer. Recommended: `4.1.0` or newer.

### External Dependencies:

`mathtrader` uses the following external libraries, which should be
automatically integrated by the build system.

- [Abseil][abseil ref] common libraries. libraries, used to solve the math trade
  problem.
- [GoogleTest][gtest ref] testing and mocking framework.
- [OR-Tools][ortools ref] optimization
- [RE2][re2 ref] regular expression library.

*Note: earlier versions would also depend on the [Google Logging][glog ref]
Library, until it was replaced by the OR-Tools provided implementation.*

## Compiling

To be added.

## Running

To be added.

## Contributing

Pull requests are welcome. Please take a look at the
[technical notes][notes doc] for details on the style guide and automation
tools.

[abseil ref]: https://abseil.io
[bazel ref]: https://bazel.build
[bgg ref]: https://www.boardgamegeek.com/wiki/page/Math_Trades
[glog ref]: https://github.com/google/glog
[gtest ref]: http://google.github.io/googletest
[notes doc]: mathtrader/technical_notes.md
[ortools ref]: https://developers.google.com/optimization
[re2 ref]: https://github.com/google/re2
[tradegenie ref]: https://www.boardgamegeek.com/wiki/page/TradeGenie
[trademaximizer ref]: https://www.boardgamegeek.com/wiki/page/TradeMaximizer
[tradething ref]: https://sourceforge.net/projects/tradething/files/
[user:okasaki ref]: https://www.boardgamegeek.com/user/cokasaki
[user:perry ref]: https://www.boardgamegeek.com/user/Kayvon
