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
#include <hera/lexer.h>
#include <hera/util.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum LexerAction {
    LEXER_ACTION_DISCARD, // skip entirely
    LEXER_ACTION_IGNORE, // ignore this run (remember: instructions can tell the
                         // engine to change handler, this is used to recheck
                         // the same character with another handler)
    LEXER_ACTION_PUSH
} LexerAction;

typedef struct LexerInstruction LexerInstruction;
typedef LexerInstruction        LexerCharHandler(ER_WChar c);

struct LexerInstruction {
    LexerCharHandler *li_handler;
    LexerAction       li_charaction;
    LexerAction       li_tokaction;
    ER_TokenType      li_toktype;
};

typedef struct LexerState {
    ER_String ls_input;
    ER_u64    ls_next;
    ER_u64    ls_row;
    ER_u64    ls_col;
    ER_u64    ls_bincol;
    ER_u64    ls_rowstart;
} LexerState;

// Utilities

static ER_WCharResult lexer_fetch_result(ER_WChar c) {
    return (ER_WCharResult){.res_status = ER_STATUS_OK, .res_val = c};
}

static ER_u64 lexer_char_width(LexerState *lexer) {
    return ER_RESULT_GET(
        ER_char_width(ER_STRING_SUB(lexer->ls_input, lexer->ls_next)));
}

static void lexer_step(LexerState *lexer) {
    // we do ER_RESULT_GET here because this cannot fail since step is only ever
    // called after a character fetch and the character fetch would have already
    // failed if the length was invalid
    ER_u64 char_w = lexer_char_width(lexer);
    // we step the column only by one because this is used for accounting, not
    // binary access
    lexer->ls_col++;
    // and we advance binary indicators by char_w
    lexer->ls_next += char_w;
    lexer->ls_bincol += char_w;
}

static void lexer_reset_row(LexerState *lexer) {
    lexer->ls_row++;
    lexer->ls_col      = 1;
    lexer->ls_bincol   = 0;
    lexer->ls_rowstart = lexer->ls_next + 1;
}

// NOTE: we include <= and not < because parser_peek has a special case for that
static bool lexer_can_peek(LexerState *lexer) {
    return lexer->ls_next <= lexer->ls_input.str_len;
}

static ER_WCharResult lexer_peek(LexerState *lexer) {
    if (lexer->ls_next == lexer->ls_input.str_len) {
        return lexer_fetch_result(EOF);
    }

    return ER_char_to_wchar(ER_STRING_SUB(lexer->ls_input, lexer->ls_next));
}

static bool lexer_is_keyword_identifier_char(ER_WChar c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ||
           c > 127;
}

static bool lexer_is_number_char(ER_WChar c) {
    return c >= '0' && c <= '9';
}

static ER_TokenType lexer_token_type(ER_String buffer, ER_TokenType hint) {
    if (hint != ER_TOKEN_TYPE_NONE) {
        return hint;
    }

    // otherwise, try to match it against known keywords
    if (ER_STRING_EQ_LITERAL(buffer, "entity")) {
        return ER_TOKEN_TYPE_ENTITY;
    } else if (ER_STRING_EQ_LITERAL(buffer, "attribute")) {
        return ER_TOKEN_TYPE_ATTRIBUTE;
    } else if (ER_STRING_EQ_LITERAL(buffer, "relation")) {
        return ER_TOKEN_TYPE_RELATION;
    } else if (ER_STRING_EQ_LITERAL(buffer, "total")) {
        return ER_TOKEN_TYPE_TOTAL;
    } else if (ER_STRING_EQ_LITERAL(buffer, "specifies")) {
        return ER_TOKEN_TYPE_SPECIFIES;
    } else if (ER_STRING_EQ_LITERAL(buffer, "key")) {
        return ER_TOKEN_TYPE_KEY;
    } else if (ER_STRING_EQ_LITERAL(buffer, "exclusive")) {
        return ER_TOKEN_TYPE_EXCLUSIVE;
    } else if (ER_STRING_EQ_LITERAL(buffer, "alias")) {
        return ER_TOKEN_TYPE_ALIAS;
    }

    // at this point we assume it is an identifier
    return ER_TOKEN_TYPE_IDENTIFIER;
}

static void lexer_panic_handler(void *arg) {
    LexerState *lexer = arg;
    ER_generic_panic_output(lexer->ls_input,
                            lexer->ls_row,
                            lexer->ls_col,
                            lexer->ls_rowstart,
                            lexer->ls_col - 1,
                            1,
                            "unexpected character");
}

