/* Test Suite: SQL Generation
 * 
 * Tests for SQL builder, escaping, and dialect-specific features.
 */

#define _GNU_SOURCE
#include "src/core/model.h"
#include "src/core/field_types.h"
#include "src/core/queryset.h"
#include "src/core/sql_builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

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

// Helper to create a simple user model
static ORM_Model* CreateUserModel() {
    ORM_Model* model = ORM_CreateModel("users");
    
    // Add primary key
    ORM_Field* id_field = ORM_CreateField("id", FIELD_INT);
    id_field->is_primary_key = true;
    id_field->auto_increment = true;
    ORM_AddField(model, id_field);
    
    // Add fields
    ORM_Field* name = ORM_CreateField("name", FIELD_VARCHAR);
    name->max_length = 100;
    name->nullable = false;
    ORM_AddField(model, name);
    
    ORM_Field* email = ORM_CreateField("email", FIELD_VARCHAR);
    email->max_length = 255;
    email->unique = true;
    ORM_AddField(model, email);
    
    ORM_Field* age = ORM_CreateField("age", FIELD_INT);
    age->nullable = true;
    ORM_AddField(model, age);
    
    ORM_Field* active = ORM_CreateField("active", FIELD_BOOL);
    active->has_default = true;
    active->default_value.bool_val = true;
    ORM_AddField(model, active);
    
    return model;
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

TEST(test_parameter_binding) {
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_SQLITE);
    
    ORM_Value val1;
    val1.int_val = 42;
    int idx1 = ORM_AddParameter(builder, &val1, FIELD_INT, 0);
    ASSERT(idx1 == 0, "First parameter index");
    ASSERT(builder->param_count == 1, "Parameter count");
    
    ORM_Value val2;
    val2.string_val = "test";
    int idx2 = ORM_AddParameter(builder, &val2, FIELD_VARCHAR, 4);
    ASSERT(idx2 == 1, "Second parameter index");
    ASSERT(builder->param_count == 2, "Parameter count");
    
    // Check stored values
    ASSERT(builder->parameters[0].value.int_val == 42, "First parameter value");
    ASSERT(strcmp(builder->parameters[1].value.string_val, "test") == 0, "Second parameter value");
    
    ORM_FreeSQLBuilder(builder);
}

// ============================================================================
// String Escaping Tests
// ============================================================================

TEST(test_escape_string_standard_sql) {
    char* escaped;
    
    // Single quote doubling
    escaped = ORM_EscapeString("O'Reilly", ORM_DIALECT_SQLITE);
    ASSERT(strcmp(escaped, "O''Reilly") == 0, "Single quote escaped");
    free(escaped);
    
    // Multiple quotes
    escaped = ORM_EscapeString("It's John's book", ORM_DIALECT_POSTGRESQL);
    ASSERT(strcmp(escaped, "It''s John''s book") == 0, "Multiple quotes escaped");
    free(escaped);
    
    // No quotes
    escaped = ORM_EscapeString("Simple text", ORM_DIALECT_SQLITE);
    ASSERT(strcmp(escaped, "Simple text") == 0, "No escaping needed");
    free(escaped);
}

TEST(test_escape_string_mysql) {
    char* escaped;
    
    // Newline
    escaped = ORM_EscapeString("Line 1\nLine 2", ORM_DIALECT_MYSQL);
    ASSERT(strcmp(escaped, "Line 1\\nLine 2") == 0, "Newline escaped");
    free(escaped);
    
    // Backslash
    escaped = ORM_EscapeString("C:\\path\\to\\file", ORM_DIALECT_MYSQL);
    ASSERT(strcmp(escaped, "C:\\\\path\\\\to\\\\file") == 0, "Backslash escaped");
    free(escaped);
    
    // Single quote
    escaped = ORM_EscapeString("O'Reilly", ORM_DIALECT_MYSQL);
    ASSERT(strcmp(escaped, "O\\'Reilly") == 0, "Single quote escaped");
    free(escaped);
}

