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

#include <assert.h>
#include <ergen/graph.h>
#include <stdlib.h>

typedef ER_RESULT(ER_GraphEntity *) GraphEntityResult;
typedef ER_RESULT(ER_GraphRelation *) GraphRelationResult;

// NOTE: this must be here because we want to use the default HASH_FUNCTION
// definition
static inline void graph_hash_string(ER_String *s, unsigned int *hashv) {
    unsigned int r;
    HASH_FUNCTION(s->str_buf, s->str_len, r);
    *hashv = r;
}

#undef HASH_FUNCTION
#define HASH_FUNCTION(keyptr, keylen, hashv) graph_hash_string(keyptr, &hashv)

// NOTE: we negate the value of ER_STRING_EQ because uthash sues the POSIX
// convention (i.e., 0 = success, all other values are errors)
#undef HASH_KEYCMP
#define HASH_KEYCMP(a, b, n) !ER_STRING_EQ(*(ER_String *)a, *(ER_String *)b)

static GraphEntityResult graph_get_entity(ER_Graph *graph, ER_String name) {
    ER_GraphEntity *ent = NULL;
    HASH_FIND(gen_hh, graph->gr_entities, &name, sizeof(name), ent);

    if (NULL != ent) {
        return (GraphEntityResult){.res_status = ER_STATUS_OK, .res_val = ent};
    }

    return (GraphEntityResult){.res_status = ER_STATUS_ERR,
                               .res_err    = "unknown entity"};
}

static GraphRelationResult graph_get_relation(ER_Graph *graph, ER_String name) {
    ER_GraphRelation *rel = NULL;
    HASH_FIND(gre_hh, graph->gr_relations, &name, sizeof(name), rel);

    if (NULL != rel) {
        return (GraphRelationResult){.res_status = ER_STATUS_OK,
                                     .res_val    = rel};
    }

    return (GraphRelationResult){.res_status = ER_STATUS_ERR,
                                 .res_err    = "unknown relation"};
}

static GraphEntityResult graph_add_entity(ER_Graph         *graph,
                                          ER_ASTEntityNode *entity) {
    GraphEntityResult get_res = graph_get_entity(graph, entity->ent_name);
    if (ER_RESULT_OK(get_res)) {
        return (GraphEntityResult){.res_status = ER_STATUS_ERR,
                                   .res_err    = "entity redefined"};
    }

    ER_GraphEntity *graph_entity = calloc(1, sizeof(*graph_entity));
    assert(NULL != graph_entity);
    graph_entity->gen_astnode = entity;

    HASH_ADD(gen_hh,
             graph->gr_entities,
             gen_astnode->ent_name,
             sizeof(ER_String),
             graph_entity);
    return (GraphEntityResult){.res_status = ER_STATUS_OK,
                               .res_val    = graph_entity};
}

static GraphRelationResult graph_add_relation(ER_Graph           *graph,
                                              ER_ASTRelationNode *relation) {
    GraphRelationResult get_res = graph_get_relation(graph, relation->rel_name);
    if (ER_RESULT_OK(get_res)) {
        return (GraphRelationResult){.res_status = ER_STATUS_ERR,
                                     .res_err    = "relation redefined"};
    }

    ER_GraphRelation *graph_relation = calloc(1, sizeof(*graph_relation));
    assert(NULL != graph_relation);
    graph_relation->gre_astnode = relation;
    TAILQ_INIT(&graph_relation->gre_edges);

    HASH_ADD(gre_hh,
             graph->gr_relations,
             gre_astnode->rel_name,
             sizeof(ER_String),
             graph_relation);
    return (GraphRelationResult){.res_status = ER_STATUS_OK,
                                 .res_val    = graph_relation};
}

static inline ER_GraphResult graph_err(const char *msg) {
    return (ER_GraphResult){.res_status = ER_STATUS_ERR, .res_err = msg};
}

static inline ER_GraphResult graph_ok(ER_Graph graph) {
    return (ER_GraphResult){.res_status = ER_STATUS_OK, .res_val = graph};
}

ER_GraphResult er_graph_compute(ER_ASTRootNode *ast_root) {
    // NOTE: zeroeing this structure causes pointer fields to become NULL, which
    // constitutes implicit initialization as per uthash documentation
    ER_Graph graph = {0};

    // add entities to the entity hashmap
    ER_ASTNode *ast_ent;
    TAILQ_FOREACH(ast_ent, &ast_root->rt_entities, an_link) {
        assert(ER_AST_NODE_TYPE_ENTITY == ast_ent->an_type);
        ER_ASTEntityNode *ent = (void *)ast_ent;

        GraphEntityResult add_res = graph_add_entity(&graph, ent);
        if (!ER_RESULT_OK(add_res)) {
            return graph_err(ER_RESULT_ERROR(add_res));
        }
    }

    // add relations to the relation hashmap
    ER_ASTNode *ast_rel;
    TAILQ_FOREACH(ast_rel, &ast_root->rt_relations, an_link) {
        assert(ER_AST_NODE_TYPE_RELATION == ast_rel->an_type);
        ER_ASTRelationNode *rel = (void *)ast_rel;

        GraphRelationResult add_res = graph_add_relation(&graph, rel);
        if (!ER_RESULT_OK(add_res)) {
            return graph_err(ER_RESULT_ERROR(add_res));
        }
    }

    // link entities to their parent
    ER_GraphEntity *graph_ent, *_;
    HASH_ITER(gen_hh, graph.gr_entities, graph_ent, _) {
        if (!(graph_ent->gen_astnode->ent_flags & ER_ENTITY_FLAGS_SPECIFIES)) {
            continue;
        }

        GraphEntityResult get_res = graph_get_entity(
            &graph,
            graph_ent->gen_astnode->ent_specifies);
        if (!ER_RESULT_OK(get_res)) {
            return graph_err(ER_RESULT_ERROR(get_res));
        }

        ER_GraphEntity *specifies = ER_RESULT_GET(get_res);
        graph_ent->gen_specifies  = specifies;
    }

    // TODO: link relations

    return graph_ok(graph);
}
