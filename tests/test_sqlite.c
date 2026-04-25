/* SQLite Backend Tests
 * 
 * Comprehensive tests for SQLite backend functionality.
 */

#define _GNU_SOURCE
#include "src/backends/sqlite/sqlite_backend.h"
#include "src/core/model.h"
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
// Connection Tests
// ============================================================================

TEST(test_connect_memory_database) {
    ORM_SQLiteConnection* conn = ORM_SQLiteConnect(":memory:");
    ASSERT(conn != NULL, "Connection should be created");
    ASSERT(ORM_SQLiteIsConnected(conn), "Connection should be open");
    ORM_SQLiteDisconnect(conn);
}

TEST(test_connect_file_database) {
    const char* dbfile = "test_temp.db";
    
    ORM_SQLiteConnection* conn = ORM_SQLiteConnect(dbfile);
    ASSERT(conn != NULL, "Connection should be created");
    ASSERT(ORM_SQLiteIsConnected(conn), "Connection should be open");
    
    ORM_SQLiteDisconnect(conn);
    
    // Clean up
    remove(dbfile);
}

TEST(test_disconnect) {
    ORM_SQLiteConnection* conn = ORM_SQLiteConnect(":memory:");
    ASSERT(ORM_SQLiteIsConnected(conn), "Connection should be open");
    
    ORM_SQLiteDisconnect(conn);
    // After disconnect, connection is freed
}

// ============================================================================
// Query Execution Tests
// ============================================================================

TEST(test_execute_create_table) {
    ORM_SQLiteConnection* conn = ORM_SQLiteConnect(":memory:");
    
    int result = ORM_SQLiteExecuteNonQuery(conn,
        "CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)",
        NULL, 0);
    
    ASSERT(result == 0, "CREATE TABLE should succeed");
    ASSERT(ORM_SQLiteTableExists(conn, "test"), "Table should exist");
    
    ORM_SQLiteDisconnect(conn);
}

TEST(test_execute_insert) {
    ORM_SQLiteConnection* conn = ORM_SQLiteConnect(":memory:");
    
    ORM_SQLiteExecuteNonQuery(conn,
        "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, age INTEGER)",
        NULL, 0);
    
    // Insert with parameters
    int age = 25;
    ORM_Parameter params[] = {
        {.type = ORM_TYPE_STRING, .value = "John Doe", .size = 8},
        {.type = ORM_TYPE_INT, .value = &age, .size = sizeof(int)}
    };
    
    int result = ORM_SQLiteExecuteNonQuery(conn,
        "INSERT INTO users (name, age) VALUES (?, ?)",
        params, 2);
    
    ASSERT(result == 0, "INSERT should succeed");
    ASSERT(ORM_SQLiteLastInsertId(conn) == 1, "Last insert ID should be 1");
    ASSERT(ORM_SQLiteAffectedRows(conn) == 1, "Affected rows should be 1");
    
    ORM_SQLiteDisconnect(conn);
}

TEST(test_execute_select) {
    ORM_SQLiteConnection* conn = ORM_SQLiteConnect(":memory:");
    
    // Create and populate table
    ORM_SQLiteExecuteNonQuery(conn,
        "CREATE TABLE users (id INTEGER, name TEXT)",
        NULL, 0);
    
    ORM_SQLiteExecuteNonQuery(conn,
        "INSERT INTO users VALUES (1, 'Alice')",
        NULL, 0);
    
    ORM_SQLiteExecuteNonQuery(conn,
        "INSERT INTO users VALUES (2, 'Bob')",
        NULL, 0);
    
    // Execute SELECT
    ORM_SQLiteResultSet* rs = ORM_SQLiteExecute(conn,
        "SELECT * FROM users",
        NULL, 0);
    
    ASSERT(rs != NULL, "Result set should not be NULL");
    ASSERT(rs->column_count == 2, "Should have 2 columns");
    ASSERT(rs->row_count == 2, "Should have 2 rows");
    
    // Check column names
    ASSERT(strcmp(rs->column_names[0], "id") == 0, "First column should be 'id'");
    ASSERT(strcmp(rs->column_names[1], "name") == 0, "Second column should be 'name'");
    
    ORM_FreeSQLiteResultSet(rs);
    ORM_SQLiteDisconnect(conn);
}

TEST(test_parameterized_query) {
    ORM_SQLiteConnection* conn = ORM_SQLiteConnect(":memory:");
    
    ORM_SQLiteExecuteNonQuery(conn,
        "CREATE TABLE users (id INTEGER, name TEXT, age INTEGER)",
        NULL, 0);
    
    // Insert multiple rows with parameters
    for (int i = 1; i <= 5; i++) {
        char name[32];
        snprintf(name, sizeof(name), "User%d", i);
        
        ORM_Parameter params[] = {
            {.type = ORM_TYPE_INT, .value = &i, .size = sizeof(int)},
            {.type = ORM_TYPE_STRING, .value = name, .size = strlen(name)},
            {.type = ORM_TYPE_INT, .value = &(int){20 + i}, .size = sizeof(int)}
        };
        
        ORM_SQLiteExecuteNonQuery(conn,
            "INSERT INTO users VALUES (?, ?, ?)",
            params, 3);
    }
    
    // Query with WHERE parameter
    int age_filter = 23;
    ORM_Parameter params[] = {
        {.type = ORM_TYPE_INT, .value = &age_filter, .size = sizeof(int)}
    };
    
    ORM_SQLiteResultSet* rs = ORM_SQLiteExecute(conn,
        "SELECT * FROM users WHERE age > ?",
        params, 1);
    
    ASSERT(rs != NULL, "Result set should not be NULL");
    ASSERT(rs->row_count == 2, "Should return 2 rows (age 24 and 25)");
    
    ORM_FreeSQLiteResultSet(rs);
    ORM_SQLiteDisconnect(conn);
}

