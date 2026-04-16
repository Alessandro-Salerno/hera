# Architecture

This documentation page focuses on Hera's internal architecture with the aim of
reducing ambiguity and confusion for potential contributors. 

## Pipeline overview
Currently, Hera employs a five-stage pipeline to turn input files into diagrams:
1. **Lexical analysis:**  This pass is performed by the [lexer](../src/lexer.c). 
    It turns the raw text input into a list of _tokens_ to facilitate subsequent
    passes. As part of tokenization, it also ensures that the input is not
    malformed, and guarantees that each returned token's contents are valid
    and comply with the definition of its _token type_. Token types include,
    for example, identifiers, strings, numbers, and keywords
2. **Parsing:** This pass is performed by the [parser](../src/parser.c). It turns
    the lexer's token list into an _Abstract Syntax Tree_ representing the
    initial input's meaning in an abstract form. Each node in the AST is of one
    of a finite set of possible types, including entities, relationships, attributes,
    and references. As part of this process, the parser also validates syntax
    rules
3. **Graph resolution:** This pass is performed by the [graph resolver](../src/graph.c).
    It turns the parser's AST into an abstract, in-memory representation of the diagram
    as a graph of entities and relationships. As part of this process, it also validates
    references and names
4. **Layout generation:** This pass is performed by the [layout engine](../src/layout.c).
    It changes the graph in-place, giving each node concrete physical characteristics,
    such as position and size
5. **SVG emission:** This pass is performed by the [SVG emitter](../src/svg.c).
    It walks the graph and outputs the SVG representation to Standard Output

### Component interaction
As is implicit in the above bullet list, each component resides in its own file
and acts as a simple transformation of its inputs.

Normally, inputs to a given component are passed by the program's entrypoint
(currently either [`main.c`](../src/main.c), or [`libfuzzer.c`](../fuzz/libfuzzer.c))
and consist of the previous component's output. 

