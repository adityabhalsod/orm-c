/* Reference Counting Implementation
 * 
 * Provides automatic memory management through reference counting.
 * Thread-safe using atomic operations.
 */

#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <assert.h>

// Reference counted object header
typedef struct {
    atomic_int ref_count;
    void (*destructor)(void*);
    size_t size;
} ORM_RefHeader;

// Magic number to detect invalid objects
#define REF_MAGIC 0x52454643  // "REFC"

// Get header from object pointer
static inline ORM_RefHeader* get_header(void* obj) {
    if (!obj) {
        return NULL;
    }
    return (ORM_RefHeader*)((char*)obj - sizeof(ORM_RefHeader));
}

// Get object from header pointer
static inline void* get_object(ORM_RefHeader* header) {
    return (void*)((char*)header + sizeof(ORM_RefHeader));
}

void* ORM_AllocRef(size_t size, void (*destructor)(void*)) {
    if (size == 0) {
        return NULL;
    }
    
    // Allocate header + object
    size_t total_size = sizeof(ORM_RefHeader) + size;
    ORM_RefHeader* header = (ORM_RefHeader*)malloc(total_size);
    if (!header) {
        return NULL;
    }
    
    // Initialize header
    atomic_init(&header->ref_count, 1);
    header->destructor = destructor;
    header->size = size;
    
    // Get object pointer and zero initialize
    void* obj = get_object(header);
    memset(obj, 0, size);
    
    return obj;
}

void ORM_Retain(void* obj) {
    if (!obj) {
        return;
    }
    
    ORM_RefHeader* header = get_header(obj);
    
    // Increment ref count atomically
    int old_count = atomic_fetch_add(&header->ref_count, 1);
    
    // Sanity check - ref count should never be 0 when retaining
    assert(old_count > 0 && "Retaining object with zero ref count");
}

void ORM_Release(void* obj) {
    if (!obj) {
        return;
    }
    
    ORM_RefHeader* header = get_header(obj);
    
    // Decrement ref count atomically
    int old_count = atomic_fetch_sub(&header->ref_count, 1);
    
    // Sanity check
    assert(old_count > 0 && "Releasing object with zero ref count");
    
    // If ref count reaches 0, free the object
    if (old_count == 1) {
        // Call destructor if provided
        if (header->destructor) {
            header->destructor(obj);
        }
        
        // Free the memory
        free(header);
    }
}

int ORM_RefCount(void* obj) {
    if (!obj) {
        return -1;
    }
    
    ORM_RefHeader* header = get_header(obj);
    return atomic_load(&header->ref_count);
}
