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
#include <ergen/svg.h>
#include <stdlib.h>
#include <stdbool.h>

typedef ER_RESULT(ER_GraphEntity *) GraphEntityResult;
typedef ER_RESULT(ER_GraphRelation *) GraphRelationResult;

static inline void graph_hash_string(ER_String *s, unsigned int *hashv) {
    unsigned int r;
    HASH_FUNCTION(s->str_buf, s->str_len, r);
    *hashv = r;
}

#undef HASH_FUNCTION
#define HASH_FUNCTION(keyptr, keylen, hashv) graph_hash_string(keyptr, &hashv)

#undef HASH_KEYCMP
#define HASH_KEYCMP(a, b, n) !ER_STRING_EQ(*(ER_String *)a, *(ER_String *)b)

static GraphEntityResult graph_get_entity(ER_Graph *graph, ER_String name) {
    ER_GraphEntity *ent = NULL;
    HASH_FIND(gen_hh, graph->gr_entities, &name, sizeof(name), ent);
    if (NULL != ent) return (GraphEntityResult){.res_status = ER_STATUS_OK, .res_val = ent};
    return (GraphEntityResult){.res_status = ER_STATUS_ERR, .res_err = "unknown entity"};
}

static GraphRelationResult graph_get_relation(ER_Graph *graph, ER_String name) {
    ER_GraphRelation *rel = NULL;
    HASH_FIND(gre_hh, graph->gr_relations, &name, sizeof(name), rel);
    if (NULL != rel) return (GraphRelationResult){.res_status = ER_STATUS_OK, .res_val = rel};
    return (GraphRelationResult){.res_status = ER_STATUS_ERR, .res_err = "unknown relation"};
}

static GraphEntityResult graph_add_entity(ER_Graph *graph, ER_ASTEntityNode *entity) {
    GraphEntityResult get_res = graph_get_entity(graph, entity->ent_name);
    if (ER_RESULT_OK(get_res)) return (GraphEntityResult){.res_status = ER_STATUS_ERR, .res_err = "entity redefined"};
    ER_GraphEntity *graph_entity = calloc(1, sizeof(*graph_entity));
    assert(NULL != graph_entity);
    graph_entity->gen_astnode = entity;
    HASH_ADD(gen_hh, graph->gr_entities, gen_astnode->ent_name, sizeof(ER_String), graph_entity);
    return (GraphEntityResult){.res_status = ER_STATUS_OK, .res_val = graph_entity};
}

static GraphRelationResult graph_add_relation(ER_Graph *graph, ER_ASTRelationNode *relation) {
    GraphRelationResult get_res = graph_get_relation(graph, relation->rel_name);
    if (ER_RESULT_OK(get_res)) return (GraphRelationResult){.res_status = ER_STATUS_ERR, .res_err = "relation redefined"};
    ER_GraphRelation *graph_relation = calloc(1, sizeof(*graph_relation));
    assert(NULL != graph_relation);
    graph_relation->gre_astnode = relation;
    TAILQ_INIT(&graph_relation->gre_edges);
    HASH_ADD(gre_hh, graph->gr_relations, gre_astnode->rel_name, sizeof(ER_String), graph_relation);
    return (GraphRelationResult){.res_status = ER_STATUS_OK, .res_val = graph_relation};
}

static inline ER_GraphResult graph_err(const char *msg) { return (ER_GraphResult){.res_status = ER_STATUS_ERR, .res_err = msg}; }
static inline ER_GraphResult graph_ok(ER_Graph graph) { return (ER_GraphResult){.res_status = ER_STATUS_OK, .res_val = graph}; }

ER_GraphResult er_graph_compute(ER_ASTRootNode *ast_root) {
    ER_Graph graph = {0};
    ER_ASTNode *ast_ent;
    TAILQ_FOREACH(ast_ent, &ast_root->rt_entities, an_link) {
        GraphEntityResult add_res = graph_add_entity(&graph, (void *)ast_ent);
        if (!ER_RESULT_OK(add_res)) return graph_err(ER_RESULT_ERROR(add_res));
    }
    ER_ASTNode *ast_rel;
    TAILQ_FOREACH(ast_rel, &ast_root->rt_relations, an_link) {
        GraphRelationResult add_res = graph_add_relation(&graph, (void *)ast_rel);
        if (!ER_RESULT_OK(add_res)) return graph_err(ER_RESULT_ERROR(add_res));
    }
    ER_GraphEntity *graph_ent, *graph_ent_tmp;
    HASH_ITER(gen_hh, graph.gr_entities, graph_ent, graph_ent_tmp) {
        if (!(graph_ent->gen_astnode->ent_flags & ER_ENTITY_FLAGS_SPECIFIES)) continue;
        GraphEntityResult get_res = graph_get_entity(&graph, graph_ent->gen_astnode->ent_specifies);
        if (!ER_RESULT_OK(get_res)) return graph_err(ER_RESULT_ERROR(get_res));
        graph_ent->gen_specifies = ER_RESULT_GET(get_res);
    }
    ER_GraphRelation *graph_rel, *graph_rel_tmp;
    HASH_ITER(gre_hh, graph.gr_relations, graph_rel, graph_rel_tmp) {
        ER_ASTNode *ref_astnode;
        TAILQ_FOREACH(ref_astnode, &graph_rel->gre_astnode->rel_entities, an_link) {
            ER_ASTReferenceNode *ref = (void *)ref_astnode;
            ER_GraphEdge *edge = calloc(1, sizeof(*edge));
            assert(NULL != edge);
            GraphEntityResult get_res = graph_get_entity(&graph, ref->ref_entname);
            if (!ER_RESULT_OK(get_res)) return graph_err(ER_RESULT_ERROR(get_res));
            edge->ged_astnode = ref;
            edge->ged_entity = ER_RESULT_GET(get_res);
            edge->ged_relation = graph_rel;
            TAILQ_INSERT_TAIL(&graph_rel->gre_edges, edge, ged_link);
        }
    }
    return graph_ok(graph);
}

