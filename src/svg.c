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

#include <hera/svg.h>
#include <hera/util.h>
#include <stdio.h>

static void svg_emit_attributes(ER_ASTNode *first, ER_i32 x, ER_i32 y) {
    ER_ASTNode *attr;
    ER_i32      curr_y = y;

    for (attr = first; attr != NULL; attr = TAILQ_NEXT(attr, an_link)) {
        ER_ASTAttributeNode *atrn   = (void *)attr;
        const char          *weight = (atrn->atr_flags & ER_ATTRIBUTE_FLAGS_KEY)
                                          ? "900"
                                          : "normal";
        const char          *color  = (atrn->atr_flags & ER_ATTRIBUTE_FLAGS_KEY)
                                          ? ER_SVG_KEY_ATTR_COLOR
                                          : "black";
        const char          *decor  = (atrn->atr_flags & ER_ATTRIBUTE_FLAGS_KEY)
                                          ? "text-decoration=\"underline\""
                                          : "";

        printf(
            "<text x=\"%d\" y=\"%d\" font-family=\"monospace\" "
            "font-size=\"%d\" font-weight=\"%s\" fill=\"%s\" %s>%.*s</text>\n",
            x,
            curr_y,
            ER_SVG_FONT_SIZE_M,
            weight,
            color,
            decor,
            ER_STRING_PRINTF(atrn->atr_name.tok_value));
        curr_y += ER_SVG_ATTR_H_STEP;
    }
}

static void svg_emit_entity(ER_GraphEntity *ent) {
    printf("<g id=\"entity_%.*s\">\n",
           ER_STRING_PRINTF(ent->gen_astnode->ent_name.tok_value));
    printf("<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" fill=\"white\" "
           "stroke=\"black\" stroke-width=\"2\" />\n",
           ent->gen_x,
           ent->gen_y,
           ent->gen_w,
           ent->gen_h);

    printf("<text x=\"%d\" y=\"%d\" font-family=\"monospace\" font-size=\"%d\" "
           "font-weight=\"bold\" text-anchor=\"middle\">%.*s</text>\n",
           ent->gen_x + ent->gen_w / 2,
           ent->gen_y + 25,
           ER_SVG_FONT_SIZE_L,
           ER_STRING_PRINTF(ent->gen_astnode->ent_name.tok_value));

    printf("<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke=\"black\" "
           "stroke-width=\"1\" />\n",
           ent->gen_x,
           ent->gen_y + 35,
           ent->gen_x + ent->gen_w,
           ent->gen_y + 35);

    svg_emit_attributes(TAILQ_FIRST(&ent->gen_astnode->ent_attributes),
                        ent->gen_x + 10,
                        ent->gen_y + 55);
    printf("</g>\n");
}

static void svg_emit_relation(ER_GraphRelation *rel) {
    printf("<g id=\"relation_%.*s\">\n",
           ER_STRING_PRINTF(rel->gre_astnode->rel_name.tok_value));
    ER_i32 cx = rel->gre_x + rel->gre_w / 2;
    ER_i32 cy = rel->gre_y + rel->gre_h / 2;

    printf("<polygon points=\"%d,%d %d,%d %d,%d %d,%d\" fill=\"white\" "
           "stroke=\"black\" stroke-width=\"2\" />\n",
           cx,
           rel->gre_y,
           rel->gre_x + rel->gre_w,
           cy,
           cx,
           rel->gre_y + rel->gre_h,
           rel->gre_x,
           cy);

    printf("<text x=\"%d\" y=\"%d\" font-family=\"monospace\" font-size=\"%d\" "
           "font-weight=\"bold\" text-anchor=\"middle\">%.*s</text>\n",
           cx,
           cy + 5,
           ER_SVG_FONT_SIZE_L,
           ER_STRING_PRINTF(rel->gre_astnode->rel_name.tok_value));

    svg_emit_attributes(TAILQ_FIRST(&rel->gre_astnode->rel_attributes),
                        rel->gre_x + 10,
                        rel->gre_y + rel->gre_h + 20);

    printf("</g>\n");
}

