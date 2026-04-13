# Conventions

This documentation page defines the standards and conventions used in this
project.

## C source and header conventions
This section highlights standards and conventions specific to C source and header files.

### Naming
- All types, whether exposed or internal, shall use `PascalCase`
- All function names shall use `snake_case`
- All local variables and function parameters shall use `snake_case`
- All global variables shall use `snake_case` and shall be prefixed with `g_`
- All static global variables shall use `snake_case` and shall be prefixed with `s_`
- All static function names inside source files shall use `snake_case` and
    shall be prefixed with  the name of the source file
- All macros shall use `UPPERCASE_SNAKE_CASE`
- All source files shall have names that reflect that of the corresponding header
    file, if present
- File and directory names should preferably be a single word, and shall use `_`
    as separator in case they are not
- All struct or union fields shall use `snake_case` and shall be prefixed with
    a 1-to-3- character-long sequence uniquely identifying the element they are
    a field of
- All enumerator values shall use `UPPERCASE_SNAKE_CASE` and their names shall
    reflect that of the enumerator type
- All language elements exposed by headers under [`include/hera`](../include/hera)
    shall be prefixed with `ER_`. This prefix applies with maximum priority
- All files shall display a copy of the [`LICENSE`](../LICENSE) file at the top
- All header files shall use `#pragma once`

An exemption is made to these provisions for files originating from third-parties
and files whose contents are imposed by other standards (as is the case, for example,
for [`fuzz/libfuzzer.c`](../fuzz/libfuzzer.c)).

### Formatting
Formatting rules are specified by the [`.clang-format`](../.clang-format) file
and automatically applied by the relevant tools.

These formatting rules shall not apply to files originating from third-parties.

### Style
- Equality and inequality comparisons against constants shall use Yoda notation
- Constructs, including but not limited to branches and loops, shall not omit
    curly braces
- Usage of Hera-specific types and mechanisms, such as those defined in [`include/hera/types.h`](../include/hera/types.h),
    and [`include/hera/result.h`](../include/hera/result.h), shall be preferred
    over ambiguous C-standard types and rudimentary mechanisms
- Usage of `#include "..."` is prohibited. Header files shall be placed in the
    [`include`](../include) directory and include statements shall only reference
    files in that directory or in system include directories

An exemption is made to these provisions for files originating from third-parties.

### Comments
- Comments starting with `// TODO: ` shall be employed to mark code blocks
    scheduled for future completion
- Comments starting with `// FIXME: ` shall be employed to mark code blocks
    whose behavior is expected or thought to be the cause of current or
    future issues
- Comments starting with `// NOTE: ` shall be employed to state non-obvious
    characteristics of the file or code block

An exemption is made to these provisions for files originating from third-parties.

### Justification
- **Yoda notation:** while uncommon in modern code bases, it aids readability
    in the presence of repeated comparisons of the same variable against
    different constants, especially when using large font sizes or screen
    magnifiers
- **Field prefixes:**  while also uncommon, this is expected to help distinguish
    container types when type information is not immediately visibile, as is the
    case, for example, in macros, direct dereferences of function return values,
    and global variables
