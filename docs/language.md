# Language Specification

This documentation page the structure, syntax, and formal grammar of the Hera
language.

## Structural elements
A Hera source file consists of a sequence of top-level declarations. These
declarations shall define either an **Entity** or a **Relation**.

### Entities
Entities are the primary objects in an ER model. They shall be defined using the
`entity` keyword followed by a name and a block of members.

#### Definition
```hera
entity <name> [modifiers] {
    [members]
}
```
The `<name>` shall be either an identifier or a double-quoted string.

#### Modifiers
Entities may be characterized by the following optional modifiers:
- `specifies <parent>`: Indicates that the entity is a specialization of the
    parent entity
- `total`: Specifies that the specialization is total (every instance of the
    parent must belong to at least one subtype)
- `exclusive`: Specifies that the specialization is exclusive (an instance of the
    parent can belong to at most one subtype)
- `alias <identifier>`: Defines a short name for the entity, typically used for
    internal referencing and not shown in the final diagram

#### Members
The body of an entity declaration may contain a sequence of members, each
terminated by a semicolon (`;`). Members may be:
- **Attributes**: Defined using the `attribute` keyword
- **Relation References**: Defined using the `relation` keyword

### Relations
Relations describe how entities interact. They may be defined as top-level
objects to include attributes specific to the relationship itself.

#### Definition
```hera
relation <name> [alias <identifier>] {
    [attributes]
}
```

## Attributes
Attributes define the properties of entities or relations.

### Definition
```hera
attribute <name> [key | optional];
```
The `<name>` shall be an identifier or a double-quoted string.

### Modifiers
- `key`: Marks the attribute as part of the object's primary identifier
- `optional`: Indicates that the attribute may be null or omitted. This modifier
    is mutually exclusive with `key`

## Relation References
Within an entity block, a relation reference specifies the entity's participation
in a relationship.

### Definition
```hera
relation <name> (<min>, <max>) [key];
```
- `<name>`: The name of the relation being referenced
- `(<min>, <max>)`: The participation cardinality. `<min>` and `<max>` may be
    numeric literals, character sequences, or strings (e.g., `(0, N)`).

> [!NOTE]
> In academic ER modeling, cardinalities specify the minimum and maximum
> number of relationship instances in which an entity may participate.
> Hera preserves this semantics in its internal representation.

- `key`: Indicates that the entity's participation in this relation is part of
    its primary identifier (weak entity support)

## Requirements
To ensure the validity and consistency of the ER model, the following semantic
requirements shall be met:

### Unique Identifiers
- All top-level entity and relation names shall be unique within the source file
- Aliases, if provided, shall not conflict with any other entity or relation
    name or alias
- Attributes within the same entity or relation shall have unique names

### Referential Integrity
- Any entity referenced in a `specifies` clause shall be defined within the
    same source file.
- Any relation referenced within an entity block shall be defined as a top-level
    `relation`

### Model consistency
- Specialization hierarchies shall be acyclic; an entity shall not, directly or
    indirectly, specialize itself

## Formal grammar
The following EBNF-like notation describes the formal grammar of the Hera
language.

```ebnf
source_file      = { entity_decl | relation_decl } ;

entity_decl      = "entity" , name , { entity_modifier } , block ;
entity_modifier  = specifies_clause | "total" | "exclusive" | alias_clause ;
specifies_clause = "specifies" , name ;
alias_clause     = "alias" , identifier ;

relation_decl    = "relation" , name , [ alias_clause ] , block ;

block            = "{" , { block_item } , "}" ;
block_item       = ( attribute_def | relation_ref ) , ";" ;

attribute_def    = "attribute" , name , [ "key" | "optional" ] ;
relation_ref     = "relation" , name , cardinality , [ "key" ] ;

cardinality      = "(" , card_val , "," , card_val , ")" ;
card_val         = number | identifier | string ;

name             = identifier | string ;
identifier       = [a-zA-Z_] , { [a-zA-Z0-9_] } ;
string           = '"' , { any_character_except_quote_or_newline } , '"' ;
number           = digit , { digit } ;
```

## Lexical requirements
- **Comments**: Single-line comments start with `//`. Multi-line comments are
    enclosed between `/*` and `*/`
- **Case Sensitivity**: Keywords and identifiers are case-sensitive
- **Whitespace**: Whitespace is used as a separator but is otherwise ignored,
    except within quoted strings
