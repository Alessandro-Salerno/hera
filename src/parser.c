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

// NOTE: we do not clean up allocated structures in case of failure because that
// would make the code totally unreadable (as `defer` is not supported in this
// version of the standard) and also the gain would be minimal since the tool
// will still exit shortly after the error here

#include <ergen/parser.h>
#include <ergen/util.h>
#include <stdbool.h>

typedef struct ParserState {
    ER_TokenList par_tokens;
    ER_u64       par_index;
    ER_String    par_input;
} ParserState;

typedef ER_RESULT(ER_Token) ParserExpectResult;
typedef ER_RESULT(ER_ASTNode *) ParserNodeResult;

// Utilities

static void parser_step(ParserState *parser) {
    parser->par_index++;
}

static ER_Token parser_peek(ParserState *parser) {
    return *EZLD_ARRAY_AT(parser->par_tokens, parser->par_index);
}

static ParserExpectResult parser_expect(ParserState *parser,
                                        ER_u32       accepted_mask) {
    ER_Token tok = parser_peek(parser);

    if (!(tok.tok_type & accepted_mask)) {
        return (ParserExpectResult){.res_status = ER_STATUS_ERR,
                                    .res_err    = "unexpected token"};
    }

    parser_step(parser);
    return (ParserExpectResult){.res_status = ER_STATUS_OK, .res_val = tok};
}

static bool parser_matches_types(ParserState *parser, ER_u32 accepted_mask) {
    return ER_RESULT_OK(parser_expect(parser, accepted_mask));
}

static ParserNodeResult parser_node_ok(ER_ASTNode *node) {
    return (ParserNodeResult){.res_status = ER_STATUS_OK, .res_val = node};
}

static ParserNodeResult parser_node_err(const char *message) {
    return (ParserNodeResult){.res_status = ER_STATUS_ERR, .res_err = message};
}

static ParserNodeResult parser_node_err_expect(ParserExpectResult err) {
    return parser_node_err(err.res_err);
}

static void parser_panic_handler(void *arg) {
    ParserState *parser = arg;
    ER_Token     tok    = parser_peek(parser);

    ER_generic_panic_output(parser->par_input,
                            tok.tok_row,
                            tok.tok_col,
                            tok.tok_rowstart,
                            tok.tok_col - 1,
                            tok.tok_value.str_len,
                            "unexpected token '%.*s'",
                            ER_STRING_PRINTF(tok.tok_value));
}

static ER_ParserResult parser_panic(ParserState *parser) {
    ParserState *snapshot = calloc(1, sizeof(*snapshot));
    assert(NULL != snapshot);
    *snapshot = *parser;

    return (ER_ParserResult){.res_status       = ER_STATUS_PANIC,
                             .res_panicarg     = snapshot,
                             .res_panichandler = parser_panic_handler};
}

// Parser logic

ParserNodeResult parser_do_attribute(ParserState *parser) {
    ParserExpectResult name_tok_res = parser_expect(parser,
                                                    ER_TOKEN_TYPE_IDENTIFIER |
                                                        ER_TOKEN_TYPE_STRING);
    if (!ER_RESULT_OK(name_tok_res)) {
        return parser_node_err_expect(name_tok_res);
    }

    ER_Token             name_tok = ER_RESULT_GET(name_tok_res);
    ER_ASTAttributeNode *node     = calloc(1, sizeof(*node));
    assert(NULL != node);
    node->atr_node.an_type = ER_AST_NODE_TYPE_ATTRIBUTE;
    node->atr_name         = name_tok;

    if (parser_matches_types(parser, ER_TOKEN_TYPE_KEY)) {
        node->atr_flags |= ER_ATTRIBUTE_FLAGS_KEY;
    }

    return parser_node_ok((ER_ASTNode *)node);
}

