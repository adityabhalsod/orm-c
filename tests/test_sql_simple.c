/* Simple SQL Generation Tests
 * 
 * Basic tests for SQL escaping and dialect-specific features.
 */

#define _GNU_SOURCE
#include "src/core/model.h"
#include "src/core/sql_builder.h"
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
// SQL Escaping Tests
// ============================================================================

TEST(test_escape_sql_string_standard) {
    char* escaped;
    
    // Single quote doubling (SQLite/PostgreSQL)
    escaped = ORM_EscapeSQLString("O'Reilly", ORM_DIALECT_SQLITE);
    ASSERT(strcmp(escaped, "O''Reilly") == 0, "Single quote escaped");
    free(escaped);
    
    // Multiple quotes
    escaped = ORM_EscapeSQLString("It's John's book", ORM_DIALECT_POSTGRESQL);
    ASSERT(strcmp(escaped, "It''s John''s book") == 0, "Multiple quotes escaped");
    free(escaped);
    
    // No quotes
    escaped = ORM_EscapeSQLString("Simple text", ORM_DIALECT_SQLITE);
    ASSERT(strcmp(escaped, "Simple text") == 0, "No escaping needed");
    free(escaped);
}

TEST(test_escape_sql_string_mysql) {
    char* escaped;
    
    // Newline
    escaped = ORM_EscapeSQLString("Line 1\nLine 2", ORM_DIALECT_MYSQL);
    ASSERT(strcmp(escaped, "Line 1\\nLine 2") == 0, "Newline escaped");
    free(escaped);
    
    // Backslash
    escaped = ORM_EscapeSQLString("C:\\path\\to\\file", ORM_DIALECT_MYSQL);
    ASSERT(strcmp(escaped, "C:\\\\path\\\\to\\\\file") == 0, "Backslash escaped");
    free(escaped);
    
    // Single quote
    escaped = ORM_EscapeSQLString("O'Reilly", ORM_DIALECT_MYSQL);
    ASSERT(strcmp(escaped, "O\\'Reilly") == 0, "Single quote escaped");
    free(escaped);
}

TEST(test_unescape_sql_string) {
    char* unescaped;
    
    // Standard SQL
    unescaped = ORM_UnescapeSQLString("O''Reilly", ORM_DIALECT_SQLITE);
    ASSERT(strcmp(unescaped, "O'Reilly") == 0, "Single quote unescaped");
    free(unescaped);
    
    // MySQL
    unescaped = ORM_UnescapeSQLString("Line 1\\nLine 2", ORM_DIALECT_MYSQL);
    ASSERT(strcmp(unescaped, "Line 1\nLine 2") == 0, "Newline unescaped");
    free(unescaped);
}

// ============================================================================
// Identifier Quoting Tests
// ============================================================================

TEST(test_quote_identifier) {
    char* quoted;
    
    // SQLite/PostgreSQL use double quotes
    quoted = ORM_QuoteIdentifier("users", ORM_DIALECT_SQLITE);
    ASSERT(strcmp(quoted, "\"users\"") == 0, "SQLite identifier quoting");
    free(quoted);
    
    quoted = ORM_QuoteIdentifier("table_name", ORM_DIALECT_POSTGRESQL);
    ASSERT(strcmp(quoted, "\"table_name\"") == 0, "PostgreSQL identifier quoting");
    free(quoted);
    
    // MySQL uses backticks
    quoted = ORM_QuoteIdentifier("users", ORM_DIALECT_MYSQL);
    ASSERT(strcmp(quoted, "`users`") == 0, "MySQL identifier quoting");
    free(quoted);
    
    // MSSQL uses brackets
    quoted = ORM_QuoteIdentifier("users", ORM_DIALECT_MSSQL);
    ASSERT(strcmp(quoted, "[users]") == 0, "MSSQL identifier quoting");
    free(quoted);
}

TEST(test_validate_identifier) {
    ASSERT(ORM_ValidateIdentifier("users"), "Valid simple identifier");
    ASSERT(ORM_ValidateIdentifier("table_name"), "Valid identifier with underscore");
    ASSERT(ORM_ValidateIdentifier("_private"), "Valid identifier starting with underscore");
    ASSERT(ORM_ValidateIdentifier("Table123"), "Valid identifier with numbers");
    
    ASSERT(!ORM_ValidateIdentifier(""), "Empty identifier invalid");
    ASSERT(!ORM_ValidateIdentifier("123users"), "Identifier starting with number invalid");
    ASSERT(!ORM_ValidateIdentifier("user-name"), "Identifier with hyphen invalid");
    ASSERT(!ORM_ValidateIdentifier("user name"), "Identifier with space invalid");
}

