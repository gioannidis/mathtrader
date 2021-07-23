# Common types and functions

Defines types and functions that are commonly used in `mathtrader`. No code
should depend outside of `//mathtrader/common`, except for the standard
libraries. Definitions:

* `item.proto`: defines an exchangeable item.
* `wantlist.proto`: defines a relationship between items, where one item is
  offered in exchange for any one item from a list.
