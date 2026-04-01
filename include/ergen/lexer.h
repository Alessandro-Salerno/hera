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

#include <ergen/result.h>
#include <ergen/types.h>
#include <ezld/array.h>

typedef enum ER_TokenType {
    ER_TOKEN_TYPE_NONE       = 0,
    ER_TOKEN_TYPE_NUMBER     = 1 << 0,
    ER_TOKEN_TYPE_IDENTIFIER = 1 << 1,
    ER_TOKEN_TYPE_STRING     = 1 << 2,
    ER_TOKEN_TYPE_ENTITY     = 1 << 3,
    ER_TOKEN_TYPE_ATTRIBUTE  = 1 << 4,
    ER_TOKEN_TYPE_RELATION   = 1 << 5,
    ER_TOKEN_TYPE_TOTAL      = 1 << 6,
    ER_TOKEN_TYPE_SPECIFIES  = 1 << 7,
    ER_TOKEN_TYPE_SEMICOLON  = 1 << 8,
    ER_TOKEN_TYPE_KEY        = 1 << 9,
    ER_TOKEN_TYPE_LPAREN     = 1 << 10,
    ER_TOKEN_TYPE_RPAREN     = 1 << 11,
    ER_TOKEN_TYPE_LBLOCK     = 1 << 12,
    ER_TOKEN_TYPE_RBLOCK     = 1 << 13,
    ER_TOKEN_TYPE_COMMA      = 1 << 14,
    ER_TOKEN_TYPE_EOF        = 1 << 15
} ER_TokenType;

typedef struct ER_Token {
    ER_String    tok_value;
    ER_u64       tok_off;
    ER_u64       tok_row;
    ER_u64       tok_col;
    ER_u64       tok_rowstart;
    ER_u64       tok_rowoff; // = tok_col for ASCII, >= for UTF-8
    ER_TokenType tok_type;
} ER_Token;

typedef EZLD_ARRAY(ER_Token) ER_TokenList;
typedef ER_RESULT(ER_TokenList) ER_LexerResult;

ER_LexerResult ER_lexer_run(ER_String input);