TEST(test_unescape_string) {
    char* unescaped;
    
    // Standard SQL
    unescaped = ORM_UnescapeString("O''Reilly", ORM_DIALECT_SQLITE);
    ASSERT(strcmp(unescaped, "O'Reilly") == 0, "Single quote unescaped");
    free(unescaped);
    
    // MySQL
    unescaped = ORM_UnescapeString("Line 1\\nLine 2", ORM_DIALECT_MYSQL);
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
    ASSERT(strcmp(ORM_GetSQLTypeName(FIELD_INT, ORM_DIALECT_SQLITE), "INTEGER") == 0, "INT -> INTEGER");
    ASSERT(strcmp(ORM_GetSQLTypeName(FIELD_FLOAT, ORM_DIALECT_SQLITE), "REAL") == 0, "FLOAT -> REAL");
    ASSERT(strcmp(ORM_GetSQLTypeName(FIELD_VARCHAR, ORM_DIALECT_SQLITE), "VARCHAR") == 0, "VARCHAR -> VARCHAR");
    ASSERT(strcmp(ORM_GetSQLTypeName(FIELD_TEXT, ORM_DIALECT_SQLITE), "TEXT") == 0, "TEXT -> TEXT");
    ASSERT(strcmp(ORM_GetSQLTypeName(FIELD_BLOB, ORM_DIALECT_SQLITE), "BLOB") == 0, "BLOB -> BLOB");
}

TEST(test_type_mapping_postgresql) {
    ASSERT(strcmp(ORM_GetSQLTypeName(FIELD_INT, ORM_DIALECT_POSTGRESQL), "INTEGER") == 0, "INT -> INTEGER");
    ASSERT(strcmp(ORM_GetSQLTypeName(FIELD_DOUBLE, ORM_DIALECT_POSTGRESQL), "DOUBLE PRECISION") == 0, "DOUBLE -> DOUBLE PRECISION");
    ASSERT(strcmp(ORM_GetSQLTypeName(FIELD_BOOL, ORM_DIALECT_POSTGRESQL), "BOOLEAN") == 0, "BOOL -> BOOLEAN");
    ASSERT(strcmp(ORM_GetSQLTypeName(FIELD_DATETIME, ORM_DIALECT_POSTGRESQL), "TIMESTAMP") == 0, "DATETIME -> TIMESTAMP");
    ASSERT(strcmp(ORM_GetSQLTypeName(FIELD_BLOB, ORM_DIALECT_POSTGRESQL), "BYTEA") == 0, "BLOB -> BYTEA");
}

TEST(test_type_mapping_mysql) {
    ASSERT(strcmp(ORM_GetSQLTypeName(FIELD_INT, ORM_DIALECT_MYSQL), "INT") == 0, "INT -> INT");
    ASSERT(strcmp(ORM_GetSQLTypeName(FIELD_BOOL, ORM_DIALECT_MYSQL), "TINYINT(1)") == 0, "BOOL -> TINYINT(1)");
    ASSERT(strcmp(ORM_GetSQLTypeName(FIELD_DATETIME, ORM_DIALECT_MYSQL), "DATETIME") == 0, "DATETIME -> DATETIME");
    ASSERT(strcmp(ORM_GetSQLTypeName(FIELD_BLOB, ORM_DIALECT_MYSQL), "BLOB") == 0, "BLOB -> BLOB");
}

// ============================================================================
// SELECT SQL Tests
// ============================================================================

TEST(test_select_simple) {
    ORM_Model* model = CreateUserModel();
    ORM_QuerySet* qs = ORM_CreateQuerySet(model);
    
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_SQLITE);
    const char* sql = ORM_BuildSelectSQL(builder, qs);
    
    ASSERT(sql != NULL, "SQL generated");
    ASSERT(strstr(sql, "SELECT") != NULL, "Contains SELECT");
    ASSERT(strstr(sql, "FROM \"users\"") != NULL, "Contains FROM with quoted table");
    
    ORM_FreeSQLBuilder(builder);
    ORM_FreeQuerySet(qs);
    ORM_FreeModel(model);
}

TEST(test_select_with_fields) {
    ORM_Model* model = CreateUserModel();
    ORM_QuerySet* qs = ORM_CreateQuerySet(model);
    
    const char* fields[] = {"id", "name", "email"};
    ORM_Select(qs, fields, 3);
    
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_SQLITE);
    const char* sql = ORM_BuildSelectSQL(builder, qs);
    
    ASSERT(strstr(sql, "\"id\"") != NULL, "Contains id field");
    ASSERT(strstr(sql, "\"name\"") != NULL, "Contains name field");
    ASSERT(strstr(sql, "\"email\"") != NULL, "Contains email field");
    
    ORM_FreeSQLBuilder(builder);
    ORM_FreeQuerySet(qs);
    ORM_FreeModel(model);
}

