/*
 * ============================================================================
 * MEMORY.H - Bellek Yönetimi
 * ============================================================================
 */

#ifndef _MEMORY_H
#define _MEMORY_H

#include "types.h"

/* Bellek bloğu bilgisi */
typedef struct memory_block {
    size_t size;
    bool free;
    struct memory_block* next;
    struct memory_block* prev;
} memory_block;

/* Bellek alanı bilgisi */
typedef struct {
    void* start;
    size_t total_size;
    memory_block* first_block;
} memory_arena;

/* Bellek yönetimi fonksiyonları */
void mem_init(memory_arena* arena, void* start, size_t size);
void* kmalloc(memory_arena* arena, size_t size);
void kfree(memory_arena* arena, void* ptr);
void* krealloc(memory_arena* arena, void* ptr, size_t new_size);
void* kcalloc(memory_arena* arena, size_t nmemb, size_t size);

/* Bilgi fonksiyonları */
size_t mem_free_space(memory_arena* arena);
size_t mem_used_space(memory_arena* arena);

#endif /* _MEMORY_H */