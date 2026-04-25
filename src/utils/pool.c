/* Memory Pool Allocator Implementation
 * 
 * Fast fixed-size block allocator using a free list.
 * Thread-safe with mutex protection.
 */

#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

// Free list node (stored in free blocks)
typedef struct FreeNode {
    struct FreeNode* next;
} FreeNode;

// Memory pool structure
struct ORM_MemPool {
    void* memory;              // Base memory block
    size_t block_size;         // Size of each block
    size_t block_count;        // Total number of blocks
    size_t blocks_used;        // Number of allocated blocks
    FreeNode* free_list;       // Head of free list
    pthread_mutex_t lock;      // Thread safety
};

ORM_MemPool* ORM_CreatePool(size_t block_size, size_t block_count) {
    if (block_size == 0 || block_count == 0) {
        return NULL;
    }
    
    // Ensure block size can hold a free list pointer
    if (block_size < sizeof(FreeNode)) {
        block_size = sizeof(FreeNode);
    }
    
    // Align block size to 8 bytes
    block_size = (block_size + 7) & ~7;
    
    ORM_MemPool* pool = (ORM_MemPool*)malloc(sizeof(ORM_MemPool));
    if (!pool) {
        return NULL;
    }
    
    // Allocate memory for all blocks
    size_t total_size = block_size * block_count;
    pool->memory = malloc(total_size);
    if (!pool->memory) {
        free(pool);
        return NULL;
    }
    
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->blocks_used = 0;
    
    // Initialize mutex
    pthread_mutex_init(&pool->lock, NULL);
    
    // Initialize free list - link all blocks
    pool->free_list = NULL;
    char* block_ptr = (char*)pool->memory;
    
    for (size_t i = 0; i < block_count; i++) {
        FreeNode* node = (FreeNode*)block_ptr;
        node->next = pool->free_list;
        pool->free_list = node;
        block_ptr += block_size;
    }
    
    return pool;
}

void* ORM_PoolAlloc(ORM_MemPool* pool) {
    if (!pool) {
        return NULL;
    }
    
    pthread_mutex_lock(&pool->lock);
    
    // Check if pool has free blocks
    if (!pool->free_list) {
        pthread_mutex_unlock(&pool->lock);
        return NULL;
    }
    
    // Remove block from free list
    FreeNode* block = pool->free_list;
    pool->free_list = block->next;
    pool->blocks_used++;
    
    pthread_mutex_unlock(&pool->lock);
    
    // Zero out the block
    memset(block, 0, pool->block_size);
    
    return (void*)block;
}

void ORM_PoolFree(ORM_MemPool* pool, void* ptr) {
    if (!pool || !ptr) {
        return;
    }
    
    // Verify pointer is within pool bounds
    char* block_ptr = (char*)ptr;
    char* pool_start = (char*)pool->memory;
    char* pool_end = pool_start + (pool->block_size * pool->block_count);
    
    if (block_ptr < pool_start || block_ptr >= pool_end) {
        // Pointer not from this pool
        return;
    }
    
    // Verify pointer is aligned to block boundary
    size_t offset = block_ptr - pool_start;
    if (offset % pool->block_size != 0) {
        // Not a valid block start
        return;
    }
    
    pthread_mutex_lock(&pool->lock);
    
    // Add block back to free list
    FreeNode* node = (FreeNode*)ptr;
    node->next = pool->free_list;
    pool->free_list = node;
    pool->blocks_used--;
    
    pthread_mutex_unlock(&pool->lock);
}

void ORM_PoolStats(ORM_MemPool* pool, size_t* total, size_t* used, size_t* free) {
    if (!pool) {
        if (total) *total = 0;
        if (used) *used = 0;
        if (free) *free = 0;
        return;
    }
    
    pthread_mutex_lock(&pool->lock);
    
    if (total) *total = pool->block_count;
    if (used) *used = pool->blocks_used;
    if (free) *free = pool->block_count - pool->blocks_used;
    
    pthread_mutex_unlock(&pool->lock);
}

void ORM_DestroyPool(ORM_MemPool* pool) {
    if (!pool) {
        return;
    }
    
    pthread_mutex_destroy(&pool->lock);
    free(pool->memory);
    free(pool);
}
