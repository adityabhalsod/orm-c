/* Result Set Management Tests
 * 
 * Comprehensive tests for result set operations, iteration,
 * type-safe access, pagination, and transformations.
 */

#define _GNU_SOURCE
#include "src/core/result_set.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test framework macros
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"

static int tests_run = 0;
static int tests_passed = 0;
static int assertions = 0;

#define TEST(name) \
    static void name(); \
    static void run_##name() { \
        printf("  " COLOR_YELLOW "Running: %s" COLOR_RESET "\n", #name); \
        tests_run++; \
        name(); \
        tests_passed++; \
        printf("  " COLOR_GREEN "✓ %s passed" COLOR_RESET "\n", #name); \
    } \
    static void name()

#define ASSERT(condition, message) \
    do { \
        assertions++; \
        if (!(condition)) { \
            fprintf(stderr, COLOR_RED "Assertion failed: %s\n  at %s:%d" COLOR_RESET "\n", \
                    message, __FILE__, __LINE__); \
            exit(1); \
        } \
    } while (0)

#define RUN_TEST(name) run_##name()

// ============================================================================
// Helper Functions
// ============================================================================

ORM_ResultSet* create_test_result_set() {
    ORM_ResultSet* rs = ORM_CreateResultSet(3, 4);
    
    // Set up columns
    ORM_SetColumnMeta(rs, 0, "id", ORM_TYPE_INT, sizeof(int));
    ORM_SetColumnMeta(rs, 1, "name", ORM_TYPE_STRING, 64);
    ORM_SetColumnMeta(rs, 2, "age", ORM_TYPE_INT, sizeof(int));
    ORM_SetColumnMeta(rs, 3, "score", ORM_TYPE_DOUBLE, sizeof(double));
    
    // Fill in data - Row 0
    int* id0 = malloc(sizeof(int));
    *id0 = 1;
    rs->rows[0][0] = id0;
    
    char* name0 = strdup("Alice");
    rs->rows[0][1] = name0;
    
    int* age0 = malloc(sizeof(int));
    *age0 = 25;
    rs->rows[0][2] = age0;
    
    double* score0 = malloc(sizeof(double));
    *score0 = 95.5;
    rs->rows[0][3] = score0;
    
    // Fill in data - Row 1
    int* id1 = malloc(sizeof(int));
    *id1 = 2;
    rs->rows[1][0] = id1;
    
    char* name1 = strdup("Bob");
    rs->rows[1][1] = name1;
    
    int* age1 = malloc(sizeof(int));
    *age1 = 30;
    rs->rows[1][2] = age1;
    
    double* score1 = malloc(sizeof(double));
    *score1 = 87.3;
    rs->rows[1][3] = score1;
    
    // Fill in data - Row 2
    int* id2 = malloc(sizeof(int));
    *id2 = 3;
    rs->rows[2][0] = id2;
    
    char* name2 = strdup("Charlie");
    rs->rows[2][1] = name2;
    
    int* age2 = malloc(sizeof(int));
    *age2 = 22;
    rs->rows[2][2] = age2;
    
    double* score2 = malloc(sizeof(double));
    *score2 = 91.7;
    rs->rows[2][3] = score2;
    
    return rs;
}

// ============================================================================
// Creation and Basic Operations Tests
// ============================================================================

TEST(test_create_result_set) {
    ORM_ResultSet* rs = ORM_CreateResultSet(10, 5);
    
    ASSERT(rs != NULL, "Result set should be created");
    ASSERT(rs->row_count == 10, "Row count should be 10");
    ASSERT(rs->column_count == 5, "Column count should be 5");
    ASSERT(rs->current_row == -1, "Current row should be -1 initially");
    ASSERT(rs->owns_data == true, "Should own data");
    
    ORM_FreeResultSet(rs);
}

TEST(test_column_metadata) {
    ORM_ResultSet* rs = ORM_CreateResultSet(5, 3);
    
    ORM_SetColumnMeta(rs, 0, "id", ORM_TYPE_INT, sizeof(int));
    ORM_SetColumnMeta(rs, 1, "name", ORM_TYPE_STRING, 64);
    ORM_SetColumnMeta(rs, 2, "value", ORM_TYPE_DOUBLE, sizeof(double));
    
    ASSERT(strcmp(ORM_GetColumnName(rs, 0), "id") == 0, "Column 0 name");
    ASSERT(strcmp(ORM_GetColumnName(rs, 1), "name") == 0, "Column 1 name");
    ASSERT(strcmp(ORM_GetColumnName(rs, 2), "value") == 0, "Column 2 name");
    
    ASSERT(ORM_GetColumnType(rs, 0) == ORM_TYPE_INT, "Column 0 type");
    ASSERT(ORM_GetColumnType(rs, 1) == ORM_TYPE_STRING, "Column 1 type");
    ASSERT(ORM_GetColumnType(rs, 2) == ORM_TYPE_DOUBLE, "Column 2 type");
    
    ASSERT(ORM_GetColumnIndex(rs, "id") == 0, "Find column by name: id");
    ASSERT(ORM_GetColumnIndex(rs, "name") == 1, "Find column by name: name");
    ASSERT(ORM_GetColumnIndex(rs, "value") == 2, "Find column by name: value");
    ASSERT(ORM_GetColumnIndex(rs, "nonexistent") == -1, "Nonexistent column");
    
    ORM_FreeResultSet(rs);
}

// ============================================================================
// Iteration Tests
// ============================================================================

TEST(test_iteration) {
    ORM_ResultSet* rs = create_test_result_set();
    
    ASSERT(ORM_ResultHasNext(rs), "Should have next initially");
    
    int count = 0;
    while (ORM_ResultNext(rs)) {
        void* id_ptr = ORM_ResultCurrent(rs, 0);
        ASSERT(id_ptr != NULL, "ID should not be NULL");
        count++;
    }
    
    ASSERT(count == 3, "Should iterate 3 rows");
    ASSERT(!ORM_ResultHasNext(rs), "Should not have next after iteration");
    
    // Reset and iterate again
    ORM_ResultReset(rs);
    ASSERT(rs->current_row == -1, "Reset should set current_row to -1");
    ASSERT(ORM_ResultHasNext(rs), "Should have next after reset");
    
    ORM_FreeResultSet(rs);
}

TEST(test_foreach_macro) {
    ORM_ResultSet* rs = create_test_result_set();
    
    int count = 0;
    ORM_FOREACH_ROW(rs) {
        void* id = ORM_ResultCurrent(rs, 0);
        ASSERT(id != NULL, "Row should have ID");
        count++;
    }
    
    ASSERT(count == 3, "FOREACH should iterate 3 rows");
    
    ORM_FreeResultSet(rs);
}

// ============================================================================
// Type-Safe Access Tests
// ============================================================================

TEST(test_typed_getters) {
    ORM_ResultSet* rs = create_test_result_set();
    
    // Test row 0
    ASSERT(ORM_GetInt(rs, 0, "id") == 1, "Row 0 ID");
    ASSERT(strcmp(ORM_GetString(rs, 0, "name"), "Alice") == 0, "Row 0 name");
    ASSERT(ORM_GetInt(rs, 0, "age") == 25, "Row 0 age");
    ASSERT(ORM_GetDouble(rs, 0, "score") > 95.4 && ORM_GetDouble(rs, 0, "score") < 95.6, 
           "Row 0 score");
    
    // Test row 1
    ASSERT(ORM_GetInt(rs, 1, "id") == 2, "Row 1 ID");
    ASSERT(strcmp(ORM_GetString(rs, 1, "name"), "Bob") == 0, "Row 1 name");
    ASSERT(ORM_GetInt(rs, 1, "age") == 30, "Row 1 age");
    
    // Test row 2
    ASSERT(ORM_GetInt(rs, 2, "id") == 3, "Row 2 ID");
    ASSERT(strcmp(ORM_GetString(rs, 2, "name"), "Charlie") == 0, "Row 2 name");
    ASSERT(ORM_GetInt(rs, 2, "age") == 22, "Row 2 age");
    
    ORM_FreeResultSet(rs);
}

TEST(test_null_handling) {
    ORM_ResultSet* rs = ORM_CreateResultSet(2, 2);
    
    ORM_SetColumnMeta(rs, 0, "id", ORM_TYPE_INT, sizeof(int));
    ORM_SetColumnMeta(rs, 1, "name", ORM_TYPE_STRING, 64);
    
    // Set row 0 with values
    int* id0 = malloc(sizeof(int));
    *id0 = 1;
    rs->rows[0][0] = id0;
    rs->rows[0][1] = strdup("Test");
    
    // Row 1 has NULL name
    int* id1 = malloc(sizeof(int));
    *id1 = 2;
    rs->rows[1][0] = id1;
    rs->rows[1][1] = NULL;
    
    ASSERT(!ORM_IsNull(rs, 0, "id"), "Row 0 ID should not be NULL");
    ASSERT(!ORM_IsNull(rs, 0, "name"), "Row 0 name should not be NULL");
    
    ASSERT(!ORM_IsNull(rs, 1, "id"), "Row 1 ID should not be NULL");
    ASSERT(ORM_IsNull(rs, 1, "name"), "Row 1 name should be NULL");
    
    ORM_FreeResultSet(rs);
}

// ============================================================================
// Iterator Pattern Tests
// ============================================================================

TEST(test_iterator) {
    ORM_ResultSet* rs = create_test_result_set();
    
    ORM_ResultIterator* iter = ORM_CreateIterator(rs);
    ASSERT(iter != NULL, "Iterator should be created");
    ASSERT(iter->current_index == -1, "Initial index should be -1");
    
    int count = 0;
    while (ORM_IteratorHasNext(iter)) {
        void** row = ORM_IteratorNext(iter);
        ASSERT(row != NULL, "Row should not be NULL");
        count++;
    }
    
    ASSERT(count == 3, "Should iterate 3 rows");
    ASSERT(!ORM_IteratorHasNext(iter), "Should not have next");
    
    // Reset and iterate again
    ORM_IteratorReset(iter);
    ASSERT(iter->current_index == -1, "Reset should set index to -1");
    ASSERT(ORM_IteratorHasNext(iter), "Should have next after reset");
    
    ORM_FreeIterator(iter);
    ORM_FreeResultSet(rs);
}

// ============================================================================
// Cursor Pagination Tests
// ============================================================================

TEST(test_cursor_pagination) {
    // Create larger result set
    ORM_ResultSet* rs = ORM_CreateResultSet(25, 2);
    ORM_SetColumnMeta(rs, 0, "id", ORM_TYPE_INT, sizeof(int));
    ORM_SetColumnMeta(rs, 1, "value", ORM_TYPE_INT, sizeof(int));
    
    // Fill with data
    for (int i = 0; i < 25; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        rs->rows[i][0] = id;
        
        int* value = malloc(sizeof(int));
        *value = (i + 1) * 10;
        rs->rows[i][1] = value;
    }
    
    // Create cursor with page size 10
    ORM_Cursor* cursor = ORM_CreateCursor(rs, 10);
    ASSERT(cursor != NULL, "Cursor should be created");
    ASSERT(cursor->page_size == 10, "Page size should be 10");
    ASSERT(cursor->has_next, "Should have next page");
    ASSERT(!cursor->has_previous, "Should not have previous initially");
    
    // Get first page
    ORM_ResultSet* page1 = ORM_CursorNextPage(cursor);
    ASSERT(page1 != NULL, "Page 1 should not be NULL");
    ASSERT(page1->row_count == 10, "Page 1 should have 10 rows");
    ASSERT(ORM_GetInt(page1, 0, "id") == 1, "First row ID");
    ASSERT(ORM_GetInt(page1, 9, "id") == 10, "Last row ID of page 1");
    
    // Get second page
    ORM_ResultSet* page2 = ORM_CursorNextPage(cursor);
    ASSERT(page2 != NULL, "Page 2 should not be NULL");
    ASSERT(page2->row_count == 10, "Page 2 should have 10 rows");
    ASSERT(ORM_GetInt(page2, 0, "id") == 11, "First row of page 2");
    
    // Get third page (partial)
    ORM_ResultSet* page3 = ORM_CursorNextPage(cursor);
    ASSERT(page3 != NULL, "Page 3 should not be NULL");
    ASSERT(page3->row_count == 5, "Page 3 should have 5 rows");
    ASSERT(!cursor->has_next, "Should not have next after last page");
    
    ORM_FreeResultSet(page1);
    ORM_FreeResultSet(page2);
    ORM_FreeResultSet(page3);
    ORM_FreeCursor(cursor);
    ORM_FreeResultSet(rs);
}

TEST(test_cursor_token) {
    ORM_ResultSet* rs = ORM_CreateResultSet(20, 1);
    ORM_SetColumnMeta(rs, 0, "id", ORM_TYPE_INT, sizeof(int));
    
    for (int i = 0; i < 20; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        rs->rows[i][0] = id;
    }
    
    ORM_Cursor* cursor = ORM_CreateCursor(rs, 5);
    ORM_ResultSet* page1 = ORM_CursorNextPage(cursor);
    
    const char* token = ORM_GetCursorToken(cursor);
    ASSERT(token != NULL, "Token should not be NULL");
    
    // Create new cursor from token
    ORM_Cursor* cursor2 = ORM_FromCursorToken(rs, token, 5);
    ASSERT(cursor2 != NULL, "Cursor from token should be created");
    ASSERT(cursor2->current_offset == cursor->current_offset, "Offset should match");
    
    ORM_FreeResultSet(page1);
    ORM_FreeCursor(cursor);
    ORM_FreeCursor(cursor2);
    ORM_FreeResultSet(rs);
}

// ============================================================================
// Transformation Tests
// ============================================================================

static bool filter_age_over_25(void** row, int column_count, void* context) {
    (void)context;
    (void)column_count;
    // Column 2 is age
    int* age = (int*)row[2];
    return age && *age > 25;
}

TEST(test_filter) {
    ORM_ResultSet* rs = create_test_result_set();
    
    ORM_ResultSet* filtered = ORM_FilterResultSet(rs, filter_age_over_25, NULL);
    
    ASSERT(filtered != NULL, "Filtered result should not be NULL");
    ASSERT(filtered->row_count == 1, "Should have 1 row with age > 25");
    ASSERT(ORM_GetInt(filtered, 0, "age") == 30, "Filtered row should have age 30");
    
    ORM_FreeResultSet(filtered);
    ORM_FreeResultSet(rs);
}

TEST(test_slice) {
    ORM_ResultSet* rs = create_test_result_set();
    
    ORM_ResultSet* slice = ORM_SliceResultSet(rs, 1, 3);
    
    ASSERT(slice != NULL, "Slice should not be NULL");
    ASSERT(slice->row_count == 2, "Slice should have 2 rows");
    ASSERT(ORM_GetInt(slice, 0, "id") == 2, "First row of slice");
    ASSERT(ORM_GetInt(slice, 1, "id") == 3, "Second row of slice");
    
    ORM_FreeResultSet(slice);
    ORM_FreeResultSet(rs);
}

TEST(test_clone) {
    ORM_ResultSet* rs = create_test_result_set();
    
    ORM_ResultSet* clone = ORM_CloneResultSet(rs);
    
    ASSERT(clone != NULL, "Clone should not be NULL");
    ASSERT(clone->row_count == rs->row_count, "Row counts should match");
    ASSERT(clone->column_count == rs->column_count, "Column counts should match");
    
    // Verify data
    ASSERT(ORM_GetInt(clone, 0, "id") == ORM_GetInt(rs, 0, "id"), "Cloned data should match");
    ASSERT(strcmp(ORM_GetString(clone, 1, "name"), ORM_GetString(rs, 1, "name")) == 0, 
           "Cloned strings should match");
    
    ORM_FreeResultSet(clone);
    ORM_FreeResultSet(rs);
}

TEST(test_to_json) {
    ORM_ResultSet* rs = create_test_result_set();
    
    char* json = ORM_ResultSetToJSON(rs);
    
    ASSERT(json != NULL, "JSON should not be NULL");
    ASSERT(strstr(json, "\"id\": 1") != NULL, "JSON should contain ID");
    ASSERT(strstr(json, "\"name\": \"Alice\"") != NULL, "JSON should contain name");
    ASSERT(strstr(json, "\"age\": 25") != NULL, "JSON should contain age");
    
    free(json);
    ORM_FreeResultSet(rs);
}

TEST(test_to_csv) {
    ORM_ResultSet* rs = create_test_result_set();
    
    char* csv = ORM_ResultSetToCSV(rs);
    
    ASSERT(csv != NULL, "CSV should not be NULL");
    ASSERT(strstr(csv, "id,name,age,score") != NULL, "CSV should have header");
    ASSERT(strstr(csv, "1,Alice,25") != NULL, "CSV should have data");
    ASSERT(strstr(csv, "2,Bob,30") != NULL, "CSV should have row 2");
    
    free(csv);
    ORM_FreeResultSet(rs);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    printf("\n" COLOR_YELLOW "=== Result Set Test Suite ===" COLOR_RESET "\n\n");
    
    printf(COLOR_YELLOW "Creation and Basic Operations:" COLOR_RESET "\n");
    RUN_TEST(test_create_result_set);
    RUN_TEST(test_column_metadata);
    
    printf("\n" COLOR_YELLOW "Iteration Tests:" COLOR_RESET "\n");
    RUN_TEST(test_iteration);
    RUN_TEST(test_foreach_macro);
    
    printf("\n" COLOR_YELLOW "Type-Safe Access Tests:" COLOR_RESET "\n");
    RUN_TEST(test_typed_getters);
    RUN_TEST(test_null_handling);
    
    printf("\n" COLOR_YELLOW "Iterator Pattern Tests:" COLOR_RESET "\n");
    RUN_TEST(test_iterator);
    
    printf("\n" COLOR_YELLOW "Cursor Pagination Tests:" COLOR_RESET "\n");
    RUN_TEST(test_cursor_pagination);
    RUN_TEST(test_cursor_token);
    
    printf("\n" COLOR_YELLOW "Transformation Tests:" COLOR_RESET "\n");
    RUN_TEST(test_filter);
    RUN_TEST(test_slice);
    RUN_TEST(test_clone);
    RUN_TEST(test_to_json);
    RUN_TEST(test_to_csv);
    
    printf("\n" COLOR_YELLOW "=== Test Summary ===" COLOR_RESET "\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: " COLOR_GREEN "%d" COLOR_RESET "\n", tests_passed);
    printf("Assertions: %d\n", assertions);
    
    if (tests_passed == tests_run) {
        printf("\n" COLOR_GREEN "✓ All tests passed!" COLOR_RESET "\n\n");
        return 0;
    } else {
        printf("\n" COLOR_RED "✗ Some tests failed!" COLOR_RESET "\n\n");
        return 1;
    }
}
