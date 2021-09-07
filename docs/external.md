# External Repositories

`mathtrader` integrates the following external libraries via `bazel`.

## Dependency Repositories

These repositories are required to build `mathtrader`:

- [Abseil][abseil ref] common libraries.
- [Google Logging][glog ref] library.
- [GoogleTest][gtest ref] testing and mocking framework.
- [OR-Tools][ortools ref] optimization
- [RE2][re2 ref] regular expression library.

## Development Tools

These repositories improve development experience and are not strictly necessary
to compile `mathtrader`:

- [bazel_clang_tidy][clang-tidy ref]: optionally executes `clang-tidy` directly
  on C++ targets. The applied checks are specified in the
  [.clang-tidy][clang-tidy config ref] configuration file. Example usage:

```
bazel build //... --config=clang-tidy
```

[abseil ref]: https://abseil.io
[clang-tidy config ref]: .clang-tidy
[clang-tidy ref]: https://github.com/erenon/bazel_clang_tidy
[glog ref]: https://github.com/google/glog
[gtest ref]: http://google.github.io/googletest
[ortools ref]: https://developers.google.com/optimization
[re2 ref]: https://github.com/google/re2
