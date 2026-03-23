/*
BSD 2-Clause License

Copyright (c) 2026, Alessandro Salerno

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <bsd/queue.h>
#include <ergen/lexer.h>
#include <ergen/types.h>

#define ER_ENTITY_FLAGS_TOTAL     1
#define ER_ENTITY_FLAGS_SPECIFIES 2

#define ER_ATTRIBUTE_FLAGS_KEY 1

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

typedef struct ER_ASTRootNode {
    ER_ASTNode rt_node;
    TAILQ_HEAD(, ER_ASTNode) rt_entities;
    TAILQ_HEAD(, ER_ASTNode) rt_relations;
} ER_ASTRootNode;

typedef struct ER_ASTEntityNode {
    ER_ASTNode ent_node;
    ER_String  ent_name;
    ER_String  ent_specifies;
    ER_u64     ent_flags;
    TAILQ_HEAD(, ER_ASTNode) ent_attributes;
} ER_ASTEntityNode;

typedef struct ER_ASTReferenceNode {
    ER_ASTNode ref_node;
    ER_String  ref_entname;
    ER_Token   ref_lcard;
    ER_Token   ref_rcard;
} ER_ASTReferenceNode;

typedef struct ER_ASTRelationNode {
    ER_ASTNode rel_node;
    ER_String  rel_name;
    TAILQ_HEAD(, ER_ASTNode) rel_entities;   // of type ER_ASTReferenceNode
    TAILQ_HEAD(, ER_ASTNode) rel_attributes; // of type ER_ASTAttributeNode
} ER_ASTRelationNode;

typedef struct ER_ASTAttributeNode {
    ER_ASTNode atr_node;
    ER_String  atr_name;
    ER_u64     atr_flags;
} ER_ASTAttributeNode;

typedef ER_RESULT(ER_ASTRootNode) ER_ParserResult;

ER_ParserResult er_parser_run(ER_TokenList tokens);
