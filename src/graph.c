/*
BSD 2-Clause License

Copyright (c) 2026, Alessandro Salerno and contributors

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
#include <hera/graph.h>
#include <hera/svg.h>
#include <hera/util.h>
#include <stdbool.h>
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

static void graph_unknown_panic_handler(void *arg) {
    ER_Token *tok   = arg;
    ER_String input = ER_STRING_SUP(tok->tok_value, tok->tok_off);

    ER_generic_panic_output(input,
                            tok->tok_row,
                            tok->tok_col,
                            tok->tok_rowstart,
                            tok->tok_col - 1,
                            tok->tok_value.str_len,
                            "unknown name '%.*s'",
                            ER_STRING_PRINTF(tok->tok_value));
}

static void graph_redefined_panic_handler(void *arg) {
    ER_Token *tok   = arg;
    ER_String input = ER_STRING_SUP(tok->tok_value, tok->tok_off);

    ER_generic_panic_output(input,
                            tok->tok_row,
                            tok->tok_col,
                            tok->tok_rowstart,
                            tok->tok_col - 1,
                            tok->tok_value.str_len,
                            "multiple definitions of '%.*s'",
                            ER_STRING_PRINTF(tok->tok_value));
}

static void graph_cycle_panic_handler(void *arg) {
    ER_GraphEntity *ent   = arg;
    ER_Token       *spec  = &ent->gen_astnode->ent_specifies;
    ER_String       input = ER_STRING_SUP(spec->tok_value, spec->tok_off);

    ER_generic_panic_output(
        input,
        spec->tok_row,
        spec->tok_col,
        spec->tok_rowstart,
        spec->tok_col - 1,
        spec->tok_value.str_len,
        "specialization of entity '%.*s' from '%.*s' creates a cycle",
        ER_STRING_PRINTF(ent->gen_astnode->ent_name.tok_value),
        ER_STRING_PRINTF(spec->tok_value));
}

static GraphEntityResult graph_get_entity(ER_Graph *graph, ER_Token *name) {
    ER_GraphEntity *ent = NULL;
    HASH_FIND(gen_hh,
              graph->gr_entities,
              &name->tok_value,
              sizeof(name->tok_value),
              ent);

    if (ent != NULL) {
        return (GraphEntityResult){.res_status = ER_STATUS_OK, .res_val = ent};
    }

    HASH_FIND(gen_hhalias,
              graph->gr_entaliases,
              &name->tok_value,
              sizeof(name->tok_value),
              ent);

    if (ent != NULL) {
        return (GraphEntityResult){.res_status = ER_STATUS_OK, .res_val = ent};
    }

    return (GraphEntityResult){.res_status       = ER_STATUS_PANIC,
                               .res_panicarg     = name,
                               .res_panichandler = graph_unknown_panic_handler};
}

static GraphRelationResult graph_get_relation(ER_Graph *graph, ER_Token *name) {
    ER_GraphRelation *rel = NULL;
    HASH_FIND(gre_hh,
              graph->gr_relations,
              &name->tok_value,
              sizeof(name->tok_value),
              rel);

    if (rel != NULL) {
        return (GraphRelationResult){.res_status = ER_STATUS_OK,
                                     .res_val    = rel};
    }

    HASH_FIND(gre_hhalias,
              graph->gr_relaliases,
              &name->tok_value,
              sizeof(name->tok_value),
              rel);

    if (rel != NULL) {
        return (GraphRelationResult){.res_status = ER_STATUS_OK,
                                     .res_val    = rel};
    }

    return (GraphRelationResult){
        .res_status       = ER_STATUS_PANIC,
        .res_panicarg     = name,
        .res_panichandler = graph_unknown_panic_handler};
}

static GraphEntityResult graph_add_entity(ER_Graph         *graph,
                                          ER_ASTEntityNode *entity) {
    GraphEntityResult get_res = graph_get_entity(graph, &entity->ent_name);
    if (ER_RESULT_OK(get_res)) {
        return (GraphEntityResult){
            .res_status       = ER_STATUS_PANIC,
            .res_panicarg     = &entity->ent_name,
            .res_panichandler = graph_redefined_panic_handler};
    }

    ER_GraphEntity *graph_entity = calloc(1, sizeof(*graph_entity));
    assert(graph_entity != NULL);
    graph_entity->gen_astnode = entity;
    TAILQ_INIT(&graph_entity->gen_specifiers);

    HASH_ADD(gen_hh,
             graph->gr_entities,
             gen_astnode->ent_name.tok_value,
             sizeof(ER_String),
             graph_entity);

    if (entity->ent_flags & ER_ENTITY_FLAGS_ALIAS) {
        HASH_ADD(gen_hhalias,
                 graph->gr_entaliases,
                 gen_astnode->ent_alias.tok_value,
                 sizeof(ER_String),
                 graph_entity);
    }

    return (GraphEntityResult){.res_status = ER_STATUS_OK,
                               .res_val    = graph_entity};
}

static GraphRelationResult graph_add_relation(ER_Graph           *graph,
                                              ER_ASTRelationNode *relation) {
    GraphRelationResult get_res = graph_get_relation(graph,
                                                     &relation->rel_name);
    if (ER_RESULT_OK(get_res)) {
        return (GraphRelationResult){
            .res_status       = ER_STATUS_PANIC,
            .res_panicarg     = &relation->rel_name,
            .res_panichandler = graph_redefined_panic_handler};
    }

    ER_GraphRelation *graph_relation = calloc(1, sizeof(*graph_relation));
    assert(graph_relation != NULL);
    graph_relation->gre_astnode = relation;
    TAILQ_INIT(&graph_relation->gre_edges);

    HASH_ADD(gre_hh,
             graph->gr_relations,
             gre_astnode->rel_name.tok_value,
             sizeof(ER_String),
             graph_relation);

    if (relation->rel_flags & ER_RELATION_FLAGS_ALIAS) {
        HASH_ADD(gre_hhalias,
                 graph->gr_relaliases,
                 gre_astnode->rel_alias.tok_value,
                 sizeof(ER_String),
                 graph_relation);
    }

    return (GraphRelationResult){.res_status = ER_STATUS_OK,
                                 .res_val    = graph_relation};
}

static inline ER_GraphResult graph_ok(ER_Graph graph) {
    return (ER_GraphResult){.res_status = ER_STATUS_OK, .res_val = graph};
}

static bool graph_has_cycle(ER_GraphEntity *entity) {
    ER_GraphEntity *slow = entity;
    ER_GraphEntity *fast = entity;

    while (fast != NULL && fast->gen_specifies != NULL) {
        slow = slow->gen_specifies;
        fast = fast->gen_specifies->gen_specifies;
        if (slow == fast) {
            return true;
        }
    }

    return false;
}

static ER_GraphResult graph_panic_cycle(ER_GraphEntity *entity) {
    return (ER_GraphResult){.res_status       = ER_STATUS_PANIC,
                            .res_panicarg     = entity,
                            .res_panichandler = graph_cycle_panic_handler};
}

ER_GraphResult ER_graph_compute(ER_ASTRootNode *ast_root) {
    // NOTE: zeroing this structure causes pointer fields to become NULL, which
    // constitutes implicit initialization as per uthash documentation
    ER_Graph graph = {0};

    // add entities to the entity hashmap
    ER_ASTNode *ast_ent;
    TAILQ_FOREACH(ast_ent, &ast_root->rt_entities, an_link) {
        assert(ER_AST_NODE_TYPE_ENTITY == ast_ent->an_type);
        ER_ASTEntityNode *ent = (void *)ast_ent;

        GraphEntityResult add_res = graph_add_entity(&graph, ent);
        if (!ER_RESULT_OK(add_res)) {
            return ER_RESULT_CAST(ER_GraphResult, add_res);
        }
    }

    // add relations to the relation hashmap
    ER_ASTNode *ast_rel;
    TAILQ_FOREACH(ast_rel, &ast_root->rt_relations, an_link) {
        assert(ER_AST_NODE_TYPE_RELATION == ast_rel->an_type);
        ER_ASTRelationNode *rel = (void *)ast_rel;

        GraphRelationResult add_res = graph_add_relation(&graph, rel);
        if (!ER_RESULT_OK(add_res)) {
            return ER_RESULT_CAST(ER_GraphResult, add_res);
        }
    }

    // link entities to their parent
    ER_GraphEntity *graph_ent, *graph_ent_tmp;
    HASH_ITER(gen_hh, graph.gr_entities, graph_ent, graph_ent_tmp) {
        // relation handling
        ER_ASTNode *ref_astnode;
        TAILQ_FOREACH(ref_astnode,
                      &graph_ent->gen_astnode->ent_relations,
                      an_link) {
            assert(ER_AST_NODE_TYPE_REFERENCE == ref_astnode->an_type);
            ER_ASTReferenceNode *ref  = (void *)ref_astnode;
            ER_GraphEdge        *edge = calloc(1, sizeof(*edge));
            assert(NULL != edge);

            GraphRelationResult get_res = graph_get_relation(&graph,
                                                             &ref->ref_relname);
            if (!ER_RESULT_OK(get_res)) {
                return ER_RESULT_CAST(ER_GraphResult, get_res);
            }

            ER_GraphRelation *ref_relation = ER_RESULT_GET(get_res);
            edge->ged_astnode              = ref;
            edge->ged_entity               = graph_ent;
            edge->ged_relation             = ref_relation;
            TAILQ_INSERT_TAIL(&ref_relation->gre_edges, edge, ged_link);
        }

        // generalization hierarchies handling
        if (!(graph_ent->gen_astnode->ent_flags & ER_ENTITY_FLAGS_SPECIFIES)) {
            continue;
        }

        GraphEntityResult get_res = graph_get_entity(
            &graph,
            &graph_ent->gen_astnode->ent_specifies);
        if (!ER_RESULT_OK(get_res)) {
            return ER_RESULT_CAST(ER_GraphResult, get_res);
        }

        ER_GraphEntity *specifies = ER_RESULT_GET(get_res);
        graph_ent->gen_specifies  = specifies;
        graph_ent->gen_numspecifiers++;
        TAILQ_INSERT_TAIL(&specifies->gen_specifiers, graph_ent, gen_link);
    }

    // check loops
    HASH_ITER(gen_hh, graph.gr_entities, graph_ent, graph_ent_tmp) {
        if (graph_has_cycle(graph_ent)) {
            return graph_panic_cycle(graph_ent);
        }
    }

    return graph_ok(graph);
}

// NOTE: the following code was generated by Gemini CLI and is **totally
// experimental**

static ER_i32 graph_calculate_entity_layer(ER_GraphEntity *ent) {
    if (ent->gen_specifies == NULL) {
        return 0;
    }

    return 1 + graph_calculate_entity_layer(ent->gen_specifies);
}

// Get the height of the entire specialization subtree starting at ent
static ER_i32 graph_get_tree_height(ER_Graph *graph, ER_GraphEntity *ent) {
    ER_i32 max_child_h = 0;

    ER_GraphEntity *child;
    TAILQ_FOREACH(child, &ent->gen_specifiers, gen_link) {
        ER_i32 child_h = graph_get_tree_height(graph, child);
        max_child_h    = ER_MAX(max_child_h, child_h);
    }

    return ent->gen_h + (max_child_h > 0 ? 100 + max_child_h : 0);
}

void ER_graph_place(ER_Graph *graph) {
    ER_GraphRelation *rel, *tmp_rel;

    // Phase 1: Sizes and Layers
    ER_GraphEntity *ent, *tmp_ent;
    HASH_ITER(gen_hh, graph->gr_entities, ent, tmp_ent) {
        ent->gen_layer = graph_calculate_entity_layer(ent);
        ent->gen_w     = (ent->gen_astnode->ent_name.tok_value.str_len * 12) +
                     ER_SVG_ENTITY_W_PAD;
        ent->gen_h = ER_SVG_ENTITY_H_MIN;

        ER_ASTNode *attr;
        TAILQ_FOREACH(attr, &ent->gen_astnode->ent_attributes, an_link) {
            ER_ASTAttributeNode *atrn = (void *)attr;
            ER_i32 attr_w = (atrn->atr_name.tok_value.str_len * 10) +
                            ER_SVG_ENTITY_W_PAD;
            ent->gen_h += ER_SVG_ATTR_H_STEP;
            if (attr_w > ent->gen_w) {
                ent->gen_w = attr_w;
            }
        }
    }

    // Phase 2: Root-Tree Placement
    ER_i32 col_x[2] = {100, 1200}, col_y[2] = {100, 100};
    int    root_idx = 0;
    HASH_ITER(gen_hh, graph->gr_entities, ent, tmp_ent) {
        if (0 != ent->gen_layer) {
            continue;
        }

        int c        = root_idx % 2;
        ent->gen_col = c;
        ent->gen_x   = col_x[c];
        ent->gen_y   = col_y[c];
        col_y[c] += graph_get_tree_height(graph, ent) + 150;
        root_idx++;
    }

    // Recursive Child Placement
    for (ER_u8 l = 1; l < 5; l++) {
        HASH_ITER(gen_hh, graph->gr_entities, ent, tmp_ent) {
            if (ent->gen_layer != l) {
                continue;
            }

            ER_GraphEntity *p = ent->gen_specifies;
            ent->gen_col      = p->gen_col;
            ent->gen_x        = p->gen_x;
            ent->gen_y        = p->gen_y + p->gen_h + 100;

            // Shift siblings
            ER_GraphEntity *s, *ts;
            HASH_ITER(gen_hh, graph->gr_entities, s, ts) {
                if (s != ent && s->gen_specifies == p &&
                    s->gen_x >= ent->gen_x && s->gen_y == ent->gen_y) {
                    ent->gen_x = s->gen_x + s->gen_w + 50;
                }
            }
        }
    }

    // Phase 3: Relations (Center Column)
    ER_i32 rel_x = 650, rel_y_curr = 100;
    HASH_ITER(gre_hh, graph->gr_relations, rel, tmp_rel) {
        rel->gre_w = (rel->gre_astnode->rel_name.tok_value.str_len * 12) +
                     ER_SVG_RELATION_W_PAD;
        rel->gre_h = ER_SVG_RELATION_H_MIN;

        ER_ASTNode *attr;
        TAILQ_FOREACH(attr, &rel->gre_astnode->rel_attributes, an_link) {
            ER_ASTAttributeNode *atrn = (void *)attr;
            ER_i32 attr_w = (atrn->atr_name.tok_value.str_len * 10) +
                            ER_SVG_RELATION_W_PAD;
            rel->gre_h += ER_SVG_ATTR_H_STEP;
            if (attr_w > rel->gre_w) {
                rel->gre_w = attr_w;
            }
        }

        ER_i32        ay = 0, ec = 0;
        ER_GraphEdge *edge;
        TAILQ_FOREACH(edge, &rel->gre_edges, ged_link) {
            ay += edge->ged_entity->gen_y + edge->ged_entity->gen_h / 2;
            ec++;
        }

        ay         = (ec > 0) ? ay / ec : rel_y_curr;
        rel->gre_x = rel_x;
        rel->gre_y = ay - rel->gre_h / 2;
        if (rel->gre_y < rel_y_curr) {
            rel->gre_y = rel_y_curr;
        }
        rel_y_curr = rel->gre_y + rel->gre_h + 150;

        // Phase 4: Better Vertex Allocation and Surface Distribution
        ER_i32 v_used[4] = {0, 0, 0, 0}; // L, R, T, B
        ER_i32 e_idx     = 0;
        TAILQ_FOREACH(edge, &rel->gre_edges, ged_link) {
            ER_GraphEntity *e = edge->ged_entity;
            ER_i32          v = -1;

            // Strategy: 1. Try side closest to entity. 2. Try side with least
            // usage.
            if (e->gen_x + e->gen_w <= rel->gre_x && v_used[0] == 0) {
                v = 0;
            } else if (e->gen_x >= rel->gre_x + rel->gre_w && v_used[1] == 0) {
                v = 1;
            } else if (e->gen_y + e->gen_h <= rel->gre_y && v_used[2] == 0) {
                v = 2;
            } else if (e->gen_y >= rel->gre_y + rel->gre_h && v_used[3] == 0) {
                v = 3;
            } else { // Find least used
                ER_i32 min_u = v_used[0];
                v            = 0;
                for (ER_u8 i = 1; i < 4; i++) {
                    if (v_used[i] < min_u) {
                        min_u = v_used[i];
                        v     = i;
                    }
                }
            }
            v_used[v]++;

            switch (v) {
                case 0:
                    edge->ged_x1 = rel->gre_x;
                    edge->ged_y1 = rel->gre_y + rel->gre_h / 2;
                    break;

                case 1:
                    edge->ged_x1 = rel->gre_x + rel->gre_w;
                    edge->ged_y1 = rel->gre_y + rel->gre_h / 2;
                    break;

                case 2:
                    edge->ged_x1 = rel->gre_x + rel->gre_w / 2;
                    edge->ged_y1 = rel->gre_y;
                    break;

                case 3:
                    edge->ged_x1 = rel->gre_x + rel->gre_w / 2;
                    edge->ged_y1 = rel->gre_y + rel->gre_h;
                    break;

                default:
                    assert(!"invalid vertex");
            }

            // Distribute on Entity Surface
            if (e->gen_x + e->gen_w <= edge->ged_x1) { // Entity is left
                edge->ged_x2 = e->gen_x + e->gen_w;
                edge->ged_y2 = e->gen_y + 20 + (e_idx * 30) % (e->gen_h - 40);
            } else if (e->gen_x >= edge->ged_x1) { // Entity is right
                edge->ged_x2 = e->gen_x;
                edge->ged_y2 = e->gen_y + 20 + (e_idx * 30) % (e->gen_h - 40);
            } else if (e->gen_y + e->gen_h <= edge->ged_y1) { // Entity is above
                edge->ged_x2 = e->gen_x + 20 + (e_idx * 30) % (e->gen_w - 40);
                edge->ged_y2 = e->gen_y + e->gen_h;
            } else { // Entity is below
                edge->ged_x2 = e->gen_x + 20 + (e_idx * 30) % (e->gen_w - 40);
                edge->ged_y2 = e->gen_y;
            }
            e_idx++;
        }
    }
}
