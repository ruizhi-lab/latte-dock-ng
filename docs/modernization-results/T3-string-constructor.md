# T3 string-constructor fixture probe

## Result

A real standard-library type gives `bugprone-string-constructor` a portable
negative fixture under the unchanged production flags:

```cpp
#include <string_view>

void stringConstructorFixture()
{
    std::string_view empty("test", 0);
    (void)empty;
}
```

The check reports `constructor creating an empty string` at the constructor
call. The explicit use prevents the production warning flags from adding an
unused-variable diagnostic. This is the documented empty-string pattern and
uses the check's default `::std::basic_string_view` target; it requires no
custom class, check option, exception flag, or altered compiler arguments.

## Verification

The probe used the `app/lattecorona.cpp` entry from
`build/modernization/clang-debug/compile_commands.json`, retargeted with the
existing `CLAZY_GATE.retarget` helper to `/tmp/latte-t3-string/probes.cpp`.
The only clang-tidy configuration override was to enable this one check and
promote its finding to an error. The production compile flags were unchanged.

Both commands emitted exactly one `bugprone-string-constructor` diagnostic on
the constructor, with no compiler diagnostics:

```sh
clang-tidy --config-file=/tmp/latte-t3-string/tidy.yaml \
  -p=/tmp/latte-t3-string /tmp/latte-t3-string/probes.cpp --quiet
PATH=/tmp/latte-llvm18/bin:$PATH clang-tidy \
  --config-file=/tmp/latte-t3-string/tidy.yaml \
  -p=/tmp/latte-t3-string /tmp/latte-t3-string/probes.cpp --quiet
```

The tools were host clang-tidy 22.1.8 and the isolated clang-tidy 18.1.3
wrapper. The production database supplies the host GCC 16 C++ headers; the
wrapper setup substitutes the extracted GCC 13 headers as described in
`T1-data/README.md`.

## Limits and disposition

The documented `std::string("test", 200)` and `std::string('x', 50)` probes
remain unsuitable: neither triggered on host clang-tidy 22.1.8. Additional
literal-bound cases showed a toolchain difference: clang-tidy 18 reported
`std::string("test", -1, 10)` and `std::string("test", 10, 10)`, while 22
reported neither for `std::string`; both versions did report the corresponding
empty-length and oversized-length cases for `std::string_view`. This probe does
not establish why the `std::string` matcher differs across the two standard
library ASTs, so it should not be described as a matcher limitation or fixed by
changing production flags.

Disposition: use the `std::string_view` empty-length fixture if the check is
promoted. The fixture proves the check runs against a production standard
library type on both supported tidy versions. The remaining promotion decision
still requires the normal production-source gate review; this probe alone does
not establish that enabling the check is clean across production sources.

The behavior descriptions and default `StringNames` list are documented in
the [official clang-tidy check documentation](https://clang.llvm.org/extra/clang-tidy/checks/bugprone/string-constructor.html).