ParserNodeResult parser_do_entity(ParserState *parser) {
    ParserExpectResult name_tok_res = parser_expect(parser,
                                                    ER_TOKEN_TYPE_IDENTIFIER |
                                                        ER_TOKEN_TYPE_STRING);
    if (!ER_RESULT_OK(name_tok_res)) {
        return parser_node_err_expect(name_tok_res);
    }

    ER_Token          name_tok = ER_RESULT_GET(name_tok_res);
    ER_ASTEntityNode *entity   = calloc(1, sizeof(*entity));
    assert(NULL != entity);
    entity->ent_node.an_type = ER_AST_NODE_TYPE_ENTITY;
    entity->ent_name         = name_tok;
    TAILQ_INIT(&entity->ent_attributes);

    // handle optional [specifies <name>]
    if (parser_matches_types(parser, ER_TOKEN_TYPE_SPECIFIES)) {
        ParserExpectResult parent_name_res = parser_expect(
            parser,
            ER_TOKEN_TYPE_IDENTIFIER | ER_TOKEN_TYPE_STRING);
        if (!ER_RESULT_OK(parent_name_res)) {
            return parser_node_err_expect(name_tok_res);
        }

        ER_Token parent_tok   = ER_RESULT_GET(parent_name_res);
        entity->ent_specifies = parent_tok;
        entity->ent_flags |= ER_ENTITY_FLAGS_SPECIFIES;
    }

    ParserExpectResult lblock_res = parser_expect(parser, ER_TOKEN_TYPE_LBLOCK);
    if (!ER_RESULT_OK(lblock_res)) {
        return parser_node_err_expect(lblock_res);
    }

    // parse block contents (attributes)
    // we use parser_matches_types instead of parsre_expect because an empty
    // block is legal and there's no path in which this can return the error to
    // the caller of parser_do_entity
    while (parser_matches_types(parser, ER_TOKEN_TYPE_ATTRIBUTE)) {
        ParserNodeResult attr_res = parser_do_attribute(parser);
        if (!ER_RESULT_OK(attr_res)) {
            return attr_res;
        }

        ParserExpectResult semic_res = parser_expect(parser,
                                                     ER_TOKEN_TYPE_SEMICOLON);
        if (!ER_RESULT_OK(semic_res)) {
            return parser_node_err_expect(semic_res);
        }

        ER_ASTNode *attr_node = ER_RESULT_GET(attr_res);
        TAILQ_INSERT_TAIL(&entity->ent_attributes, attr_node, an_link);
    }

    ParserExpectResult rblock_res = parser_expect(parser, ER_TOKEN_TYPE_RBLOCK);
    if (!ER_RESULT_OK(rblock_res)) {
        return parser_node_err_expect(rblock_res);
    }

    return parser_node_ok((ER_ASTNode *)entity);
}

ParserNodeResult parser_do_total(ParserState *parser) {
    ParserExpectResult entity_res = parser_expect(parser, ER_TOKEN_TYPE_ENTITY);
    if (!ER_RESULT_OK(entity_res)) {
        return parser_node_err_expect(entity_res);
    }

    ParserNodeResult body_res = parser_do_entity(parser);
    if (!ER_RESULT_OK(body_res)) {
        return body_res;
    }

    ER_ASTEntityNode *entity = (void *)ER_RESULT_GET(body_res);
    entity->ent_flags        = ER_ENTITY_FLAGS_TOTAL;
    return body_res;
}

ParserNodeResult parser_do_reference(ParserState *parser) {
    ParserExpectResult name_tok_res = parser_expect(parser,
                                                    ER_TOKEN_TYPE_IDENTIFIER |
                                                        ER_TOKEN_TYPE_STRING);
    if (!ER_RESULT_OK(name_tok_res)) {
        return parser_node_err_expect(name_tok_res);
    }

    ER_Token name_tok = ER_RESULT_GET(name_tok_res);

    ER_ASTReferenceNode *reference = calloc(1, sizeof(*reference));
    reference->ref_node.an_type    = ER_AST_NODE_TYPE_REFERENCE;
    reference->ref_entname         = name_tok;

    // (
    ParserExpectResult lpar_res = parser_expect(parser, ER_TOKEN_TYPE_LPAREN);
    if (!ER_RESULT_OK(lpar_res)) {
        return parser_node_err_expect(lpar_res);
    }

    // <lcard>, <rcard>
    ParserExpectResult lcard_res = parser_expect(
        parser,
        ER_TOKEN_TYPE_NUMBER | ER_TOKEN_TYPE_IDENTIFIER | ER_TOKEN_TYPE_STRING);
    if (!ER_RESULT_OK(lcard_res)) {
        return parser_node_err_expect(lcard_res);
    }

    ParserExpectResult comma_res = parser_expect(parser, ER_TOKEN_TYPE_COMMA);
    if (!ER_RESULT_OK(comma_res)) {
        return parser_node_err_expect(comma_res);
    }

    ParserExpectResult rcard_res = parser_expect(
        parser,
        ER_TOKEN_TYPE_NUMBER | ER_TOKEN_TYPE_IDENTIFIER | ER_TOKEN_TYPE_STRING);
    if (!ER_RESULT_OK(rcard_res)) {
        return parser_node_err_expect(rcard_res);
    }

    // )
    ParserExpectResult rpar_res = parser_expect(parser, ER_TOKEN_TYPE_RPAREN);
    if (!ER_RESULT_OK(rpar_res)) {
        return parser_node_err_expect(rpar_res);
    }

    reference->ref_lcard = ER_RESULT_GET(lcard_res);
    reference->ref_rcard = ER_RESULT_GET(rcard_res);
    return parser_node_ok((ER_ASTNode *)reference);
}