static void svg_emit_edge(ER_GraphEdge *edge) {
    printf("<g class=\"edge\">\n");
    ER_i32 x1 = edge->ged_x1;
    ER_i32 y1 = edge->ged_y1;
    ER_i32 x2 = edge->ged_x2;
    ER_i32 y2 = edge->ged_y2;

    // Intelligent orthogonal routing
    if (y1 == y2 || x1 == x2) {
        printf("<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke=\"black\" "
               "stroke-width=\"2\" />\n",
               x1,
               y1,
               x2,
               y2);
    } else {
        ER_i32 mid_x = (x1 + x2) / 2;
        printf("<polyline points=\"%d,%d %d,%d %d,%d %d,%d\" fill=\"none\" "
               "stroke=\"black\" stroke-width=\"2\" />\n",
               x1,
               y1,
               mid_x,
               y1,
               mid_x,
               y2,
               x2,
               y2);
    }

    // Cardinality labels: always near the entity side (x2, y2)
    ER_i32 lx = x2;
    ER_i32 ly = y2;

    if (x2 < x1) {
        lx += 5;
    } else {
        lx -= 45;
    }
    if (y2 < y1) {
        ly += 15;
    } else {
        ly -= 5;
    }

    printf("<text x=\"%d\" y=\"%d\" font-family=\"monospace\" font-size=\"%d\" "
           "font-weight=\"bold\" fill=\"%s\">(%.*s, %.*s)</text>\n",
           lx,
           ly,
           ER_SVG_FONT_SIZE_S,
           ER_SVG_CARD_COLOR,
           ER_STRING_PRINTF(edge->ged_astnode->ref_lcard.tok_value),
           ER_STRING_PRINTF(edge->ged_astnode->ref_rcard.tok_value));
    printf("</g>\n");
}

static void svg_emit_generalizations(ER_Graph *graph) {
    ER_GraphEntity *parent, *tmp_parent;
    HASH_ITER(gen_hh, graph->gr_entities, parent, tmp_parent) {
        ER_GraphEntity *child;
        ER_u64          child_count = parent->gen_numspecifiers;

        if (0 == child_count) {
            continue;
        }

        printf("<g class=\"generalization\">\n");
        ER_i32 px      = parent->gen_x + parent->gen_w / 2;
        ER_i32 py      = parent->gen_y + parent->gen_h;
        ER_i32 merge_y = py + ER_SVG_GEN_MERGE_Y_OFF;

        // Stem to parent with arrowhead
        printf("<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke=\"black\" "
               "stroke-width=\"3\" marker-end=\"url(#arrow%s)\" />\n",
               px,
               merge_y,
               px,
               py,
               (parent->gen_astnode->ent_flags & ER_ENTITY_FLAGS_TOTAL)
                   ? "Total"
                   : "Partial");

        ER_i32 min_x = px;
        ER_i32 max_x = px;
        TAILQ_FOREACH(child, &parent->gen_specifiers, gen_link) {
            ER_i32 cx = child->gen_x + child->gen_w / 2;
            ER_i32 cy = child->gen_y;
            min_x     = ER_MIN(min_x, cx);
            max_x     = ER_MAX(cx, max_x);

            printf("<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" "
                   "stroke=\"black\" stroke-width=\"2\" />\n",
                   cx,
                   cy,
                   cx,
                   merge_y);
        }

        if (min_x != max_x || min_x != px) {
            printf("<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" "
                   "stroke=\"black\" stroke-width=\"2\" />\n",
                   min_x,
                   merge_y,
                   max_x,
                   merge_y);
        }

        printf("</g>\n");
    }
}

void ER_svg_emit(ER_Graph *graph) {
    printf("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
    printf("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" "
           "height=\"%d\">\n",
           ER_SVG_WIDTH,
           ER_SVG_HEIGHT);
    printf("<defs>\n");
    printf("  <marker id=\"arrowPartial\" markerWidth=\"15\" "
           "markerHeight=\"15\" refX=\"15\" refY=\"7.5\" orient=\"auto\">\n");
    printf("    <path d=\"M0,0 L15,7.5 L0,15 Z\" fill=\"white\" "
           "stroke=\"black\" stroke-width=\"1\" />\n");
    printf("  </marker>\n");
    printf("  <marker id=\"arrowTotal\" markerWidth=\"15\" markerHeight=\"15\" "
           "refX=\"15\" refY=\"7.5\" orient=\"auto\">\n");
    printf("    <path d=\"M0,0 L15,7.5 L0,15 Z\" fill=\"black\" />\n");
    printf("  </marker>\n");
    printf("</defs>\n");

    printf("<g id=\"edges\">\n");
    ER_GraphRelation *rel, *tmp_rel;
    HASH_ITER(gre_hh, graph->gr_relations, rel, tmp_rel) {
        ER_GraphEdge *edge;
        TAILQ_FOREACH(edge, &rel->gre_edges, ged_link) {
            svg_emit_edge(edge);
        }
    }
    printf("</g>\n");

    printf("<g id=\"generalizations\">\n");
    svg_emit_generalizations(graph);
    printf("</g>\n");

    printf("<g id=\"entities\">\n");
    ER_GraphEntity *ent, *tmp_ent;
    HASH_ITER(gen_hh, graph->gr_entities, ent, tmp_ent) {
        svg_emit_entity(ent);
    }
    printf("</g>\n");

    printf("<g id=\"relations\">\n");
    HASH_ITER(gre_hh, graph->gr_relations, rel, tmp_rel) {
        svg_emit_relation(rel);
    }
    printf("</g>\n");

    printf("</svg>\n");
}
