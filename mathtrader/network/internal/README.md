# Internal `NetworkBuilder`

Implements `ArcBuilder`, the internal module of `NetworkBuilder`, which
generates the arcs that correspond to each input wantlist. Each `Arc` represents
a valid matching between an offered item and a different wanted item.

`ArcBuilder` pre-processes the model and prunes items that are certain not to
trade.