static ER_LexerResult lexer_panic(LexerState *lexer) {
    LexerState *snapshot = calloc(1, sizeof(*snapshot));
    assert(snapshot != NULL);
    *snapshot = *lexer;

    return (ER_LexerResult){.res_status       = ER_STATUS_PANIC,
                            .res_panicarg     = snapshot,
                            .res_panichandler = lexer_panic_handler};
}

static ER_Token lexer_new_token(LexerState *lexer, ER_TokenType toktype) {
    ER_Token tok          = {0};
    tok.tok_value.str_buf = lexer->ls_input.str_buf + lexer->ls_next;
    tok.tok_value.str_len = 0;
    tok.tok_off           = lexer->ls_next;
    tok.tok_row           = lexer->ls_row;
    tok.tok_col           = lexer->ls_col;
    tok.tok_rowoff        = lexer->ls_bincol;
    tok.tok_rowstart      = lexer->ls_rowstart;
    tok.tok_type          = toktype;
    return tok;
}

// Handlers

static LexerInstruction lexer_base_handler(ER_WChar c);
static LexerInstruction lexer_comment_handler(ER_WChar c);
static LexerInstruction lexer_comment2_handler(ER_WChar c);
static LexerInstruction lexer_multiline_comment_handler(ER_WChar c);
static LexerInstruction lexer_multiline_comment2_handler(ER_WChar c);
static LexerInstruction lexer_string_handler(ER_WChar c);
static LexerInstruction lexer_keyword_identifier_handler(ER_WChar c);
static LexerInstruction lexer_number_handler(ER_WChar c);

// NOTE: this function is written  assuming that the base state is never entered
// with a half-full token buffer (i.e, LEXER_ACTION_IGNORE is used to switch to
// some states with the assumption that the previous token was already pushed by
// the previous handler )
static LexerInstruction lexer_base_handler(ER_WChar c) {
    // special cases
    switch (c) {
        case ' ':
        case '\n':
        case '\r':
        case '\t':
        case EOF:
            return (LexerInstruction){.li_handler    = lexer_base_handler,
                                      .li_charaction = LEXER_ACTION_DISCARD,
                                      .li_tokaction  = LEXER_ACTION_DISCARD,
                                      .li_toktype    = ER_TOKEN_TYPE_NONE};
        default:
            break;
    }

    // Handle single characters
    ER_TokenType sc_token_type;
    switch (c) {
        case ';':
            sc_token_type = ER_TOKEN_TYPE_SEMICOLON;
            break;
        case '(':
            sc_token_type = ER_TOKEN_TYPE_LPAREN;
            break;
        case ')':
            sc_token_type = ER_TOKEN_TYPE_RPAREN;
            break;
        case '{':
            sc_token_type = ER_TOKEN_TYPE_LBLOCK;
            break;
        case '}':
            sc_token_type = ER_TOKEN_TYPE_RBLOCK;
            break;
        case ',':
            sc_token_type = ER_TOKEN_TYPE_COMMA;
            break;
        default:
            sc_token_type = ER_TOKEN_TYPE_NONE;
            break;
    }

    if (sc_token_type != ER_TOKEN_TYPE_NONE) {
        return (LexerInstruction){.li_handler    = lexer_base_handler,
                                  .li_charaction = LEXER_ACTION_PUSH,
                                  .li_tokaction  = LEXER_ACTION_PUSH,
                                  .li_toktype    = sc_token_type};
    }

    if (c == '/') {
        return (LexerInstruction){.li_handler    = lexer_comment_handler,
                                  .li_charaction = LEXER_ACTION_DISCARD,
                                  .li_tokaction  = LEXER_ACTION_IGNORE,
                                  .li_toktype    = ER_TOKEN_TYPE_NONE};
    }

    if (c == '"') {
        return (LexerInstruction){.li_handler    = lexer_string_handler,
                                  .li_charaction = LEXER_ACTION_DISCARD,
                                  .li_tokaction  = LEXER_ACTION_DISCARD,
                                  .li_toktype    = ER_TOKEN_TYPE_NONE};
    }

    if (lexer_is_keyword_identifier_char(c)) {
        return (LexerInstruction){
            .li_handler    = lexer_keyword_identifier_handler,
            .li_charaction = LEXER_ACTION_PUSH,
            .li_tokaction  = LEXER_ACTION_IGNORE,
            .li_toktype    = ER_TOKEN_TYPE_NONE};
    }

    if (lexer_is_number_char(c)) {
        return (LexerInstruction){.li_handler    = lexer_number_handler,
                                  .li_charaction = LEXER_ACTION_PUSH,
                                  .li_tokaction  = LEXER_ACTION_IGNORE,
                                  .li_toktype    = ER_TOKEN_TYPE_NONE};
    }

    // if all of these fail, we error out
    return (LexerInstruction){.li_handler = NULL};
}