// ============================================================================
// Transaction Tests
// ============================================================================

TEST(test_transaction_commit) {
    ORM_SQLiteConnection* conn = ORM_SQLiteConnect(":memory:");
    
    ORM_SQLiteExecuteNonQuery(conn,
        "CREATE TABLE test (id INTEGER)",
        NULL, 0);
    
    // Begin transaction
    int result = ORM_SQLiteBeginTransaction(conn);
    ASSERT(result == 0, "Begin transaction should succeed");
    ASSERT(ORM_SQLiteInTransaction(conn), "Should be in transaction");
    
    // Insert data
    ORM_SQLiteExecuteNonQuery(conn,
        "INSERT INTO test VALUES (1)",
        NULL, 0);
    
    // Commit
    result = ORM_SQLiteCommit(conn);
    ASSERT(result == 0, "Commit should succeed");
    ASSERT(!ORM_SQLiteInTransaction(conn), "Should not be in transaction");
    
    // Verify data exists
    ORM_SQLiteResultSet* rs = ORM_SQLiteExecute(conn, "SELECT * FROM test", NULL, 0);
    ASSERT(rs->row_count == 1, "Row should be committed");
    
    ORM_FreeSQLiteResultSet(rs);
    ORM_SQLiteDisconnect(conn);
}

TEST(test_transaction_rollback) {
    ORM_SQLiteConnection* conn = ORM_SQLiteConnect(":memory:");
    
    ORM_SQLiteExecuteNonQuery(conn,
        "CREATE TABLE test (id INTEGER)",
        NULL, 0);
    
    // Begin transaction
    ORM_SQLiteBeginTransaction(conn);
    
    // Insert data
    ORM_SQLiteExecuteNonQuery(conn,
        "INSERT INTO test VALUES (1)",
        NULL, 0);
    
    // Rollback
    int result = ORM_SQLiteRollback(conn);
    ASSERT(result == 0, "Rollback should succeed");
    ASSERT(!ORM_SQLiteInTransaction(conn), "Should not be in transaction");
    
    // Verify data doesn't exist
    ORM_SQLiteResultSet* rs = ORM_SQLiteExecute(conn, "SELECT * FROM test", NULL, 0);
    ASSERT(rs->row_count == 0, "Row should be rolled back");
    
    ORM_FreeSQLiteResultSet(rs);
    ORM_SQLiteDisconnect(conn);
}

TEST(test_nested_transactions) {
    ORM_SQLiteConnection* conn = ORM_SQLiteConnect(":memory:");
    
    ORM_SQLiteExecuteNonQuery(conn,
        "CREATE TABLE test (id INTEGER)",
        NULL, 0);
    
    // Level 1 transaction
    ORM_SQLiteBeginTransaction(conn);
    ORM_SQLiteExecuteNonQuery(conn, "INSERT INTO test VALUES (1)", NULL, 0);
    
    // Level 2 transaction (savepoint)
    ORM_SQLiteBeginTransaction(conn);
    ORM_SQLiteExecuteNonQuery(conn, "INSERT INTO test VALUES (2)", NULL, 0);
    
    // Rollback level 2
    ORM_SQLiteRollback(conn);
    
    // Commit level 1
    ORM_SQLiteCommit(conn);
    
    // Should have only row 1
    ORM_SQLiteResultSet* rs = ORM_SQLiteExecute(conn, "SELECT * FROM test", NULL, 0);
    ASSERT(rs->row_count == 1, "Should have 1 row after nested rollback");
    
    ORM_FreeSQLiteResultSet(rs);
    ORM_SQLiteDisconnect(conn);
}

// ============================================================================
// Schema Tests
// ============================================================================

TEST(test_table_exists) {
    ORM_SQLiteConnection* conn = ORM_SQLiteConnect(":memory:");
    
    ASSERT(!ORM_SQLiteTableExists(conn, "nonexistent"), "Nonexistent table should return false");
    
    ORM_SQLiteExecuteNonQuery(conn, "CREATE TABLE test (id INTEGER)", NULL, 0);
    
    ASSERT(ORM_SQLiteTableExists(conn, "test"), "Existing table should return true");
    
    ORM_SQLiteDisconnect(conn);
}

