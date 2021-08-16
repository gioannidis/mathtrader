# Technical Notes

This document covers a range of technical notes and decisions regarding the
development of `mathtrader`.

## Style Guides

`mathtrader` adheres to specific style guides in order to enforce consistency
and improve readability. All pull requests should therefore follow the same
style guides.

### C++ Style Guide

`mathtrader` follows the external [Google C++ Style Guide][cppstyle ref] as of
[this commit][head ref].

#### Mutable References

The style guide would prohibit the use of mutable references for non-optional
output and input/output function parameters, suggesting the use of non-const
pointers, instead. This restriction was lifted on [2020-05-20][mutable ref].
Nevertheless, the respective [`cpplint`][cpplint ref] tool has not been updated
to reflect this change, raising a warning on non-mutable references. These
warnings are suppressed in the code via the `NOLINT(runtime/references)`
directive.

### Markdown Style Guide

The [`mdformat` style guide][mdstyle ref] is followed on all Markdown documents,
with the additional requirement of imposing a maximum character limit of 80
characters per line. Long URLs are exempted from this requirement. It is
recommended to define link references for long URLs to enhance readability.

### Tools

The following tools are used to enforce the style guides:

- [`clang-format`][clangformat ref]
- [`clang-tidy`][clangtidy ref]
- [`cpplint`][cpplint ref]
- [`mdformat`][mdformat ref]

[clangformat ref]: https://clang.llvm.org/docs/ClangFormat.html
[clangtidy ref]: https://clang.llvm.org/extra/clang-tidy/
[cpplint ref]: https://pypi.org/project/cpplint/
[cppstyle ref]: https://google.github.io/styleguide/cppguide.html
[head ref]: https://github.com/google/styleguide/commit/b1956b20a2f218a5ea4dcd8a14227e027deb93bd
[mdformat ref]: https://pypi.org/project/mdformat/
[mdstyle ref]: https://mdformat.readthedocs.io/en/stable/users/style.html
[mutable ref]: https://github.com/google/styleguide/commit/7a7a2f510efe7d7fc5ea8fbed549ddb31fac8f3e
