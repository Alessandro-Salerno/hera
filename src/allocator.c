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
#include <hera/allocator.h>
#include <hera/util.h>
#include <stdalign.h>
#include <stdlib.h>

#define INITIAL_POOL_SIZE  (16 * 1024)
#define POOL_GROWTH_FACTOR 2
#define MAX_OBJECT_SIZE    4096
#define HEADER_PADDING \
    ER_ROUND_UP(sizeof(ER_AllocatorHeader), sizeof(max_align_t))

// NOTE: globals are fine because the application is single-threaded and these
// are only accessible within this translation unit
static ER_Allocator *s_global_allocator;
static TAILQ_HEAD(, ER_Allocator) s_allocators;

static inline ER_u64 allocator_round_size(ER_u64 size) {
    return ER_ROUND_UP(size, sizeof(max_align_t));
}

static inline ER_AllocatorHeader *
allocator_current_pool(ER_Allocator *allocator) {
    return TAILQ_LAST(&allocator->a_pools, ER_AllocatorHeaderList);
}

static ER_AllocatorHeader *allocator_new_pool(ER_Allocator *allocator) {
    ER_u64 pool_size = INITIAL_POOL_SIZE;

    if (!TAILQ_EMPTY(&allocator->a_pools)) {
        ER_AllocatorHeader *last_pool = allocator_current_pool(allocator);
        pool_size                     = last_pool->ah_size * POOL_GROWTH_FACTOR;
    }

    ER_AllocatorHeader *new_pool = calloc(1,
                                          sizeof(*new_pool) + HEADER_PADDING +
                                              pool_size);
    assert(new_pool != NULL);
    new_pool->ah_size = pool_size;
    TAILQ_INSERT_TAIL(&allocator->a_pools, new_pool, ah_link);

    return new_pool;
}

// NOTE: we use unsigned char * because arithmetic on void * is undefined
// behavior
// NOTE: this kind of magic is necessary because alignas() does not guarantee
// size, it only guarantees alignment in *some* cases. Since the buffer is the
// one we return to the caller, we must guaranttee that it be aligned to
// sizeof(max_align_). Since we can assume that the pointer returned by
// malloc/calloc for the pool itself is aligned, we just need to add
// sizeof(ER_AllocatorHeader) by adding +1 and then add the remainder separately
static inline void *allocator_get_header_buffer(ER_AllocatorHeader *pool) {
    return (unsigned char *)(pool + 1) + HEADER_PADDING;
}

static inline ER_AllocatorHeader *allocator_get_header_from_buffer(void *buf) {
    return (ER_AllocatorHeader *)((unsigned char *)buf - HEADER_PADDING) - 1;
}

// NOTE: regions allocated by this function shall not be used as actual pools,
// but rather as mere allocation headers for each object. As such, their size is
// equivilent to that of the containing object, and they shall be part of
// ER_Allocator.a_objects and not a_pools
static void *allocator_allocate_object(ER_Allocator *allocator, ER_u64 size) {
    ER_AllocatorHeader *object_header = calloc(1,
                                               sizeof(*object_header) +
                                                   HEADER_PADDING + size);
    assert(object_header != NULL);
    object_header->ah_size = size;
    TAILQ_INSERT_TAIL(&allocator->a_objects, object_header, ah_link);
    return allocator_get_header_buffer(object_header);
}

ER_Allocator *ER_allocator_init(void) {
    ER_Allocator *allocator = calloc(1, sizeof(*allocator));
    assert(allocator != NULL);
    TAILQ_INIT(&allocator->a_pools);
    TAILQ_INIT(&allocator->a_objects);
    TAILQ_INSERT_TAIL(&s_allocators, allocator, a_link);
    return allocator;
}

void ER_allocator_deinit(ER_Allocator *allocator) {
    ER_AllocatorHeader *curr, *_;

    TAILQ_FOREACH_SAFE(curr, &allocator->a_pools, ah_link, _) {
        TAILQ_REMOVE(&allocator->a_pools, curr, ah_link);
        free(curr);
    }

    TAILQ_FOREACH_SAFE(curr, &allocator->a_objects, ah_link, _) {
        TAILQ_REMOVE(&allocator->a_objects, curr, ah_link);
        free(curr);
    }

    TAILQ_REMOVE(&s_allocators, allocator, a_link);
    free(allocator);
}

void *ER_malloc(ER_Allocator *allocator, ER_u64 size) {
    ER_u64 rounded_size = allocator_round_size(size);

    if (rounded_size > MAX_OBJECT_SIZE) {
        return allocator_allocate_object(allocator, size);
    }

    if (!TAILQ_EMPTY(&allocator->a_pools)) {
        ER_AllocatorHeader *pool = allocator_current_pool(allocator);
        if (pool->ah_size - pool->ah_next >= rounded_size) {
            void *ret = allocator_get_header_buffer(pool) + pool->ah_next;
            pool->ah_next += rounded_size;
            return ret;
        }
    }

    ER_AllocatorHeader *new_pool = allocator_new_pool(allocator);
    return allocator_get_header_buffer(new_pool);
}

// TODO: perform overflow check
void *ER_calloc(ER_Allocator *allocator, ER_u64 n, ER_u64 size) {
    return ER_malloc(allocator, n * size);
}

void *ER_global_malloc(ER_u64 size) {
    return allocator_allocate_object(s_global_allocator, size);
}

// TODO: perform overflow check (same as above)
void *ER_global_calloc(ER_u64 n, ER_u64 size) {
    return ER_global_malloc(n * size);
}

void *ER_global_realloc(void *buf, ER_u64 new_size) {
    if (buf == NULL) {
        return ER_global_malloc(new_size);
    }

    ER_AllocatorHeader *header = allocator_get_header_from_buffer(buf);
    TAILQ_REMOVE(&s_global_allocator->a_objects, header, ah_link);
    // NOTE: we can reallocate now because the old pool was removed from the
    // object list
    header = realloc(header, sizeof(*header) + HEADER_PADDING + new_size);
    assert(header != NULL);
    header->ah_size = new_size;
    TAILQ_INSERT_TAIL(&s_global_allocator->a_objects, header, ah_link);

    return allocator_get_header_buffer(header);
}

void ER_global_free(void *ptr) {
    if (ptr != NULL) {
        ER_AllocatorHeader *pool = allocator_get_header_from_buffer(ptr);
        TAILQ_REMOVE(&s_global_allocator->a_objects, pool, ah_link);
        free(pool);
    }
}

void ER_memory_init(void) {
    TAILQ_INIT(&s_allocators);
    s_global_allocator = ER_allocator_init();
    atexit(ER_memory_deinit);
}

// NOTE: atexit() is used on this so to guarantee that all memory allocated
// through this layer is freed. This is especially useful for libfuzzer runs
void ER_memory_deinit(void) {
    ER_Allocator *curr, *_;
    TAILQ_FOREACH_SAFE(curr, &s_allocators, a_link, _) {
        ER_allocator_deinit(curr);
    }
}
