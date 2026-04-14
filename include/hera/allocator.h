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

#include <bsd/queue.h>
#include <hera/types.h>

typedef struct ER_AllocatorHeader {
    TAILQ_ENTRY(ER_AllocatorHeader) ah_link;
    ER_u64 ah_size;
    ER_u64 ah_next;
    // NOTE: buffer starts here
} ER_AllocatorHeader;

typedef TAILQ_HEAD(ER_AllocatorHeaderList,
                   ER_AllocatorHeader) ER_AllocatorHeaderList;

typedef struct ER_Allocator {
    ER_AllocatorHeaderList a_pools;
    ER_AllocatorHeaderList a_objects;
    TAILQ_ENTRY(ER_Allocator) a_link;
} ER_Allocator;

ER_Allocator *ER_allocator_init(void);
void          ER_allocator_deinit(ER_Allocator *allocator);

void *ER_malloc(ER_Allocator *allocator, ER_u64 size);
void *ER_calloc(ER_Allocator *allocator, ER_u64 n, ER_u64 size);

void *ER_global_malloc(ER_u64 size);
void *ER_global_calloc(ER_u64 n, ER_u64 size);
void *ER_global_realloc(void *buf, ER_u64 new_size);
void  ER_global_free(void *ptr);

void ER_memory_init(void);
void ER_memory_deinit(void);