// ============================================================================
// Type Mapping Tests
// ============================================================================

TEST(test_type_mapping_sqlite) {
    ASSERT(strcmp(ORM_DialectTypeName(ORM_TYPE_INT, ORM_DIALECT_SQLITE), "INTEGER") == 0, "INT -> INTEGER");
    ASSERT(strcmp(ORM_DialectTypeName(ORM_TYPE_FLOAT, ORM_DIALECT_SQLITE), "REAL") == 0, "FLOAT -> REAL");
    ASSERT(strcmp(ORM_DialectTypeName(ORM_TYPE_STRING, ORM_DIALECT_SQLITE), "VARCHAR") == 0, "STRING -> VARCHAR");
    ASSERT(strcmp(ORM_DialectTypeName(ORM_TYPE_TEXT, ORM_DIALECT_SQLITE), "TEXT") == 0, "TEXT -> TEXT");
    ASSERT(strcmp(ORM_DialectTypeName(ORM_TYPE_BLOB, ORM_DIALECT_SQLITE), "BLOB") == 0, "BLOB -> BLOB");
}

TEST(test_type_mapping_postgresql) {
    ASSERT(strcmp(ORM_DialectTypeName(ORM_TYPE_INT, ORM_DIALECT_POSTGRESQL), "INTEGER") == 0, "INT -> INTEGER");
    ASSERT(strcmp(ORM_DialectTypeName(ORM_TYPE_DOUBLE, ORM_DIALECT_POSTGRESQL), "DOUBLE PRECISION") == 0, "DOUBLE -> DOUBLE PRECISION");
    ASSERT(strcmp(ORM_DialectTypeName(ORM_TYPE_BOOLEAN, ORM_DIALECT_POSTGRESQL), "BOOLEAN") == 0, "BOOLEAN -> BOOLEAN");
    ASSERT(strcmp(ORM_DialectTypeName(ORM_TYPE_DATETIME, ORM_DIALECT_POSTGRESQL), "TIMESTAMP") == 0, "DATETIME -> TIMESTAMP");
    ASSERT(strcmp(ORM_DialectTypeName(ORM_TYPE_BLOB, ORM_DIALECT_POSTGRESQL), "BYTEA") == 0, "BLOB -> BYTEA");
}

TEST(test_type_mapping_mysql) {
    ASSERT(strcmp(ORM_DialectTypeName(ORM_TYPE_INT, ORM_DIALECT_MYSQL), "INT") == 0, "INT -> INT");
    ASSERT(strcmp(ORM_DialectTypeName(ORM_TYPE_BOOLEAN, ORM_DIALECT_MYSQL), "TINYINT(1)") == 0, "BOOLEAN -> TINYINT(1)");
    ASSERT(strcmp(ORM_DialectTypeName(ORM_TYPE_DATETIME, ORM_DIALECT_MYSQL), "DATETIME") == 0, "DATETIME -> DATETIME");
    ASSERT(strcmp(ORM_DialectTypeName(ORM_TYPE_BLOB, ORM_DIALECT_MYSQL), "BLOB") == 0, "BLOB -> BLOB");
}

// ============================================================================
// SQL Builder Tests
// ============================================================================

TEST(test_builder_creation) {
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_SQLITE);
    ASSERT(builder != NULL, "Builder should be created");
    ASSERT(builder->dialect == ORM_DIALECT_SQLITE, "Dialect should match");
    ASSERT(builder->param_count == 0, "Should have no parameters initially");
    ORM_FreeSQLBuilder(builder);
}

TEST(test_dialect_names) {
    ASSERT(strcmp(ORM_DialectName(ORM_DIALECT_SQLITE), "SQLite") == 0, "SQLite name");
    ASSERT(strcmp(ORM_DialectName(ORM_DIALECT_POSTGRESQL), "PostgreSQL") == 0, "PostgreSQL name");
    ASSERT(strcmp(ORM_DialectName(ORM_DIALECT_MYSQL), "MySQL") == 0, "MySQL name");
    ASSERT(strcmp(ORM_DialectName(ORM_DIALECT_MSSQL), "MS SQL Server") == 0, "MSSQL name");
}

