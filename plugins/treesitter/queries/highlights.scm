; Keywords
"entity" @keyword
"relation" @keyword
"attribute" @keyword
"total" @keyword
"exclusive" @keyword
"specifies" @keyword
"key" @keyword
"alias" @keyword

; Names
(entity_declaration name: (identifier) @type)
(entity_declaration name: (string) @type)
(relation_declaration name: (identifier) @function)
(relation_declaration name: (string) @function)

(specifies_clause parent: (identifier) @type)
(specifies_clause parent: (string) @type)

(alias_clause alias: (identifier) @type)

(attribute_definition name: (identifier) @variable)
(attribute_definition name: (string) @variable)

(relation_reference name: (identifier) @function)
(relation_reference name: (string) @function)

; Constants and Literals
(number) @number
((identifier) @constant
 (#eq? @constant "N"))
(string) @string

; Comments
(comment) @comment

; Punctuation
"{" @punctuation.bracket
"}" @punctuation.bracket
"(" @punctuation.bracket
")" @punctuation.bracket
";" @punctuation.delimiter
"," @punctuation.delimiter
