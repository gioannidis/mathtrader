# Internal `NetworkBuilder`

Implements the internal modules of `NetworkBuilder`, which generates a
`FlowNetwork` out of a `ParserResult`:

* `NodeBuilder`: generates two nodes for each offered item: one `Node` to
  represent the item being offered and one to represent the item being wanted.
  * Also generates a source and a sink for the `FlowNetwork`.
* `ArcBuilder`: generates the arcs that correspond to each input wantlist.
  * Also connects the source `Node` to each "offered" `Node` and each "wanted"
  `Node` to the sink `Node`.
* `node_util.h`: `Node` utility functions.
