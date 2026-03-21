// MIT License
//
// Copyright (c) 2025 - 2026 Alessandro Salerno
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Taken from:
// https://github.com/Alessandro-Salerno/ezld/blob/main/include/ezld/array.h
// License: MIT
// Author: Alessandro Salerno
// Changes: capitalized macros and added utilities

#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#define EZLD_ARRAY(type) \
    struct {             \
        type  *buf;      \
        size_t len;      \
        size_t cap;      \
    }

#define EZLD_ARRAY_NEW()     {.buf = NULL, .len = 0, .cap = 0}
#define EZLD_ARRAY_INIT(arr) arr.buf = NULL, arr.len = 0, arr.cap = 0
#define EZLD_ARRAY_ALLOC(arr, size) \
    (arr).cap = (size), (arr).buf = malloc(sizeof(*(arr).buf) * size)
#define EZLD_ARRAY_PUSH(arr)              \
    (ezld_array_grow((void **)&(arr).buf, \
                     &(arr).len,          \
                     &(arr).cap,          \
                     sizeof(*(arr).buf)), \
     &((arr).buf[(arr).len - 1]))
#define EZLD_ARRAY_FREE(arr) \
    free((arr).buf), (arr).buf = NULL, (arr).len = 0, (arr).cap = 0
#define EZLD_CONTAINER_PUSH(cont) ezld_array_push((cont).arr)
#define EZLD_ARRAY_FIRST(arr)     ((arr).buf[0])
#define EZLD_ARRAY_LAST(arr)      ((arr).buf[(arr).len - 1])
#define EZLD_ARRAY_IS_EMPTY(arr)  (0 == (arr).len)
#define EZLD_ARRAY_LENGTH(arr)    ((arr).len)
#define EZLD_ARRAY_AT(arr, i)     (&(arr).buf[i])

static inline size_t
ezld_array_grow(void **buf, size_t *len, size_t *cap, size_t elemsz) {
    if (0 == *cap) {
        *cap = 8;
        *buf = calloc(elemsz, *cap);
    } else if (*len == *cap) {
        *cap *= 2;
        *buf = realloc(*buf, elemsz * *cap);
    }
    assert(NULL != *buf);
    (*len)++;
    return (*len) - 1;
}