Components generally return an `ER_RESULT(t)` type, whose behavior is similar to
that of [Rust's Result type](https://doc.rust-lang.org/std/result/). Components
are expected to return this object as is, so as to allow the entrypoint to choose
error handling policy (`ER_RESULT_UNWRAP` or manual handling).

Following the transformation model described above, components are expected not to
interact directly and shall solely operate on their inputs. It is thus not necessary
to fully understand all pipeline components to work on one.

## Stability and ownership
Not all components are equally stable. Contributors should use the following
as a general guideline:

- **Stable components (avoid breaking changes):**
  - Lexer
  - Parser
  - Core language semantics

- **Moderately stable (changes welcome, but require care):**
  - Graph resolver
  - Memory allocator
  - Internal data structures

- **Flexible / open for redesign:**
  - Layout engine
  - SVG emitter
  - CLI behavior

## Components
This section expands on the internal architecture of each component.

### Lexer
The lexer is designed as a [Finite State Machine](https://en.wikipedia.org/wiki/Finite-state_machine).
States are represented by functions of the following type:
```c
LexerInstruction LexerCharHandler(ER_WChar c);
```
The lexer's main loop can be thought of as a sort of virtual machine implementing
a very restricted instruction set:
```c
typedef enum LexerAction {
    LEXER_ACTION_DISCARD,
    LEXER_ACTION_IGNORE,
    LEXER_ACTION_PUSH
} LexerAction;

typedef struct LexerInstruction {
    LexerCharHandler *li_handler;
    LexerAction       li_charaction;
    LexerAction       li_tokaction;
    ER_TokenType      li_toktype;
} LexerInstruction;
```
The main loop steps through each character in the input string, decodes it into
an `ER_WChar` (thus simplifying UTF-8 handling in state functions), and calls
the current state function. Said handler will return a `LexerInstruction` struct
as per its signature, instructing the main loop on:
- **What state to transition to:** The `li_handler` field of `LexerInstruction`
    tells the main loop what state is next. A state function may use this to
    switch to another state (by passing another function pointer), continue
    in the same state (by passing itself), or panic (by passing `NULL`)
- **What to do with the current character:** The `li_charaction` field instructs
    the main loop on what action to take with regards to the current character:
    - **Discard:** If the value is `LEXER_ACTION_DISCARD`, the character is 
        ignored and the lexer advances to the next character
    - **Ignore:** If the value is `LEXER_ACTION_IGNORE`, the lexer ignores
        the current iteration and retries. This is used in combination with
        `li_handler` to defer handling of a character to another state function
    - **Push:** If the value is `LEXER_ACTION_PUSH`, the lexer advances normally
- **What to do with the current token:** The lexer operates on one token at a time.
    The `li_tokaction` field, similarly to `li_charaction`, instructs the lexer
    on what action to take with regards to said token:
    - **Discard:** If the value is `LEXER_ACTION_DISCARD`, the current token is
        reset without adding it to the token list
    - **Ignore:** If the value is `LEXER_ACTION_IGNORE`, no action is taken on
        the current token. This is used to accumulate characters into a token
    - **Push:** If the value is `LEXER_ACTION_PUSH`, the current token is added
        to the token list and its contents are reset to prepare for the next token.
        In this case, the `li_toktype` field acts as a hint of the final token
        type. Note that if `li_toktype == ER_TOKEN_TYPE_NONE`, more checks are
        performed by the `lexer_token_type` function to determine the final
        token type

### Parser
The parser is implemented using simple [recursive descent](https://en.wikipedia.org/wiki/Recursive_descent_parser)
given the relative simplicity of Hera's syntax.

All AST nodes are instances of subtypes of `ER_ASTNode`:
```c
typedef enum ER_ASTNodeType {
    ER_AST_NODE_TYPE_NONE      = 0,
    ER_AST_NODE_TYPE_ROOT      = 1 << 0,
    ER_AST_NODE_TYPE_ENTITY    = 1 << 1,
    ER_AST_NODE_TYPE_REFERENCE = 1 << 2,
    ER_AST_NODE_TYPE_RELATION  = 1 << 3,
    ER_AST_NODE_TYPE_ATTRIBUTE = 1 << 4,
} ER_ASTNodeType;

typedef struct ER_ASTNode {
    ER_ASTNodeType an_type;
    TAILQ_ENTRY(ER_ASTNode) an_link;
} ER_ASTNode;
```
This structure simplifies handling of tree nodes, while also maintaining correctness
and high performance. The value of `an_type` can be used to determine the exact
AST node struct subtype, and the full structure can be derived by performing a
simple pointer cast, as shown here:
```c
ER_ASTNode *n = // ...
if (n->an_type == ER_AST_NODE_TYPE_ENTITY) {
    ER_ASTEntityNode *en = (void *)n;
    // ...
}
```

### Graph resolver
The graph resolver employs an internal four-stage pipeline:
1. **Entity resolution:** The resolver walks  the list of entities in the
    AST's root node, instantiates a graph equivalent (`ER_GraphEntity`), and adds
    it to the entity hash table. If the entity was declared with an alias, it is also
    added to the entity alias hash table
2. **Relation resolution:** The resolver walks the list of relations in the AST's
    root node, instantiates a graph equivalent (`ER_GraphRelation`), and adds it
    to the relation hash table. If the relation was declared with an alias, it is also
    added to the relation alias hash table 
3. **Reference resolution:** The resolver walks the newly-created entity hash table.
    For each entity, it performs two sub-steps:

    3.1. **Relation reference resolution:** For each of the entity's relation references,
        the resolver instantiates an edge representation (`ER_GraphEdge`) linking the
        entity and the relationship directly

    3.2. **Parent reference resolution:** If the entity specializes another entity,
        the resolver establishes a direct pointer-based link between the two

5. **Acyclicity enforcement:** The resolver walks the entity hash table and checks
    the specialization hierarchy of each entity, ensuring it does not create a cycle

### Layout engine
The layout engine's architecture is obscure and currently doesn't match domain
requirements. A rewrite is needed, as noted [here](https://github.com/Alessandro-Salerno/hera/issues/1).

### SVG emitter
The SVG emitter works by walking the input graph and printing SVG representations
for each node to Standard Output. This component also needs a rewrite, as noted
[in the same issue](https://github.com/Alessandro-Salerno/hera/issues/1).

### Memory allocator
Hera uses a custom dynamic [arena memory allocator](https://en.wikipedia.org/wiki/Region-based_memory_management)
to help manage allocation lifetimes. 

The allocator provides two main interfaces:
- **Local interface (recommended):** Uses a [Zig-like allocator pattern](https://zig.guide/standard-library/allocators/).
    Each pipeline component with dynamic memory needs takes an allocator as an
    additional parameter. All regular allocations performed by said pipeline
    component shall thus use the provided allocator instance
- **Global interface (for compatibility):** Some dependencies rely on the C Standard
    Library's `malloc`/`free` abstractions more-or-less directly. The global
    interface allows lifetime tracking of allocations performed by these components
    with minimal changes to their code. The global interface is also useful
    to implement `realloc`-heavy structures, such as dynamic arrays, due to the
    lack of a proper local `realloc` interface. The global interface is designed
    to act externally identical to standard interfaces, thus some optimizations
    are not present

> [!IMPORTANT]
> Hera treats memory allocation as an axiom. Calls to any allocation interface
> never return invalid memory addresses. The allocator transparently terminates
> the process in case of allocation failure. 
> 
> This, while seemingly unintuitive, significantly simplifies error handling in
> allocation paths, and has no significant downsides for this specific domain,
> especially considering modern operating systems often resort to wait states or
> OOM killers in case of heavy memory pressure.

#### Allocation coalescing
When allocating through the local interface, the allocator coalesces small requests
into _pools_ (also known as arenas or regions). An allocator instance can hold
a theoretically unlimited number of pools. When the current pool's space runs out,
a new one is allocated with double the size. 

This process ensures amortized fast allocations for most requests.

#### Allocator tracking and teardown
The custom memory allocator also keeps track of all allocator instances (including
the hidden global instance), allowing for easy lifetime management. It is in fact possible
to free all allocations in a given allocator instance (`ER_allocator_deinit`), or
in _all_ allocator instances (`ER_memory_deinit`). 

This distinction maps easily on both contexts where direct teardown is not strictly
necessary (e.g., when using Hera as a CLI compiler) and others where it is (e.g.,
when running Hera as an LSP server). 