TEST(test_placeholders) {
    ORM_SQLBuilder* builder;
    
    // SQLite uses ?
    builder = ORM_CreateSQLBuilder(ORM_DIALECT_SQLITE);
    ASSERT(strcmp(ORM_GetPlaceholder(builder, 0), "?") == 0, "SQLite placeholder 0");
    ASSERT(strcmp(ORM_GetPlaceholder(builder, 5), "?") == 0, "SQLite placeholder 5");
    ORM_FreeSQLBuilder(builder);
    
    // PostgreSQL uses $1, $2, etc
    builder = ORM_CreateSQLBuilder(ORM_DIALECT_POSTGRESQL);
    ASSERT(strcmp(ORM_GetPlaceholder(builder, 0), "$1") == 0, "PostgreSQL placeholder 0");
    ASSERT(strcmp(ORM_GetPlaceholder(builder, 1), "$2") == 0, "PostgreSQL placeholder 1");
    ASSERT(strcmp(ORM_GetPlaceholder(builder, 9), "$10") == 0, "PostgreSQL placeholder 9");
    ORM_FreeSQLBuilder(builder);
    
    // MySQL uses ?
    builder = ORM_CreateSQLBuilder(ORM_DIALECT_MYSQL);
    ASSERT(strcmp(ORM_GetPlaceholder(builder, 0), "?") == 0, "MySQL placeholder");
    ORM_FreeSQLBuilder(builder);
    
    // MSSQL uses @p1, @p2, etc
    builder = ORM_CreateSQLBuilder(ORM_DIALECT_MSSQL);
    ASSERT(strcmp(ORM_GetPlaceholder(builder, 0), "@p1") == 0, "MSSQL placeholder 0");
    ASSERT(strcmp(ORM_GetPlaceholder(builder, 1), "@p2") == 0, "MSSQL placeholder 1");
    ORM_FreeSQLBuilder(builder);
}

TEST(test_operator_to_sql) {
    ASSERT(strcmp(ORM_OperatorToSQL(OP_EQ), "=") == 0, "EQ -> =");
    ASSERT(strcmp(ORM_OperatorToSQL(OP_NE), "!=") == 0, "NE -> !=");
    ASSERT(strcmp(ORM_OperatorToSQL(OP_LT), "<") == 0, "LT -> <");
    ASSERT(strcmp(ORM_OperatorToSQL(OP_GT), ">") == 0, "GT -> >");
    ASSERT(strcmp(ORM_OperatorToSQL(OP_LIKE), "LIKE") == 0, "LIKE -> LIKE");
    ASSERT(strcmp(ORM_OperatorToSQL(OP_IS_NULL), "IS NULL") == 0, "IS_NULL -> IS NULL");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    printf("\n" COLOR_YELLOW "=== Simple SQL Generation Tests ===" COLOR_RESET "\n\n");
    
    // String Escaping Tests
    printf(COLOR_YELLOW "String Escaping Tests:" COLOR_RESET "\n");
    RUN_TEST(test_escape_sql_string_standard);
    RUN_TEST(test_escape_sql_string_mysql);
    RUN_TEST(test_unescape_sql_string);
    
    // Identifier Tests
    printf("\n" COLOR_YELLOW "Identifier Tests:" COLOR_RESET "\n");
    RUN_TEST(test_quote_identifier);
    RUN_TEST(test_validate_identifier);
    
    // Type Mapping Tests
    printf("\n" COLOR_YELLOW "Type Mapping Tests:" COLOR_RESET "\n");
    RUN_TEST(test_type_mapping_sqlite);
    RUN_TEST(test_type_mapping_postgresql);
    RUN_TEST(test_type_mapping_mysql);
    
    // SQL Builder Tests
    printf("\n" COLOR_YELLOW "SQL Builder Tests:" COLOR_RESET "\n");
    RUN_TEST(test_builder_creation);
    RUN_TEST(test_dialect_names);
    RUN_TEST(test_placeholders);
    RUN_TEST(test_operator_to_sql);
    
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
