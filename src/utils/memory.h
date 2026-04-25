/* ORM Memory Management System
 * 
 * Provides custom memory allocators for optimal performance and zero leaks:
 * - Memory pools for fixed-size allocations
 * - Arena allocators for temporary allocations
 * - Reference counting for shared objects
 * - Debug memory tracking
 */

#ifndef ORM_MEMORY_H
#define ORM_MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// ============================================================================
// Memory Pool Allocator
// ============================================================================

typedef struct ORM_MemPool ORM_MemPool;

/* Create a memory pool with fixed block size
 * block_size: Size of each block in bytes
 * block_count: Number of blocks to pre-allocate
 * Returns: Pool pointer or NULL on failure
 */
ORM_MemPool* ORM_CreatePool(size_t block_size, size_t block_count);

/* Allocate a block from the pool
 * pool: Pool to allocate from
 * Returns: Pointer to block or NULL if pool is full
 */
void* ORM_PoolAlloc(ORM_MemPool* pool);

/* Free a block back to the pool
 * pool: Pool to return block to
 * ptr: Block to free (must be from this pool)
 */
void ORM_PoolFree(ORM_MemPool* pool, void* ptr);

/* Get pool statistics
 * pool: Pool to query
 * total: Output for total blocks
 * used: Output for used blocks
 * free: Output for free blocks
 */
void ORM_PoolStats(ORM_MemPool* pool, size_t* total, size_t* used, size_t* free);

/* Destroy pool and free all memory
 * pool: Pool to destroy
 */
void ORM_DestroyPool(ORM_MemPool* pool);

// ============================================================================
// Arena Allocator
// ============================================================================

typedef struct ORM_Arena ORM_Arena;

/* Create an arena allocator
 * size: Initial size in bytes
 * Returns: Arena pointer or NULL on failure
 */
ORM_Arena* ORM_CreateArena(size_t size);

/* Allocate from arena (aligned to 8 bytes)
 * arena: Arena to allocate from
 * size: Number of bytes to allocate
 * Returns: Pointer to memory or NULL on failure
 */
void* ORM_ArenaAlloc(ORM_Arena* arena, size_t size);

/* Reset arena to initial state (keeps allocated memory)
 * arena: Arena to clear
 */
void ORM_ArenaClear(ORM_Arena* arena);

/* Get arena statistics
 * arena: Arena to query
 * total: Output for total capacity
 * used: Output for used bytes
 */
void ORM_ArenaStats(ORM_Arena* arena, size_t* total, size_t* used);

/* Destroy arena and free all memory
 * arena: Arena to destroy
 */
void ORM_DestroyArena(ORM_Arena* arena);

// ============================================================================
// Reference Counting
// ============================================================================

/* Allocate reference-counted object
 * size: Size of object in bytes (excluding header)
 * destructor: Optional cleanup function called when ref count reaches 0
 * Returns: Pointer to object or NULL on failure
 */
void* ORM_AllocRef(size_t size, void (*destructor)(void*));

/* Increment reference count
 * obj: Reference-counted object
 */
void ORM_Retain(void* obj);

/* Decrement reference count (frees if reaches 0)
 * obj: Reference-counted object
 */
void ORM_Release(void* obj);

/* Get current reference count
 * obj: Reference-counted object
 * Returns: Current ref count or -1 if invalid
 */
int ORM_RefCount(void* obj);

// ============================================================================
// Memory Tracking (Debug Mode)
// ============================================================================

#ifdef ORM_DEBUG_MEMORY

typedef struct {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    time_t timestamp;
} ORM_AllocInfo;

typedef struct {
    size_t total_allocations;
    size_t total_frees;
    size_t current_allocations;
    size_t current_bytes;
    size_t peak_bytes;
    size_t total_leaked;
} ORM_MemoryReport;

/* Enable memory tracking */
void ORM_EnableMemoryTracking(void);

/* Disable memory tracking */
void ORM_DisableMemoryTracking(void);

/* Debug allocation with file/line tracking */
void* ORM_DebugAlloc(size_t size, const char* file, int line);

/* Debug free with tracking */
void ORM_DebugFree(void* ptr);

/* Get memory report */
void ORM_GetMemoryReport(ORM_MemoryReport* report);

/* Print memory report to stdout */
void ORM_PrintMemoryReport(void);

/* Print all current allocations */
void ORM_PrintAllocations(void);

#define ORM_Alloc(size) ORM_DebugAlloc(size, __FILE__, __LINE__)
#define ORM_Free(ptr) ORM_DebugFree(ptr)

#else

#include <stdlib.h>
#define ORM_Alloc(size) malloc(size)
#define ORM_Free(ptr) free(ptr)

#endif

// ============================================================================
// String Buffer
// ============================================================================

typedef struct {
    char* data;
    size_t length;
    size_t capacity;
} ORM_StringBuffer;

/* Create string buffer with initial capacity
 * initial_size: Initial capacity in bytes
 * Returns: Buffer pointer or NULL on failure
 */
ORM_StringBuffer* ORM_StringBufferNew(size_t initial_size);

/* Append string to buffer (grows if needed)
 * sb: String buffer
 * str: String to append
 * Returns: 0 on success, -1 on failure
 */
int ORM_StringBufferAppend(ORM_StringBuffer* sb, const char* str);

/* Append formatted string to buffer
 * sb: String buffer
 * format: Printf-style format string
 * Returns: 0 on success, -1 on failure
 */
int ORM_StringBufferAppendF(ORM_StringBuffer* sb, const char* format, ...);

/* Release ownership of string data (caller must free)
 * sb: String buffer
 * Returns: String data (must be freed with ORM_Free)
 */
char* ORM_StringBufferRelease(ORM_StringBuffer* sb);

/* Get current string (still owned by buffer)
 * sb: String buffer
 * Returns: Current string (do not free)
 */
const char* ORM_StringBufferGet(ORM_StringBuffer* sb);

/* Clear buffer content (keeps allocated memory)
 * sb: String buffer
 */
void ORM_StringBufferClear(ORM_StringBuffer* sb);

/* Destroy buffer and free all memory
 * sb: String buffer
 */
void ORM_StringBufferFree(ORM_StringBuffer* sb);

// ============================================================================
// Utility Macros
// ============================================================================

/* Automatic cleanup (GCC/Clang only) */
#if defined(__GNUC__) || defined(__clang__)
#define ORM_AUTO_FREE __attribute__((cleanup(ORM_AutoFree_)))
void ORM_AutoFree_(void* ptr);
#else
#define ORM_AUTO_FREE
#endif

/* Safe allocation with zero initialization */
#define ORM_NEW(type) ((type*)ORM_Alloc(sizeof(type)))
#define ORM_NEW_ARRAY(type, count) ((type*)calloc((count), sizeof(type)))

#endif /* ORM_MEMORY_H */