TEST(test_select_with_where) {
    ORM_Model* model = CreateUserModel();
    ORM_QuerySet* qs = ORM_CreateQuerySet(model);
    
    ORM_Value val;
    val.int_val = 18;
    ORM_Filter(qs, "age", OP_GREATER_THAN, &val, FIELD_INT);
    
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_SQLITE);
    const char* sql = ORM_BuildSelectSQL(builder, qs);
    
    ASSERT(strstr(sql, "WHERE") != NULL, "Contains WHERE");
    ASSERT(strstr(sql, "\"age\" >") != NULL, "Contains age filter");
    ASSERT(builder->param_count == 1, "Has one parameter");
    
    ORM_FreeSQLBuilder(builder);
    ORM_FreeQuerySet(qs);
    ORM_FreeModel(model);
}

TEST(test_select_with_order) {
    ORM_Model* model = CreateUserModel();
    ORM_QuerySet* qs = ORM_CreateQuerySet(model);
    
    const char* order_fields[] = {"name", "-age"};
    ORM_OrderBy(qs, order_fields, 2);
    
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_SQLITE);
    const char* sql = ORM_BuildSelectSQL(builder, qs);
    
    ASSERT(strstr(sql, "ORDER BY") != NULL, "Contains ORDER BY");
    ASSERT(strstr(sql, "\"name\"") != NULL, "Contains name field");
    ASSERT(strstr(sql, "\"age\" DESC") != NULL, "Contains descending age");
    
    ORM_FreeSQLBuilder(builder);
    ORM_FreeQuerySet(qs);
    ORM_FreeModel(model);
}

TEST(test_select_with_limit) {
    ORM_Model* model = CreateUserModel();
    ORM_QuerySet* qs = ORM_CreateQuerySet(model);
    
    ORM_Limit(qs, 10);
    ORM_Offset(qs, 5);
    
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_SQLITE);
    const char* sql = ORM_BuildSelectSQL(builder, qs);
    
    ASSERT(strstr(sql, "LIMIT 10") != NULL, "Contains LIMIT");
    ASSERT(strstr(sql, "OFFSET 5") != NULL, "Contains OFFSET");
    
    ORM_FreeSQLBuilder(builder);
    ORM_FreeQuerySet(qs);
    ORM_FreeModel(model);
}

TEST(test_select_distinct) {
    ORM_Model* model = CreateUserModel();
    ORM_QuerySet* qs = ORM_CreateQuerySet(model);
    
    ORM_Distinct(qs);
    
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_SQLITE);
    const char* sql = ORM_BuildSelectSQL(builder, qs);
    
    ASSERT(strstr(sql, "SELECT DISTINCT") != NULL, "Contains DISTINCT");
    
    ORM_FreeSQLBuilder(builder);
    ORM_FreeQuerySet(qs);
    ORM_FreeModel(model);
}

// ============================================================================
// INSERT SQL Tests
// ============================================================================

TEST(test_insert_simple) {
    ORM_Model* model = CreateUserModel();
    
    ORM_ModelInstance* inst = ORM_CreateInstance(model);
    ORM_SetFieldString(inst, "name", "John Doe");
    ORM_SetFieldString(inst, "email", "john@example.com");
    ORM_SetFieldInt(inst, "age", 30);
    
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_SQLITE);
    const char* sql = ORM_BuildInsertSQL(builder, inst);
    
    ASSERT(sql != NULL, "SQL generated");
    ASSERT(strstr(sql, "INSERT INTO \"users\"") != NULL, "Contains INSERT INTO");
    ASSERT(strstr(sql, "VALUES") != NULL, "Contains VALUES");
    ASSERT(builder->param_count == 4, "Has 4 parameters (name, email, age, active)");
    
    ORM_FreeSQLBuilder(builder);
    ORM_FreeInstance(inst);
    ORM_FreeModel(model);
}