static bool is_parent_entity(ER_Graph *graph, ER_GraphEntity *ent) {
    ER_GraphEntity *curr, *tmp;
    HASH_ITER(gen_hh, graph->gr_entities, curr, tmp) {
        if (curr->gen_specifies == ent) return true;
    }
    return false;
}

void er_graph_place(ER_Graph *graph) {
    ER_GraphEntity *ent, *tmp_ent;
    ER_GraphRelation *rel, *tmp_rel;

    ER_i32 left_y = 100;
    ER_i32 right_y = 100;
    ER_i32 left_x = 100;
    ER_i32 right_x = 1600;
    ER_i32 rel_x = 850;
    ER_i32 rel_y = 100;

    // 1. Placement: Staggered Entities
    int idx = 0;
    HASH_ITER(gen_hh, graph->gr_entities, ent, tmp_ent) {
        ent->gen_w = (ent->gen_astnode->ent_name.str_len * 12) + ER_SVG_ENTITY_W_PAD;
        ent->gen_h = ER_SVG_ENTITY_H_MIN;
        ER_ASTNode *attr;
        TAILQ_FOREACH(attr, &ent->gen_astnode->ent_attributes, an_link) {
            ER_ASTAttributeNode *atrn = (void *)attr;
            ER_i32 attr_w = (atrn->atr_name.str_len * 10) + ER_SVG_ENTITY_W_PAD;
            if (attr_w > ent->gen_w) ent->gen_w = attr_w;
            ent->gen_h += ER_SVG_ATTR_H_STEP;
        }

        if (ent->gen_specifies != NULL) {
            ent->gen_x = ent->gen_specifies->gen_x;
            ent->gen_y = ent->gen_specifies->gen_y + ent->gen_specifies->gen_h + 350;
            ER_GraphEntity *sib, *tmp_sib;
            HASH_ITER(gen_hh, graph->gr_entities, sib, tmp_sib) {
                if (sib != ent && sib->gen_specifies == ent->gen_specifies && sib->gen_x != 0) {
                    ent->gen_x += sib->gen_w + 150;
                }
            }
        } else {
            if (idx % 2 == 0) {
                ent->gen_x = left_x;
                ent->gen_y = left_y;
                left_y += ent->gen_h + 350;
            } else {
                ent->gen_x = right_x;
                ent->gen_y = right_y;
                right_y += ent->gen_h + 350;
            }
            idx++;
        }
    }

    // 2. Relations: Middle column
    HASH_ITER(gre_hh, graph->gr_relations, rel, tmp_rel) {
        rel->gre_w = (rel->gre_astnode->rel_name.str_len * 12) + ER_SVG_RELATION_W_PAD;
        rel->gre_h = ER_SVG_RELATION_H_MIN;
        ER_ASTNode *attr;
        TAILQ_FOREACH(attr, &rel->gre_astnode->rel_attributes, an_link) {
            ER_ASTAttributeNode *atrn = (void *)attr;
            ER_i32 attr_w = (atrn->atr_name.str_len * 10) + ER_SVG_RELATION_W_PAD;
            if (attr_w > rel->gre_w) rel->gre_w = attr_w;
            rel->gre_h += ER_SVG_ATTR_H_STEP;
        }

        rel->gre_x = rel_x;
        rel->gre_y = rel_y;
        rel_y += rel->gre_h + 450;

        // 3. Routing: Use different vertices for different sides
        ER_GraphEdge *edge;
        TAILQ_FOREACH(edge, &rel->gre_edges, ged_link) {
            ER_GraphEntity *e = edge->ged_entity;
            
            if (e->gen_x + e->gen_w <= rel->gre_x) {
                // Entity on the left -> Left vertex
                edge->ged_x1 = rel->gre_x; 
                edge->ged_y1 = rel->gre_y + rel->gre_h / 2;
                edge->ged_x2 = e->gen_x + e->gen_w;
                edge->ged_y2 = e->gen_y + e->gen_h / 2;
            } else if (e->gen_x >= rel->gre_x + rel->gre_w) {
                // Entity on the right -> Right vertex
                edge->ged_x1 = rel->gre_x + rel->gre_w;
                edge->ged_y1 = rel->gre_y + rel->gre_h / 2;
                edge->ged_x2 = e->gen_x;
                edge->ged_y2 = e->gen_y + e->gen_h / 2;
            } else if (e->gen_y + e->gen_h <= rel->gre_y) {
                // Entity above -> Top vertex
                edge->ged_x1 = rel->gre_x + rel->gre_w / 2;
                edge->ged_y1 = rel->gre_y;
                edge->ged_x2 = e->gen_x + e->gen_w / 2;
                edge->ged_y2 = e->gen_y + e->gen_h;
            } else {
                // Entity below -> Bottom vertex
                edge->ged_x1 = rel->gre_x + rel->gre_w / 2;
                edge->ged_y1 = rel->gre_y + rel->gre_h;
                edge->ged_x2 = e->gen_x + e->gen_w / 2;
                edge->ged_y2 = e->gen_y;
            }

            // Offset to avoid generalization arrow collisions
            if (is_parent_entity(graph, e) && edge->ged_y2 == e->gen_y + e->gen_h) edge->ged_x2 -= 40;
            if (e->gen_specifies != NULL && edge->ged_y2 == e->gen_y) edge->ged_x2 += 40;
        }
    }
}
