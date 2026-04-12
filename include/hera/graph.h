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
#include <hera/parser.h>
#include <hera/result.h>
#include <hera/types.h>
#include <ezld/array.h>
#include <uthash/uthash.h>

typedef struct ER_GraphEntity   ER_GraphEntity;
typedef struct ER_GraphRelation ER_GraphRelation;

struct ER_GraphEntity {
    TAILQ_ENTRY(ER_GraphEntity) gen_link;
    TAILQ_HEAD(, ER_GraphEntity) gen_specifiers;
    ER_ASTEntityNode *gen_astnode;
    ER_GraphEntity   *gen_specifies;
    ER_u64            gen_numspecifiers;
    ER_i32            gen_x;
    ER_i32            gen_y;
    ER_i32            gen_w;
    ER_i32            gen_h;
    ER_i32            gen_layer;
    ER_i32            gen_col;
    UT_hash_handle    gen_hh;
    UT_hash_handle    gen_hhalias;
};

typedef struct ER_GraphEdge {
    TAILQ_ENTRY(ER_GraphEdge) ged_link;
    ER_ASTReferenceNode *ged_astnode;
    ER_GraphEntity      *ged_entity;
    ER_GraphRelation    *ged_relation;
    ER_i32               ged_x1;
    ER_i32               ged_y1;
    ER_i32               ged_x2;
    ER_i32               ged_y2;
} ER_GraphEdge;

struct ER_GraphRelation {
    TAILQ_HEAD(, ER_GraphEdge) gre_edges;
    ER_ASTRelationNode *gre_astnode;
    ER_i32              gre_x;
    ER_i32              gre_y;
    ER_i32              gre_w;
    ER_i32              gre_h;
    UT_hash_handle      gre_hh;
    UT_hash_handle      gre_hhalias;
};

typedef struct ER_Graph {
    ER_GraphEntity   *gr_entities;
    ER_GraphRelation *gr_relations;
    ER_GraphEntity   *gr_entaliases;
    ER_GraphRelation *gr_relaliases;
} ER_Graph;

typedef ER_RESULT(ER_Graph) ER_GraphResult;

ER_GraphResult ER_graph_compute(ER_ASTRootNode *ast_root);
void           ER_graph_place(ER_Graph *graph);