TEST(test_insert_skips_autoincrement) {
    ORM_Model* model = CreateUserModel();
    
    ORM_ModelInstance* inst = ORM_CreateInstance(model);
    ORM_SetFieldString(inst, "name", "Jane Doe");
    
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_SQLITE);
    const char* sql = ORM_BuildInsertSQL(builder, inst);
    
    // Should not include 'id' field since it's auto increment
    ASSERT(strstr(sql, "\"id\"") == NULL, "Does not include id field");
    
    ORM_FreeSQLBuilder(builder);
    ORM_FreeInstance(inst);
    ORM_FreeModel(model);
}

// ============================================================================
// UPDATE SQL Tests
// ============================================================================

TEST(test_update_simple) {
    ORM_Model* model = CreateUserModel();
    
    ORM_ModelInstance* inst = ORM_CreateInstance(model);
    ORM_SetFieldInt(inst, "id", 1);
    ORM_SetFieldString(inst, "name", "Updated Name");
    ORM_SetFieldInt(inst, "age", 35);
    
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_SQLITE);
    const char* sql = ORM_BuildUpdateSQL(builder, inst);
    
    ASSERT(sql != NULL, "SQL generated");
    ASSERT(strstr(sql, "UPDATE \"users\"") != NULL, "Contains UPDATE");
    ASSERT(strstr(sql, "SET") != NULL, "Contains SET");
    ASSERT(strstr(sql, "WHERE \"id\" =") != NULL, "Contains WHERE with primary key");
    
    ORM_FreeSQLBuilder(builder);
    ORM_FreeInstance(inst);
    ORM_FreeModel(model);
}

// ============================================================================
// DELETE SQL Tests
// ============================================================================

TEST(test_delete_simple) {
    ORM_Model* model = CreateUserModel();
    ORM_QuerySet* qs = ORM_CreateQuerySet(model);
    
    ORM_Value val;
    val.int_val = 1;
    ORM_Filter(qs, "id", OP_EQUALS, &val, FIELD_INT);
    
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_SQLITE);
    const char* sql = ORM_BuildDeleteSQL(builder, qs);
    
    ASSERT(sql != NULL, "SQL generated");
    ASSERT(strstr(sql, "DELETE FROM \"users\"") != NULL, "Contains DELETE FROM");
    ASSERT(strstr(sql, "WHERE") != NULL, "Contains WHERE");
    
    ORM_FreeSQLBuilder(builder);
    ORM_FreeQuerySet(qs);
    ORM_FreeModel(model);
}

// ============================================================================
// CREATE TABLE Tests
// ============================================================================

TEST(test_create_table) {
    ORM_Model* model = CreateUserModel();
    
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_SQLITE);
    const char* sql = ORM_BuildCreateTableSQL(builder, model);
    
    ASSERT(sql != NULL, "SQL generated");
    ASSERT(strstr(sql, "CREATE TABLE \"users\"") != NULL, "Contains CREATE TABLE");
    ASSERT(strstr(sql, "\"id\" INTEGER PRIMARY KEY AUTOINCREMENT") != NULL, "Contains id definition");
    ASSERT(strstr(sql, "\"name\" VARCHAR(100) NOT NULL") != NULL, "Contains name definition");
    ASSERT(strstr(sql, "\"email\" VARCHAR(255) UNIQUE") != NULL, "Contains email with UNIQUE");
    
    ORM_FreeSQLBuilder(builder);
    ORM_FreeModel(model);
}

TEST(test_create_table_postgresql) {
    ORM_Model* model = CreateUserModel();
    
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_POSTGRESQL);
    const char* sql = ORM_BuildCreateTableSQL(builder, model);
    
    ASSERT(strstr(sql, "\"users\"") != NULL, "PostgreSQL uses double quotes");
    
    ORM_FreeSQLBuilder(builder);
    ORM_FreeModel(model);
}

TEST(test_create_table_mysql) {
    ORM_Model* model = CreateUserModel();
    
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_MYSQL);
    const char* sql = ORM_BuildCreateTableSQL(builder, model);
    
    ASSERT(strstr(sql, "`users`") != NULL, "MySQL uses backticks");
    ASSERT(strstr(sql, "AUTO_INCREMENT") != NULL, "MySQL uses AUTO_INCREMENT");
    
    ORM_FreeSQLBuilder(builder);
    ORM_FreeModel(model);
}

// ============================================================================
// DROP TABLE Tests
// ============================================================================

