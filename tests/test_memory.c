/* Memory Management System Tests
 * 
 * Comprehensive tests for pools, arenas, reference counting, and string buffers.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/utils/memory.h"

// Test counter
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Color codes
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_RESET "\033[0m"

// Test macros
#define TEST_START(name) \
    do { \
        printf("\n[TEST] %s\n", name); \
        tests_run++; \
    } while(0)

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf(COLOR_RED "  ✗ FAIL: %s" COLOR_RESET "\n", message); \
            tests_failed++; \
            return; \
        } else { \
            printf(COLOR_GREEN "  ✓ PASS: %s" COLOR_RESET "\n", message); \
        } \
    } while(0)

#define TEST_END() \
    do { \
        tests_passed++; \
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n"); \
    } while(0)

// ============================================================================
// Memory Pool Tests
// ============================================================================

void test_pool_creation(void) {
    TEST_START("Pool Creation");
    
    ORM_MemPool* pool = ORM_CreatePool(64, 100);
    TEST_ASSERT(pool != NULL, "Pool created successfully");
    
    size_t total, used, free;
    ORM_PoolStats(pool, &total, &used, &free);
    TEST_ASSERT(total == 100, "Total blocks is 100");
    TEST_ASSERT(used == 0, "Used blocks is 0");
    TEST_ASSERT(free == 100, "Free blocks is 100");
    
    ORM_DestroyPool(pool);
    TEST_END();
}

void test_pool_allocation(void) {
    TEST_START("Pool Allocation");
    
    ORM_MemPool* pool = ORM_CreatePool(64, 10);
    
    // Allocate all blocks
    void* blocks[10];
    for (int i = 0; i < 10; i++) {
        blocks[i] = ORM_PoolAlloc(pool);
        TEST_ASSERT(blocks[i] != NULL, "Block allocated");
    }
    
    size_t total, used, free;
    ORM_PoolStats(pool, &total, &used, &free);
    TEST_ASSERT(used == 10, "All blocks used");
    TEST_ASSERT(free == 0, "No free blocks");
    
    // Pool should be full
    void* extra = ORM_PoolAlloc(pool);
    TEST_ASSERT(extra == NULL, "Pool is full");
    
    ORM_DestroyPool(pool);
    TEST_END();
}

void test_pool_reuse(void) {
    TEST_START("Pool Block Reuse");
    
    ORM_MemPool* pool = ORM_CreatePool(64, 10);
    
    void* block1 = ORM_PoolAlloc(pool);
    void* block2 __attribute__((unused)) = ORM_PoolAlloc(pool);
    
    // Free first block
    ORM_PoolFree(pool, block1);
    
    size_t total, used, free;
    ORM_PoolStats(pool, &total, &used, &free);
    TEST_ASSERT(used == 1, "One block used");
    TEST_ASSERT(free == 9, "Nine blocks free");
    
    // Allocate again - should reuse freed block
    void* block3 = ORM_PoolAlloc(pool);
    TEST_ASSERT(block3 == block1, "Block reused");
    
    ORM_DestroyPool(pool);
    TEST_END();
}

// ============================================================================
// Arena Allocator Tests
// ============================================================================

void test_arena_creation(void) {
    TEST_START("Arena Creation");
    
    ORM_Arena* arena = ORM_CreateArena(1024);
    TEST_ASSERT(arena != NULL, "Arena created successfully");
    
    size_t total, used;
    ORM_ArenaStats(arena, &total, &used);
    TEST_ASSERT(total == 1024, "Arena size is 1024");
    TEST_ASSERT(used == 0, "No bytes used");
    
    ORM_DestroyArena(arena);
    TEST_END();
}

void test_arena_allocation(void) {
    TEST_START("Arena Allocation");
    
    ORM_Arena* arena = ORM_CreateArena(1024);
    
    void* p1 = ORM_ArenaAlloc(arena, 100);
    TEST_ASSERT(p1 != NULL, "First allocation succeeds");
    
    void* p2 = ORM_ArenaAlloc(arena, 200);
    TEST_ASSERT(p2 != NULL, "Second allocation succeeds");
    
    void* p3 = ORM_ArenaAlloc(arena, 300);
    TEST_ASSERT(p3 != NULL, "Third allocation succeeds");
    
    size_t total, used;
    ORM_ArenaStats(arena, &total, &used);
    TEST_ASSERT(used > 0, "Arena has used bytes");
    TEST_ASSERT(used <= 608, "Used bytes aligned (100+200+300+padding)");
    
    ORM_DestroyArena(arena);
    TEST_END();
}

void test_arena_clear(void) {
    TEST_START("Arena Clear");
    
    ORM_Arena* arena = ORM_CreateArena(1024);
    
    void* p1 = ORM_ArenaAlloc(arena, 100);
    void* p2 __attribute__((unused)) = ORM_ArenaAlloc(arena, 200);
    
    size_t total, used;
    ORM_ArenaStats(arena, &total, &used);
    TEST_ASSERT(used > 0, "Arena has allocations");
    
    ORM_ArenaClear(arena);
    ORM_ArenaStats(arena, &total, &used);
    TEST_ASSERT(used == 0, "Arena cleared");
    
    // Allocate again - should reuse from start
    void* p3 = ORM_ArenaAlloc(arena, 100);
    TEST_ASSERT(p3 == p1, "Arena reuses memory after clear");
    
    ORM_DestroyArena(arena);
    TEST_END();
}

void test_arena_growth(void) {
    TEST_START("Arena Growth");
    
    ORM_Arena* arena = ORM_CreateArena(512);
    
    // Allocate more than initial size
    void* p1 = ORM_ArenaAlloc(arena, 256);
    void* p2 = ORM_ArenaAlloc(arena, 256);
    void* p3 = ORM_ArenaAlloc(arena, 256); // Should trigger growth
    
    TEST_ASSERT(p1 != NULL, "First alloc succeeds");
    TEST_ASSERT(p2 != NULL, "Second alloc succeeds");
    TEST_ASSERT(p3 != NULL, "Third alloc succeeds (arena grew)");
    
    size_t total, used;
    ORM_ArenaStats(arena, &total, &used);
    TEST_ASSERT(total > 512, "Arena grew beyond initial size");
    
    ORM_DestroyArena(arena);
    TEST_END();
}

// ============================================================================
// Reference Counting Tests
// ============================================================================

void test_refcount_basic(void) {
    TEST_START("Reference Counting Basic");
    
    typedef struct {
        int value;
    } TestObj;
    
    TestObj* obj = (TestObj*)ORM_AllocRef(sizeof(TestObj), NULL);
    TEST_ASSERT(obj != NULL, "Object allocated");
    
    obj->value = 42;
    TEST_ASSERT(obj->value == 42, "Object is usable");
    
    int count = ORM_RefCount(obj);
    TEST_ASSERT(count == 1, "Initial ref count is 1");
    
    ORM_Release(obj);
    TEST_END();
}

void test_refcount_retain_release(void) {
    TEST_START("Reference Counting Retain/Release");
    
    int* obj = (int*)ORM_AllocRef(sizeof(int), NULL);
    *obj = 123;
    
    TEST_ASSERT(ORM_RefCount(obj) == 1, "Initial ref count is 1");
    
    ORM_Retain(obj);
    TEST_ASSERT(ORM_RefCount(obj) == 2, "Ref count increased to 2");
    
    ORM_Retain(obj);
    TEST_ASSERT(ORM_RefCount(obj) == 3, "Ref count increased to 3");
    
    ORM_Release(obj);
    TEST_ASSERT(ORM_RefCount(obj) == 2, "Ref count decreased to 2");
    
    ORM_Release(obj);
    TEST_ASSERT(ORM_RefCount(obj) == 1, "Ref count decreased to 1");
    
    ORM_Release(obj); // Should free
    TEST_END();
}

static int destructor_called = 0;
static void test_destructor(void* obj __attribute__((unused))) {
    destructor_called++;
}

void test_refcount_destructor(void) {
    TEST_START("Reference Counting Destructor");
    
    destructor_called = 0;
    
    int* obj = (int*)ORM_AllocRef(sizeof(int), test_destructor);
    *obj = 456;
    
    ORM_Retain(obj);
    ORM_Release(obj);
    TEST_ASSERT(destructor_called == 0, "Destructor not called (ref count > 0)");
    
    ORM_Release(obj); // Should trigger destructor
    TEST_ASSERT(destructor_called == 1, "Destructor called when ref count reaches 0");
    
    TEST_END();
}

// ============================================================================
// String Buffer Tests
// ============================================================================

void test_stringbuffer_create(void) {
    TEST_START("String Buffer Creation");
    
    ORM_StringBuffer* sb = ORM_StringBufferNew(64);
    TEST_ASSERT(sb != NULL, "String buffer created");
    
    const char* str = ORM_StringBufferGet(sb);
    TEST_ASSERT(str != NULL, "Get string succeeds");
    TEST_ASSERT(strlen(str) == 0, "Initial string is empty");
    
    ORM_StringBufferFree(sb);
    TEST_END();
}

void test_stringbuffer_append(void) {
    TEST_START("String Buffer Append");
    
    ORM_StringBuffer* sb = ORM_StringBufferNew(64);
    
    int result = ORM_StringBufferAppend(sb, "Hello");
    TEST_ASSERT(result == 0, "First append succeeds");
    TEST_ASSERT(strcmp(ORM_StringBufferGet(sb), "Hello") == 0, "Content is 'Hello'");
    
    result = ORM_StringBufferAppend(sb, ", ");
    TEST_ASSERT(result == 0, "Second append succeeds");
    
    result = ORM_StringBufferAppend(sb, "World!");
    TEST_ASSERT(result == 0, "Third append succeeds");
    
    TEST_ASSERT(strcmp(ORM_StringBufferGet(sb), "Hello, World!") == 0, 
                "Final content is 'Hello, World!'");
    
    ORM_StringBufferFree(sb);
    TEST_END();
}

void test_stringbuffer_appendf(void) {
    TEST_START("String Buffer AppendF");
    
    ORM_StringBuffer* sb = ORM_StringBufferNew(64);
    
    int result = ORM_StringBufferAppendF(sb, "Number: %d", 42);
    TEST_ASSERT(result == 0, "AppendF succeeds");
    TEST_ASSERT(strcmp(ORM_StringBufferGet(sb), "Number: 42") == 0, 
                "Formatted content correct");
    
    result = ORM_StringBufferAppendF(sb, ", String: %s", "test");
    TEST_ASSERT(result == 0, "Second AppendF succeeds");
    TEST_ASSERT(strcmp(ORM_StringBufferGet(sb), "Number: 42, String: test") == 0,
                "Combined content correct");
    
    ORM_StringBufferFree(sb);
    TEST_END();
}

void test_stringbuffer_growth(void) {
    TEST_START("String Buffer Growth");
    
    ORM_StringBuffer* sb = ORM_StringBufferNew(16); // Small initial size
    
    // Append enough to trigger growth
    for (int i = 0; i < 10; i++) {
        ORM_StringBufferAppend(sb, "12345");
    }
    
    const char* str = ORM_StringBufferGet(sb);
    TEST_ASSERT(strlen(str) == 50, "String length is 50");
    
    // Should contain repeated "12345"
    TEST_ASSERT(strncmp(str, "12345", 5) == 0, "String starts with '12345'");
    
    ORM_StringBufferFree(sb);
    TEST_END();
}

void test_stringbuffer_clear(void) {
    TEST_START("String Buffer Clear");
    
    ORM_StringBuffer* sb = ORM_StringBufferNew(64);
    
    ORM_StringBufferAppend(sb, "Some text");
    TEST_ASSERT(strlen(ORM_StringBufferGet(sb)) > 0, "Buffer has content");
    
    ORM_StringBufferClear(sb);
    TEST_ASSERT(strlen(ORM_StringBufferGet(sb)) == 0, "Buffer is empty after clear");
    
    ORM_StringBufferAppend(sb, "New text");
    TEST_ASSERT(strcmp(ORM_StringBufferGet(sb), "New text") == 0, 
                "Can append after clear");
    
    ORM_StringBufferFree(sb);
    TEST_END();
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    printf("================================================================================\n");
    printf("ORM MEMORY MANAGEMENT SYSTEM TESTS\n");
    printf("================================================================================\n");
    
    // Pool tests
    test_pool_creation();
    test_pool_allocation();
    test_pool_reuse();
    
    // Arena tests
    test_arena_creation();
    test_arena_allocation();
    test_arena_clear();
    test_arena_growth();
    
    // Reference counting tests
    test_refcount_basic();
    test_refcount_retain_release();
    test_refcount_destructor();
    
    // String buffer tests
    test_stringbuffer_create();
    test_stringbuffer_append();
    test_stringbuffer_appendf();
    test_stringbuffer_growth();
    test_stringbuffer_clear();
    
    // Summary
    printf("\n");
    printf("================================================================================\n");
    printf("TEST SUMMARY\n");
    printf("================================================================================\n");
    printf("Total tests:  %d\n", tests_run);
    printf("Passed:       %d\n", tests_passed);
    printf("Failed:       %d\n", tests_failed);
    printf("================================================================================\n");
    
    if (tests_failed == 0) {
        printf(COLOR_GREEN "✓ ALL TESTS PASSED!" COLOR_RESET "\n");
        printf("================================================================================\n\n");
        return 0;
    } else {
        printf(COLOR_RED "✗ SOME TESTS FAILED!" COLOR_RESET "\n");
        printf("================================================================================\n\n");
        return 1;
    }
}
