/* Memory Management Core Implementation
 * 
 * Provides debug memory tracking and string buffer utilities.
 */

#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>

// ============================================================================
// Debug Memory Tracking
// ============================================================================

#ifdef ORM_DEBUG_MEMORY

#define MAX_ALLOCATIONS 10000

typedef struct {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    time_t timestamp;
} AllocEntry;

static struct {
    AllocEntry entries[MAX_ALLOCATIONS];
    size_t count;
    size_t total_allocations;
    size_t total_frees;
    size_t current_bytes;
    size_t peak_bytes;
    pthread_mutex_t lock;
    bool enabled;
} memory_tracker = {.enabled = false};

void ORM_EnableMemoryTracking(void) {
    if (!memory_tracker.enabled) {
        memset(&memory_tracker, 0, sizeof(memory_tracker));
        pthread_mutex_init(&memory_tracker.lock, NULL);
        memory_tracker.enabled = true;
    }
}

void ORM_DisableMemoryTracking(void) {
    if (memory_tracker.enabled) {
        pthread_mutex_destroy(&memory_tracker.lock);
        memory_tracker.enabled = false;
    }
}

void* ORM_DebugAlloc(size_t size, const char* file, int line) {
    void* ptr = malloc(size);
    if (!ptr) {
        return NULL;
    }
    
    if (memory_tracker.enabled) {
        pthread_mutex_lock(&memory_tracker.lock);
        
        // Record allocation
        if (memory_tracker.count < MAX_ALLOCATIONS) {
            AllocEntry* entry = &memory_tracker.entries[memory_tracker.count++];
            entry->ptr = ptr;
            entry->size = size;
            entry->file = file;
            entry->line = line;
            entry->timestamp = time(NULL);
        }
        
        memory_tracker.total_allocations++;
        memory_tracker.current_bytes += size;
        if (memory_tracker.current_bytes > memory_tracker.peak_bytes) {
            memory_tracker.peak_bytes = memory_tracker.current_bytes;
        }
        
        pthread_mutex_unlock(&memory_tracker.lock);
    }
    
    return ptr;
}

void ORM_DebugFree(void* ptr) {
    if (!ptr) {
        return;
    }
    
    if (memory_tracker.enabled) {
        pthread_mutex_lock(&memory_tracker.lock);
        
        // Find and remove allocation record
        for (size_t i = 0; i < memory_tracker.count; i++) {
            if (memory_tracker.entries[i].ptr == ptr) {
                size_t size = memory_tracker.entries[i].size;
                
                // Remove entry by swapping with last
                memory_tracker.entries[i] = memory_tracker.entries[memory_tracker.count - 1];
                memory_tracker.count--;
                
                memory_tracker.total_frees++;
                memory_tracker.current_bytes -= size;
                break;
            }
        }
        
        pthread_mutex_unlock(&memory_tracker.lock);
    }
    
    free(ptr);
}

void ORM_GetMemoryReport(ORM_MemoryReport* report) {
    if (!report) {
        return;
    }
    
    memset(report, 0, sizeof(ORM_MemoryReport));
    
    if (memory_tracker.enabled) {
        pthread_mutex_lock(&memory_tracker.lock);
        
        report->total_allocations = memory_tracker.total_allocations;
        report->total_frees = memory_tracker.total_frees;
        report->current_allocations = memory_tracker.count;
        report->current_bytes = memory_tracker.current_bytes;
        report->peak_bytes = memory_tracker.peak_bytes;
        report->total_leaked = memory_tracker.current_bytes;
        
        pthread_mutex_unlock(&memory_tracker.lock);
    }
}

void ORM_PrintMemoryReport(void) {
    if (!memory_tracker.enabled) {
        printf("Memory tracking is not enabled\n");
        return;
    }
    
    ORM_MemoryReport report;
    ORM_GetMemoryReport(&report);
    
    printf("\n");
    printf("=== Memory Report ===\n");
    printf("Total allocations: %zu\n", report.total_allocations);
    printf("Total frees:       %zu\n", report.total_frees);
    printf("Current allocs:    %zu\n", report.current_allocations);
    printf("Current bytes:     %zu\n", report.current_bytes);
    printf("Peak bytes:        %zu\n", report.peak_bytes);
    printf("Leaked bytes:      %zu\n", report.total_leaked);
    printf("====================\n\n");
}

