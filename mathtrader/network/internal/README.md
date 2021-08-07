# Internal `NetworkBuilder`

Implements the internal modules of `NetworkBuilder`, which generates a
`Assignment` out of a `ParserResult`:

* `NodeBuilder`: generates two nodes for each offered item: one `Node` to
  represent the item being offered and one to represent the item being wanted.
* `ArcBuilder`: generates the arcs that correspond to each input wantlist.
* `node_util.h`: `Node` utility functions.
