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

#include <ergen/util.h>
#include <stdarg.h>
#include <stdio.h>

#define COLOR_RED   "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_CYAN  "\x1b[36m"
#define COLOR_BOLD  "\x1b[1m"
#define COLOR_RESET "\x1b[0m"

void ER_generic_panic_output(ER_String   input,
                             ER_u64      err_line,
                             ER_u64      err_col,
                             ER_u64      err_lstart,
                             ER_u64      err_loff,
                             ER_u64      err_len,
                             const char *fmt,
                             ...) {
    const char *buf = input.str_buf;
    ER_u64      len = input.str_len;

    // --- Find end of the line ---
    ER_u64 line_end = err_lstart;
    while (line_end < len && '\n' != buf[line_end]) {
        line_end++;
    }

    ER_u64 line_len = line_end - err_lstart;

    // Clamp error region inside the line
    ER_u64 underline_start = ER_MIN(err_loff - 1, line_len);
    ER_u64 underline_len   = ER_MIN(err_len, line_len - underline_start);

    // --- Location ---
    fprintf(stderr,
            COLOR_CYAN "==> line %lu, column %lu: " COLOR_RESET,
            err_line,
            err_col);

    // Header
    fprintf(stderr, COLOR_BOLD COLOR_RED "error: " COLOR_RESET);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");

    // Code line
    fprintf(stderr, " %4lu | ", err_line);
    fwrite(buf + err_lstart, 1, line_len, stderr);
    fprintf(stderr, "\n");

    // Underline
    fprintf(stderr, "      | ");

    // spacing before caret
    for (ER_u64 i = 0; i < underline_start; i++) {
        char c = buf[err_lstart + i];
        fputc(c == '\t' ? '\t' : ' ', stderr);
    }

    fprintf(stderr, COLOR_GREEN);

    if (0 == underline_len) {
        fputc('^', stderr);
    } else {
        fputc('^', stderr);
        for (ER_u64 i = 1; i < underline_len; i++) {
            fputc('~', stderr);
        }
    }

    fprintf(stderr, COLOR_RESET);
}