TEST(test_drop_table) {
    ORM_Model* model = CreateUserModel();
    
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_SQLITE);
    const char* sql = ORM_BuildDropTableSQL(builder, model);
    
    ASSERT(sql != NULL, "SQL generated");
    ASSERT(strstr(sql, "DROP TABLE IF EXISTS \"users\"") != NULL, "Contains DROP TABLE IF EXISTS");
    
    ORM_FreeSQLBuilder(builder);
    ORM_FreeModel(model);
}

// ============================================================================
// Operator Conversion Tests
// ============================================================================

TEST(test_operator_to_sql) {
    ASSERT(strcmp(ORM_OperatorToSQL(OP_EQUALS), "=") == 0, "EQUALS -> =");
    ASSERT(strcmp(ORM_OperatorToSQL(OP_NOT_EQUALS), "!=") == 0, "NOT_EQUALS -> !=");
    ASSERT(strcmp(ORM_OperatorToSQL(OP_LESS_THAN), "<") == 0, "LESS_THAN -> <");
    ASSERT(strcmp(ORM_OperatorToSQL(OP_GREATER_THAN), ">") == 0, "GREATER_THAN -> >");
    ASSERT(strcmp(ORM_OperatorToSQL(OP_LIKE), "LIKE") == 0, "LIKE -> LIKE");
    ASSERT(strcmp(ORM_OperatorToSQL(OP_IS_NULL), "IS NULL") == 0, "IS_NULL -> IS NULL");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    printf("\n" COLOR_YELLOW "=== SQL Generation Test Suite ===" COLOR_RESET "\n\n");
    
    // SQL Builder Tests
    printf(COLOR_YELLOW "SQL Builder Tests:" COLOR_RESET "\n");
    RUN_TEST(test_builder_creation);
    RUN_TEST(test_dialect_names);
    RUN_TEST(test_placeholders);
    RUN_TEST(test_parameter_binding);
    
    // String Escaping Tests
    printf("\n" COLOR_YELLOW "String Escaping Tests:" COLOR_RESET "\n");
    RUN_TEST(test_escape_string_standard_sql);
    RUN_TEST(test_escape_string_mysql);
    RUN_TEST(test_unescape_string);
    
    // Identifier Tests
    printf("\n" COLOR_YELLOW "Identifier Tests:" COLOR_RESET "\n");
    RUN_TEST(test_quote_identifier);
    RUN_TEST(test_validate_identifier);
    
    // Type Mapping Tests
    printf("\n" COLOR_YELLOW "Type Mapping Tests:" COLOR_RESET "\n");
    RUN_TEST(test_type_mapping_sqlite);
    RUN_TEST(test_type_mapping_postgresql);
    RUN_TEST(test_type_mapping_mysql);
    
    // SELECT Tests
    printf("\n" COLOR_YELLOW "SELECT SQL Tests:" COLOR_RESET "\n");
    RUN_TEST(test_select_simple);
    RUN_TEST(test_select_with_fields);
    RUN_TEST(test_select_with_where);
    RUN_TEST(test_select_with_order);
    RUN_TEST(test_select_with_limit);
    RUN_TEST(test_select_distinct);
    
    // INSERT Tests
    printf("\n" COLOR_YELLOW "INSERT SQL Tests:" COLOR_RESET "\n");
    RUN_TEST(test_insert_simple);
    RUN_TEST(test_insert_skips_autoincrement);
    
    // UPDATE Tests
    printf("\n" COLOR_YELLOW "UPDATE SQL Tests:" COLOR_RESET "\n");
    RUN_TEST(test_update_simple);
    
    // DELETE Tests
    printf("\n" COLOR_YELLOW "DELETE SQL Tests:" COLOR_RESET "\n");
    RUN_TEST(test_delete_simple);
    
    // CREATE TABLE Tests
    printf("\n" COLOR_YELLOW "CREATE TABLE Tests:" COLOR_RESET "\n");
    RUN_TEST(test_create_table);
    RUN_TEST(test_create_table_postgresql);
    RUN_TEST(test_create_table_mysql);
    
    // DROP TABLE Tests
    printf("\n" COLOR_YELLOW "DROP TABLE Tests:" COLOR_RESET "\n");
    RUN_TEST(test_drop_table);
    
    // Operator Tests
    printf("\n" COLOR_YELLOW "Operator Conversion Tests:" COLOR_RESET "\n");
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
