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

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define ER_STRING(str)                         \
    (ER_String) {                              \
        .str_buf = str, .str_len = strlen(str) \
    }

#define ER_STRING_SUB(s, at)                                     \
    (ER_String) {                                                \
        .str_buf = (s).str_buf + at, .str_len = (s).str_len - at \
    }

#define ER_STRING_SUP(s, at)                                     \
    (ER_String) {                                                \
        .str_buf = (s).str_buf - at, .str_len = (s).str_len + at \
    }

#define ER_STRING_EQ(s1, s2)         \
    ((s1).str_len == (s2).str_len && \
     memcmp((s1).str_buf, (s2).str_buf, (s1).str_len) == 0)

#define ER_STRING_EQ_LITERAL(s1, literal) \
    ((s1).str_len == strlen(literal) &&   \
     memcmp((s1).str_buf, literal, strlen(literal)) == 0)

#define ER_STRING_PRINTF(s) (int)(s).str_len, (s).str_buf

typedef uint8_t  ER_u8;
typedef uint16_t ER_u16;
typedef uint32_t ER_u32;
typedef uint64_t ER_u64;

typedef int8_t  ER_i8;
typedef int16_t ER_i16;
typedef int32_t ER_i32;
typedef int64_t ER_i64;

typedef ER_u32 ER_WChar;

// For the purposes of this project, having a remote pointer is more effective
// than a flexible array member with __attribute__((counted_by)) or a fat
// pointer
typedef struct ER_String {
    const char *str_buf;
    ER_u64      str_len;
} ER_String;
