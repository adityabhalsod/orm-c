/* Arena Allocator Implementation
 * 
 * Fast bump allocator for temporary allocations.
 * All allocations freed at once when arena is destroyed.
 */

#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Arena structure
struct ORM_Arena {
    void* memory;           // Base memory block
    size_t size;            // Total size
    size_t used;            // Bytes used
    struct ORM_Arena* next; // Next arena in chain (for growth)
};

// Align size to 8-byte boundary
static inline size_t align_size(size_t size) {
    return (size + 7) & ~7;
}

ORM_Arena* ORM_CreateArena(size_t size) {
    if (size == 0) {
        size = 4096; // Default 4KB
    }
    
    ORM_Arena* arena = (ORM_Arena*)malloc(sizeof(ORM_Arena));
    if (!arena) {
        return NULL;
    }
    
    arena->memory = malloc(size);
    if (!arena->memory) {
        free(arena);
        return NULL;
    }
    
    arena->size = size;
    arena->used = 0;
    arena->next = NULL;
    
    return arena;
}

void* ORM_ArenaAlloc(ORM_Arena* arena, size_t size) {
    if (!arena || size == 0) {
        return NULL;
    }
    
    // Align allocation
    size = align_size(size);
    
    // Try to allocate from current arena
    if (arena->used + size <= arena->size) {
        void* ptr = (char*)arena->memory + arena->used;
        arena->used += size;
        memset(ptr, 0, size); // Zero initialize
        return ptr;
    }
    
    // Need to allocate new arena in chain
    // Make it large enough for this allocation or 2x original size
    size_t new_size = size > arena->size * 2 ? size : arena->size * 2;
    
    ORM_Arena* new_arena = ORM_CreateArena(new_size);
    if (!new_arena) {
        return NULL;
    }
    
    // Link new arena
    new_arena->next = arena->next;
    arena->next = new_arena;
    
    // Allocate from new arena
    void* ptr = new_arena->memory;
    new_arena->used = size;
    memset(ptr, 0, size);
    return ptr;
}

void ORM_ArenaClear(ORM_Arena* arena) {
    if (!arena) {
        return;
    }
    
    // Reset main arena
    arena->used = 0;
    
    // Destroy chained arenas
    ORM_Arena* current = arena->next;
    while (current) {
        ORM_Arena* next = current->next;
        free(current->memory);
        free(current);
        current = next;
    }
    arena->next = NULL;
}

void ORM_ArenaStats(ORM_Arena* arena, size_t* total, size_t* used) {
    if (!arena) {
        if (total) *total = 0;
        if (used) *used = 0;
        return;
    }
    
    size_t total_size = 0;
    size_t total_used = 0;
    
    // Count main arena
    total_size += arena->size;
    total_used += arena->used;
    
    // Count chained arenas
    ORM_Arena* current = arena->next;
    while (current) {
        total_size += current->size;
        total_used += current->used;
        current = current->next;
    }
    
    if (total) *total = total_size;
    if (used) *used = total_used;
}

void ORM_DestroyArena(ORM_Arena* arena) {
    if (!arena) {
        return;
    }
    
    // Destroy all chained arenas
    ORM_Arena* current = arena->next;
    while (current) {
        ORM_Arena* next = current->next;
        free(current->memory);
        free(current);
        current = next;
    }
    
    // Destroy main arena
    free(arena->memory);
    free(arena);
}
