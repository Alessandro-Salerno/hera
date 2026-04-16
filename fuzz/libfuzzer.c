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

#include <hera/allocator.h>
#include <hera/graph.h>
#include <hera/layout.h>
#include <hera/lexer.h>
#include <hera/parser.h>
#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    ER_memory_init();

    ER_Allocator *allocator = ER_allocator_init();
    ER_String     input     = {.str_buf = (const char *)data, .str_len = size};

    ER_LexerResult lex_res = ER_lexer_run(input, allocator);
    if (!ER_RESULT_OK(lex_res)) {
        goto exit;
    }

    ER_TokenList tokens = ER_RESULT_GET(lex_res);

    ER_ParserResult par_res = ER_parser_run(tokens, allocator);
    if (!ER_RESULT_OK(par_res)) {
        goto exit;
    }

    ER_ASTRootNode ast_root  = ER_RESULT_GET(par_res);
    ER_GraphResult graph_res = ER_graph_compute(&ast_root, allocator);
    if (!ER_RESULT_OK(graph_res)) {
        goto exit;
    }

    ER_Graph graph = ER_RESULT_GET(graph_res);
    ER_layout_place(&graph);

exit:
    ER_memory_deinit();
    return 0;
}