void ORM_PrintAllocations(void) {
    if (!memory_tracker.enabled) {
        printf("Memory tracking is not enabled\n");
        return;
    }
    
    pthread_mutex_lock(&memory_tracker.lock);
    
    printf("\n");
    printf("=== Current Allocations ===\n");
    printf("Count: %zu\n\n", memory_tracker.count);
    
    for (size_t i = 0; i < memory_tracker.count; i++) {
        AllocEntry* entry = &memory_tracker.entries[i];
        printf("[%zu] %p - %zu bytes - %s:%d\n",
               i, entry->ptr, entry->size, entry->file, entry->line);
    }
    
    printf("===========================\n\n");
    
    pthread_mutex_unlock(&memory_tracker.lock);
}

#endif

// ============================================================================
// Auto-Free Cleanup
// ============================================================================

#if defined(__GNUC__) || defined(__clang__)
void ORM_AutoFree_(void* ptr) {
    void** p = (void**)ptr;
    if (p && *p) {
        ORM_Free(*p);
        *p = NULL;
    }
}
#endif

// ============================================================================
// String Buffer
// ============================================================================

ORM_StringBuffer* ORM_StringBufferNew(size_t initial_size) {
    if (initial_size == 0) {
        initial_size = 256; // Default size
    }
    
    ORM_StringBuffer* sb = (ORM_StringBuffer*)ORM_Alloc(sizeof(ORM_StringBuffer));
    if (!sb) {
        return NULL;
    }
    
    sb->data = (char*)ORM_Alloc(initial_size);
    if (!sb->data) {
        ORM_Free(sb);
        return NULL;
    }
    
    sb->data[0] = '\0';
    sb->length = 0;
    sb->capacity = initial_size;
    
    return sb;
}

static int ensure_capacity(ORM_StringBuffer* sb, size_t needed) {
    if (sb->length + needed < sb->capacity) {
        return 0; // Enough space
    }
    
    // Grow by 2x or exact amount needed, whichever is larger
    size_t new_capacity = sb->capacity * 2;
    if (new_capacity < sb->length + needed) {
        new_capacity = sb->length + needed + 1;
    }
    
    char* new_data = (char*)realloc(sb->data, new_capacity);
    if (!new_data) {
        return -1;
    }
    
    sb->data = new_data;
    sb->capacity = new_capacity;
    return 0;
}

int ORM_StringBufferAppend(ORM_StringBuffer* sb, const char* str) {
    if (!sb || !str) {
        return -1;
    }
    
    size_t str_len = strlen(str);
    if (str_len == 0) {
        return 0;
    }
    
    if (ensure_capacity(sb, str_len + 1) != 0) {
        return -1;
    }
    
    memcpy(sb->data + sb->length, str, str_len);
    sb->length += str_len;
    sb->data[sb->length] = '\0';
    
    return 0;
}

int ORM_StringBufferAppendF(ORM_StringBuffer* sb, const char* format, ...) {
    if (!sb || !format) {
        return -1;
    }
    
    va_list args;
    va_start(args, format);
    
    // First, try to format into current buffer
    size_t available = sb->capacity - sb->length;
    int needed = vsnprintf(sb->data + sb->length, available, format, args);
    va_end(args);
    
    if (needed < 0) {
        return -1;
    }
    
    // If it didn't fit, grow buffer and try again
    if ((size_t)needed >= available) {
        if (ensure_capacity(sb, needed + 1) != 0) {
            return -1;
        }
        
        va_start(args, format);
        needed = vsnprintf(sb->data + sb->length, sb->capacity - sb->length, format, args);
        va_end(args);
        
        if (needed < 0) {
            return -1;
        }
    }
    
    sb->length += needed;
    return 0;
}

char* ORM_StringBufferRelease(ORM_StringBuffer* sb) {
    if (!sb) {
        return NULL;
    }
    
    char* data = sb->data;
    sb->data = NULL;
    sb->length = 0;
    sb->capacity = 0;
    
    ORM_Free(sb);
    return data;
}

const char* ORM_StringBufferGet(ORM_StringBuffer* sb) {
    if (!sb) {
        return NULL;
    }
    return sb->data;
}

void ORM_StringBufferClear(ORM_StringBuffer* sb) {
    if (!sb) {
        return;
    }
    
    if (sb->data) {
        sb->data[0] = '\0';
    }
    sb->length = 0;
}

void ORM_StringBufferFree(ORM_StringBuffer* sb) {
    if (!sb) {
        return;
    }
    
    if (sb->data) {
        ORM_Free(sb->data);
    }
    ORM_Free(sb);
}