ParserNodeResult parser_do_relation(ParserState *parser) {
    ParserExpectResult name_tok_res = parser_expect(parser,
                                                    ER_TOKEN_TYPE_IDENTIFIER |
                                                        ER_TOKEN_TYPE_STRING);
    if (!ER_RESULT_OK(name_tok_res)) {
        return parser_node_err_expect(name_tok_res);
    }

    ER_Token name_tok = ER_RESULT_GET(name_tok_res);

    ParserExpectResult lblock_res = parser_expect(parser, ER_TOKEN_TYPE_LBLOCK);
    if (!ER_RESULT_OK(lblock_res)) {
        return parser_node_err_expect(lblock_res);
    }

    ER_ASTRelationNode *relation = calloc(1, sizeof(*relation));
    relation->rel_node.an_type   = ER_AST_NODE_TYPE_RELATION;
    relation->rel_name           = name_tok;
    TAILQ_INIT(&relation->rel_entities);
    TAILQ_INIT(&relation->rel_attributes);

    // references and attributes can be interleaved...
    ParserExpectResult memb_result;
    while ((memb_result = parser_expect(parser,
                                        ER_TOKEN_TYPE_ENTITY |
                                            ER_TOKEN_TYPE_ATTRIBUTE)),
           ER_RESULT_OK(memb_result)) {
        ER_Token memb = ER_RESULT_GET(memb_result);

        if (ER_TOKEN_TYPE_ENTITY == memb.tok_type) {
            ParserNodeResult ref_res = parser_do_reference(parser);
            if (!ER_RESULT_OK(ref_res)) {
                return ref_res;
            }

            ER_ASTNode *ref = ER_RESULT_GET(ref_res);
            TAILQ_INSERT_TAIL(&relation->rel_entities, ref, an_link);
        } else {
            ParserNodeResult attr_res = parser_do_attribute(parser);
            if (!ER_RESULT_OK(attr_res)) {
                return attr_res;
            }

            ER_ASTNode *attr = ER_RESULT_GET(attr_res);
            TAILQ_INSERT_TAIL(&relation->rel_attributes, attr, an_link);
        }

        ParserExpectResult semic_res = parser_expect(parser,
                                                     ER_TOKEN_TYPE_SEMICOLON);
        if (!ER_RESULT_OK(semic_res)) {
            return parser_node_err_expect(semic_res);
        }
    }

    ParserExpectResult rblock_res = parser_expect(parser, ER_TOKEN_TYPE_RBLOCK);
    if (!ER_RESULT_OK(rblock_res)) {
        return parser_node_err_expect(rblock_res);
    }

    return parser_node_ok((ER_ASTNode *)relation);
}

ER_ParserResult ER_parser_run(ER_TokenList tokens, ER_String input) {
    ParserState parser = {0};
    parser.par_tokens  = tokens;
    parser.par_input   = input;

    ER_ASTRootNode root;
    root.rt_node.an_type = ER_AST_NODE_TYPE_ROOT;
    TAILQ_INIT(&root.rt_entities);
    TAILQ_INIT(&root.rt_relations);

    ParserExpectResult keyword_res;
    while ((keyword_res = parser_expect(&parser,
                                        ER_TOKEN_TYPE_ENTITY |
                                            ER_TOKEN_TYPE_TOTAL |
                                            ER_TOKEN_TYPE_RELATION)),
           ER_RESULT_OK(keyword_res)) {
        ER_Token tok = ER_RESULT_GET(keyword_res);

        if (ER_TOKEN_TYPE_ENTITY == tok.tok_type) {
            ParserNodeResult entity_res = parser_do_entity(&parser);
            if (!ER_RESULT_OK(entity_res)) {
                return parser_panic(&parser);
            }
            ER_ASTNode *entity = ER_RESULT_GET(entity_res);
            TAILQ_INSERT_TAIL(&root.rt_entities, entity, an_link);
        } else if (ER_TOKEN_TYPE_TOTAL == tok.tok_type) {
            ParserNodeResult entity_res = parser_do_total(&parser);
            if (!ER_RESULT_OK(entity_res)) {
                return parser_panic(&parser);
            }
            ER_ASTNode *entity = ER_RESULT_GET(entity_res);
            TAILQ_INSERT_TAIL(&root.rt_entities, entity, an_link);
        } else {
            ParserNodeResult entity_res = parser_do_relation(&parser);
            if (!ER_RESULT_OK(entity_res)) {
                return parser_panic(&parser);
            }
            ER_ASTNode *relation = ER_RESULT_GET(entity_res);
            TAILQ_INSERT_TAIL(&root.rt_relations, relation, an_link);
        }
    }

    if (!parser_matches_types(&parser, ER_TOKEN_TYPE_EOF)) {
        return parser_panic(&parser);
    }

    return (ER_ParserResult){.res_status = ER_STATUS_OK, .res_val = root};
}