TEST(test_drop_table) {
    ORM_SQLiteConnection* conn = ORM_SQLiteConnect(":memory:");
    
    ORM_SQLiteExecuteNonQuery(conn, "CREATE TABLE test (id INTEGER)", NULL, 0);
    ASSERT(ORM_SQLiteTableExists(conn, "test"), "Table should exist");
    
    int result = ORM_SQLiteDropTable(conn, "test");
    ASSERT(result == 0, "Drop table should succeed");
    ASSERT(!ORM_SQLiteTableExists(conn, "test"), "Table should not exist");
    
    ORM_SQLiteDisconnect(conn);
}

TEST(test_get_columns) {
    ORM_SQLiteConnection* conn = ORM_SQLiteConnect(":memory:");
    
    ORM_SQLiteExecuteNonQuery(conn,
        "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL, age INTEGER)",
        NULL, 0);
    
    int col_count = 0;
    ORM_ColumnInfo* columns = ORM_SQLiteGetColumns(conn, "users", &col_count);
    
    ASSERT(columns != NULL, "Columns should not be NULL");
    ASSERT(col_count == 3, "Should have 3 columns");
    
    // Check column details
    ASSERT(strcmp(columns[0].name, "id") == 0, "First column name should be 'id'");
    ASSERT(columns[0].primary_key, "First column should be primary key");
    
    ASSERT(strcmp(columns[1].name, "name") == 0, "Second column name should be 'name'");
    ASSERT(columns[1].not_null, "Second column should be NOT NULL");
    
    ORM_FreeColumnInfo(columns, col_count);
    ORM_SQLiteDisconnect(conn);
}

TEST(test_create_index) {
    ORM_SQLiteConnection* conn = ORM_SQLiteConnect(":memory:");
    
    ORM_SQLiteExecuteNonQuery(conn,
        "CREATE TABLE users (id INTEGER, name TEXT)",
        NULL, 0);
    
    int result = ORM_SQLiteCreateIndex(conn, "users", "name", false);
    ASSERT(result == 0, "CREATE INDEX should succeed");
    
    ORM_SQLiteDisconnect(conn);
}

// ============================================================================
// Prepared Statement Tests
// ============================================================================

TEST(test_prepared_statement) {
    ORM_SQLiteConnection* conn = ORM_SQLiteConnect(":memory:");
    
    ORM_SQLiteExecuteNonQuery(conn,
        "CREATE TABLE users (id INTEGER, name TEXT)",
        NULL, 0);
    
    // Prepare statement
    ORM_PreparedStatement* stmt = ORM_SQLitePrepare(conn,
        "INSERT INTO users VALUES (?, ?)");
    
    ASSERT(stmt != NULL, "Prepared statement should not be NULL");
    
    // Execute multiple times with different parameters
    for (int i = 1; i <= 3; i++) {
        char name[32];
        snprintf(name, sizeof(name), "User%d", i);
        
        ORM_Parameter params[] = {
            {.type = ORM_TYPE_INT, .value = &i, .size = sizeof(int)},
            {.type = ORM_TYPE_STRING, .value = name, .size = strlen(name)}
        };
        
        ORM_SQLiteBindParameters(stmt, params, 2);
        ORM_SQLiteResultSet* rs = ORM_SQLiteExecutePrepared(stmt);
        ORM_FreeSQLiteResultSet(rs);
        ORM_SQLiteResetStatement(stmt);
    }
    
    ORM_SQLiteFreePrepared(stmt);
    
    // Verify all rows inserted
    ORM_SQLiteResultSet* rs = ORM_SQLiteExecute(conn, "SELECT * FROM users", NULL, 0);
    ASSERT(rs->row_count == 3, "Should have 3 rows");
    
    ORM_FreeSQLiteResultSet(rs);
    ORM_SQLiteDisconnect(conn);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    printf("\n" COLOR_YELLOW "=== SQLite Backend Test Suite ===" COLOR_RESET "\n\n");
    
    // Connection Tests
    printf(COLOR_YELLOW "Connection Tests:" COLOR_RESET "\n");
    RUN_TEST(test_connect_memory_database);
    RUN_TEST(test_connect_file_database);
    RUN_TEST(test_disconnect);
    
    // Query Execution Tests
    printf("\n" COLOR_YELLOW "Query Execution Tests:" COLOR_RESET "\n");
    RUN_TEST(test_execute_create_table);
    RUN_TEST(test_execute_insert);
    RUN_TEST(test_execute_select);
    RUN_TEST(test_parameterized_query);
    
    // Transaction Tests
    printf("\n" COLOR_YELLOW "Transaction Tests:" COLOR_RESET "\n");
    RUN_TEST(test_transaction_commit);
    RUN_TEST(test_transaction_rollback);
    RUN_TEST(test_nested_transactions);
    
    // Schema Tests
    printf("\n" COLOR_YELLOW "Schema Tests:" COLOR_RESET "\n");
    RUN_TEST(test_table_exists);
    RUN_TEST(test_drop_table);
    RUN_TEST(test_get_columns);
    RUN_TEST(test_create_index);
    
    // Prepared Statement Tests
    printf("\n" COLOR_YELLOW "Prepared Statement Tests:" COLOR_RESET "\n");
    RUN_TEST(test_prepared_statement);
    
    // Summary
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
