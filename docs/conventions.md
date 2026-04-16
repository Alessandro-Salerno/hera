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
- Comments starting with `// EDIT: ` shall be employed to state modifications
    to third-party code

An exemption is made to these provisions for files originating from third-parties.

### Justification
- **Field prefixes:**  while also uncommon, this is expected to help distinguish
    container types when type information is not immediately visibile, as is the
    case, for example, in macros, direct dereferences of function return values,
    and global variables


## Diagram conventions
This section highlights standards and conventions ideally used in the generated
Entity-Relationship diagram. 

> [!NOTE]
> This section is not representative of the current state of generated diagrams

- Entities shall be represented as empty rectangles whose content shall be limited
    to the entity name itself, which shall be placed in the middle of said rectangle
- Relationships shall be represented as diamonds whose content shall be limited
    to the relationship name itself, which shall be placed in the middle of said
    diamond
- Connections between entities and relationships shall be represented by simple,
    non-oriented lines. Said lines shall only curve at 90-degree angles. If the
    reference is marked with a `key` modifier, the connection shall be treated
    as part of the entity's identifier (as described below)
- Cardinalities for a given connection shall be placed either above or below the
    line (depending on space availability) and near the entity to which they're
    attached. Cardinalities shall follow the format: `(min, max)` as described
    in the input file
- Attributes shall be represented as small circles connected the object
    (entity or relationship) they're declared in. If the attribute is marked
    with a `key` modifier, it shall be treated as part of the object's identifier
    (as described below). If not, the above-mentioned circle shall have a black
    outline and be filled in white
- Cardinalities for a given attribute shall be placed in the proximity of the line
    connecting the attribute to its parent object. Cardinalities shall follow the
    format: `(min, max)`. By default, `(min = 1, max = N)`. If the attribute is
    marked with the `optional` modifier, `min` shall be lowered to `0`. 
- If an object component (either an attribute or a relationship reference) is
    part of the object's identifier, it shall appear with a black circle. Otherwise,
    the behavior is specified above in the relevant bullets
- Object identifiers may be composite. In this case, a curve shall be traced
    between all circles part of the identifier
- Entities which specialize others shall be connected to their parent by a pointed
    arrow. If the parent's definition is marked with the `total` modifier, the tip
    of said arrow shall contain a `T`. Similarly, if the parent's definition is
    marked with the `exclusive` modifier, the arrow tip shall also contain an `E`.
    Entities that specialize the same parent should preferably merge their arrows
    and be visually adjacent, possibly forming a specialization tree