static LexerInstruction lexer_comment_handler(ER_WChar c) {
    // expect second / (//)
    if (c == '/') {
        return (LexerInstruction){.li_handler    = lexer_comment2_handler,
                                  .li_charaction = LEXER_ACTION_DISCARD,
                                  .li_tokaction  = LEXER_ACTION_IGNORE,
                                  .li_toktype    = ER_TOKEN_TYPE_NONE};
    } else if (c == '*') {
        return (LexerInstruction){.li_handler = lexer_multiline_comment_handler,
                                  .li_charaction = LEXER_ACTION_DISCARD,
                                  .li_tokaction  = LEXER_ACTION_IGNORE,
                                  .li_toktype    = ER_TOKEN_TYPE_NONE};
    }

    return (LexerInstruction){.li_handler = NULL};
}

static LexerInstruction lexer_comment2_handler(ER_WChar c) {
    // break the comment if a new line is found
    if (c == '\n') {
        return (LexerInstruction){.li_handler    = lexer_base_handler,
                                  .li_charaction = LEXER_ACTION_DISCARD,
                                  .li_tokaction  = LEXER_ACTION_DISCARD,
                                  .li_toktype    = ER_TOKEN_TYPE_NONE};
    }

    // otherwise keep discarding characters
    return (LexerInstruction){.li_handler    = lexer_comment2_handler,
                              .li_charaction = LEXER_ACTION_DISCARD,
                              .li_tokaction  = LEXER_ACTION_IGNORE,
                              .li_toktype    = ER_TOKEN_TYPE_NONE};
}

static LexerInstruction lexer_multiline_comment_handler(ER_WChar c) {
    // closing multiline comment
    if (c == '*') {
        return (LexerInstruction){
            .li_handler    = lexer_multiline_comment2_handler,
            .li_charaction = LEXER_ACTION_DISCARD,
            .li_tokaction  = LEXER_ACTION_IGNORE,
            .li_toktype    = ER_TOKEN_TYPE_NONE};
    } else if (c == (ER_WChar)EOF) {
        return (LexerInstruction){.li_handler = NULL};
    }

    // otherwise keep discarding characters
    return (LexerInstruction){.li_handler    = lexer_multiline_comment_handler,
                              .li_charaction = LEXER_ACTION_DISCARD,
                              .li_tokaction  = LEXER_ACTION_IGNORE,
                              .li_toktype    = ER_TOKEN_TYPE_NONE};
}

static LexerInstruction lexer_multiline_comment2_handler(ER_WChar c) {
    if (c == '/') {
        return (LexerInstruction){.li_handler    = lexer_base_handler,
                                  .li_charaction = LEXER_ACTION_DISCARD,
                                  .li_tokaction  = LEXER_ACTION_DISCARD,
                                  .li_toktype    = ER_TOKEN_TYPE_NONE};
    } else if (c == (ER_WChar)EOF) {
        return (LexerInstruction){.li_handler = NULL};
    }

    // go back to regular multine comment state
    return (LexerInstruction){.li_handler    = lexer_multiline_comment_handler,
                              .li_charaction = LEXER_ACTION_DISCARD,
                              .li_tokaction  = LEXER_ACTION_IGNORE,
                              .li_toktype    = ER_TOKEN_TYPE_NONE};
}

static LexerInstruction lexer_string_handler(ER_WChar c) {
    // terminate the string (found closing quote)
    if (c == '"') {
        return (LexerInstruction){.li_handler    = lexer_base_handler,
                                  .li_charaction = LEXER_ACTION_DISCARD,
                                  .li_tokaction  = LEXER_ACTION_PUSH,
                                  .li_toktype    = ER_TOKEN_TYPE_STRING};
    }

    if (c == '\n' || c == (ER_WChar)EOF) {
        return (LexerInstruction){.li_handler = NULL};
    }

    // append characters to the string
    return (LexerInstruction){.li_handler    = lexer_string_handler,
                              .li_charaction = LEXER_ACTION_PUSH,
                              .li_tokaction  = LEXER_ACTION_IGNORE,
                              .li_toktype    = ER_TOKEN_TYPE_NONE};
}

static LexerInstruction lexer_keyword_identifier_handler(ER_WChar c) {
    if (lexer_is_keyword_identifier_char(c) || lexer_is_number_char(c)) {
        return (LexerInstruction){
            .li_handler    = lexer_keyword_identifier_handler,
            .li_charaction = LEXER_ACTION_PUSH,
            .li_tokaction  = LEXER_ACTION_IGNORE,
            .li_toktype    = ER_TOKEN_TYPE_NONE};
    }

    // pushing a token with NONE type is a hint to the lexer engine to try
    // assining a token type based on buffer contents
    return (LexerInstruction){.li_handler    = lexer_base_handler,
                              .li_charaction = LEXER_ACTION_IGNORE,
                              .li_tokaction  = LEXER_ACTION_PUSH,
                              .li_toktype    = ER_TOKEN_TYPE_NONE};
}

