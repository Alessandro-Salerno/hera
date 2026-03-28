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
#include <ergen/lexer.h>
#include <ergen/parser.h>
#include <stdio.h>

int main(int argc, const char *const argv[]) {
    if (2 != argc) {
        fprintf(stderr,
                "fatal error: incorrect number of arguments\tusage: %s <source "
                "file>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    const char *source_path = argv[1];
    FILE       *source_file = fopen(source_path, "r");

    if (NULL == source_file) {
        fprintf(stderr, "fatal error: unable to open file '%s'\n", source_path);
        return EXIT_FAILURE;
    }

    fseek(source_file, 0, SEEK_END);
    long source_len = ftell(source_file);
    rewind(source_file);

    char *source_buf = malloc(source_len);
    assert(NULL != source_buf);
    fread(source_buf, 1, source_len, source_file);

    ER_String s = (ER_String){.str_buf = source_buf, .str_len = source_len};
    ER_LexerResult lex_res = er_lexer_run(s);
    ER_TokenList   tokens  = *(ER_TokenList *)ER_RESULT_UNWRAP(lex_res);

    /*for (ER_u64 i = 0; i < EZLD_ARRAY_LENGTH(tokens); i++) {
        ER_Token *token = EZLD_ARRAY_AT(tokens, i);
        printf("%zu token(%.*s, %#x)\n",
               i,
               ER_STRING_PRINTF(token->tok_value),
               token->tok_type);
    }*/

    ER_ParserResult par_res  = er_parser_run(tokens);
    ER_ASTRootNode *ast_root = ER_RESULT_UNWRAP(par_res);

    ER_GraphResult graph_res = er_graph_compute(ast_root);
    ER_Graph      *graph     = ER_RESULT_UNWRAP(graph_res);

    return EXIT_SUCCESS;
}