static LexerInstruction lexer_number_handler(ER_WChar c) {
    if (lexer_is_keyword_identifier_char(c) || lexer_is_number_char(c)) {
        return (LexerInstruction){.li_handler    = lexer_number_handler,
                                  .li_charaction = LEXER_ACTION_PUSH,
                                  .li_tokaction  = LEXER_ACTION_IGNORE,
                                  .li_toktype    = ER_TOKEN_TYPE_NONE};
    }

    return (LexerInstruction){.li_handler    = lexer_base_handler,
                              .li_charaction = LEXER_ACTION_IGNORE,
                              .li_tokaction  = LEXER_ACTION_PUSH,
                              .li_toktype    = ER_TOKEN_TYPE_NUMBER};
}

ER_LexerResult ER_lexer_run(ER_String input) {
    LexerCharHandler *handler = lexer_base_handler;
    LexerState        lexer   = {0};
    lexer.ls_input            = input;
    lexer_reset_row(&lexer);
    lexer.ls_rowstart = 0;

    ER_TokenList tokens            = EZLD_ARRAY_NEW();
    ER_Token     curr_token        = {0};
    LexerAction  curr_token_action = LEXER_ACTION_DISCARD;

    // NOTE: this loop used to be "hera-style", i.e. it used to perform
    // ER_RESULT_OK() checks inside the loop condition and use that to break.
    // This was changed because it didn't distinguish between invalid UTF8
    // characters and EOF correctly
    while (lexer_can_peek(&lexer)) {
        ER_WCharResult fetch_result = lexer_peek(&lexer);
        if (!ER_RESULT_OK(fetch_result)) {
            EZLD_ARRAY_FREE(tokens);
            return lexer_panic(&lexer);
        }

        ER_WChar curr_char       = ER_RESULT_GET(fetch_result);
        ER_u64   curr_char_width = lexer_char_width(&lexer); // see lexer_step
        ER_u64   curr_off        = lexer.ls_next;
        ER_u64   end_off         = curr_off + curr_char_width;
        bool     advance         = true;

        if (curr_char == '\n') {
            lexer_reset_row(&lexer);
        }

        // invariant: curr_token initialization always happens here
        // NOTE: no special case for first iteration or similar
        if (curr_token_action == LEXER_ACTION_DISCARD) {
            curr_token        = lexer_new_token(&lexer, ER_TOKEN_TYPE_NONE);
            curr_token_action = LEXER_ACTION_IGNORE;
        }

        LexerInstruction instruction = handler(curr_char);

        // if the handler threw an error
        if (instruction.li_handler == NULL) {
            EZLD_ARRAY_FREE(tokens);
            return lexer_panic(&lexer);
        }

        // handle ER_WChar action
        switch (instruction.li_charaction) {
            case LEXER_ACTION_DISCARD:
                assert(curr_off != 0 ||
                       instruction.li_tokaction != LEXER_ACTION_PUSH);
                end_off -= curr_char_width;
                break;
            case LEXER_ACTION_IGNORE:
                end_off -= curr_char_width;
                advance = false;
                break;
            case LEXER_ACTION_PUSH:
                break;
        }

        switch (instruction.li_tokaction) {
            case LEXER_ACTION_DISCARD:
                curr_token_action = LEXER_ACTION_DISCARD;
                break;
            case LEXER_ACTION_IGNORE:
                break;
            // invariant: length and type assignments always happen here
            // alongside push
            case LEXER_ACTION_PUSH:
                curr_token.tok_value.str_len = end_off - curr_token.tok_off;
                curr_token.tok_type = lexer_token_type(curr_token.tok_value,
                                                       instruction.li_toktype);
                *EZLD_ARRAY_PUSH(tokens) = curr_token;
                curr_token_action        = LEXER_ACTION_DISCARD;
                break;
        }

        handler = instruction.li_handler;
        if (advance) {
            lexer_step(&lexer);
        }
    }

    // NOTE: we only get here if the loop exited successfully
    ER_Token eof             = lexer_new_token(&lexer, ER_TOKEN_TYPE_EOF);
    *EZLD_ARRAY_PUSH(tokens) = eof;

    return (ER_LexerResult){.res_status = ER_STATUS_OK, .res_val = tokens};
}
