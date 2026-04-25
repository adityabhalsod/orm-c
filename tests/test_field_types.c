#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "../src/core/field_types.h"

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
// Type Information Tests
// ============================================================================

void test_type_information(void) {
    TEST_START("Type Information Functions");
    
    TEST_ASSERT(ORM_IsNumericType(ORM_TYPE_INT), "INT is numeric");
    TEST_ASSERT(ORM_IsNumericType(ORM_TYPE_BIGINT), "BIGINT is numeric");
    TEST_ASSERT(ORM_IsNumericType(ORM_TYPE_FLOAT), "FLOAT is numeric");
    TEST_ASSERT(ORM_IsNumericType(ORM_TYPE_DOUBLE), "DOUBLE is numeric");
    TEST_ASSERT(!ORM_IsNumericType(ORM_TYPE_STRING), "STRING is not numeric");
    
    TEST_ASSERT(ORM_IsStringType(ORM_TYPE_STRING), "STRING is string type");
    TEST_ASSERT(ORM_IsStringType(ORM_TYPE_TEXT), "TEXT is string type");
    TEST_ASSERT(!ORM_IsStringType(ORM_TYPE_INT), "INT is not string type");
    
    TEST_ASSERT(ORM_IsTemporalType(ORM_TYPE_DATE), "DATE is temporal");
    TEST_ASSERT(ORM_IsTemporalType(ORM_TYPE_TIME), "TIME is temporal");
    TEST_ASSERT(ORM_IsTemporalType(ORM_TYPE_DATETIME), "DATETIME is temporal");
    TEST_ASSERT(!ORM_IsTemporalType(ORM_TYPE_INT), "INT is not temporal");
    
    TEST_END();
}

void test_sql_type_names(void) {
    TEST_START("SQL Type Names");
    
    TEST_ASSERT(strcmp(ORM_GetSQLTypeName(ORM_TYPE_INT, NULL), "INTEGER") == 0,
                "INT -> INTEGER");
    TEST_ASSERT(strcmp(ORM_GetSQLTypeName(ORM_TYPE_BIGINT, NULL), "BIGINT") == 0,
                "BIGINT -> BIGINT");
    TEST_ASSERT(strcmp(ORM_GetSQLTypeName(ORM_TYPE_STRING, NULL), "VARCHAR") == 0,
                "STRING -> VARCHAR");
    TEST_ASSERT(strcmp(ORM_GetSQLTypeName(ORM_TYPE_TEXT, NULL), "TEXT") == 0,
                "TEXT -> TEXT");
    TEST_ASSERT(strcmp(ORM_GetSQLTypeName(ORM_TYPE_DATETIME, NULL), "DATETIME") == 0,
                "DATETIME -> DATETIME");
    
    TEST_END();
}

void test_field_sizes(void) {
    TEST_START("Field Size Calculation");
    
    TEST_ASSERT(ORM_GetFieldSize(ORM_TYPE_INT, NULL) == sizeof(int),
                "INT size is correct");
    TEST_ASSERT(ORM_GetFieldSize(ORM_TYPE_BIGINT, NULL) == sizeof(int64_t),
                "BIGINT size is correct");
    TEST_ASSERT(ORM_GetFieldSize(ORM_TYPE_FLOAT, NULL) == sizeof(float),
                "FLOAT size is correct");
    TEST_ASSERT(ORM_GetFieldSize(ORM_TYPE_DOUBLE, NULL) == sizeof(double),
                "DOUBLE size is correct");
    
    TEST_END();
}

// ============================================================================
// Type Conversion Tests
// ============================================================================

void test_int_conversion(void) {
    TEST_START("Integer Type Conversion");
    
    int value = 42;
    char buffer[32];
    
    int result = ORM_ConvertToSQL(ORM_TYPE_INT, &value, buffer, sizeof(buffer));
    TEST_ASSERT(result > 0, "INT conversion to SQL succeeds");
    TEST_ASSERT(strcmp(buffer, "42") == 0, "INT converts to '42'");
    
    int converted;
    result = ORM_ConvertFromSQL(ORM_TYPE_INT, "42", &converted);
    TEST_ASSERT(result == ORM_SUCCESS, "INT conversion from SQL succeeds");
    TEST_ASSERT(converted == 42, "INT converts from '42'");
    
    // Test negative numbers
    value = -123;
    ORM_ConvertToSQL(ORM_TYPE_INT, &value, buffer, sizeof(buffer));
    TEST_ASSERT(strcmp(buffer, "-123") == 0, "Negative INT converts correctly");
    
    TEST_END();
}

void test_bigint_conversion(void) {
    TEST_START("BIGINT Type Conversion");
    
    int64_t value = 1234567890123LL;
    char buffer[32];
    
    int result = ORM_ConvertToSQL(ORM_TYPE_BIGINT, &value, buffer, sizeof(buffer));
    TEST_ASSERT(result > 0, "BIGINT conversion to SQL succeeds");
    TEST_ASSERT(strcmp(buffer, "1234567890123") == 0, "BIGINT converts correctly");
    
    int64_t converted;
    result = ORM_ConvertFromSQL(ORM_TYPE_BIGINT, "1234567890123", &converted);
    TEST_ASSERT(result == ORM_SUCCESS, "BIGINT conversion from SQL succeeds");
    TEST_ASSERT(converted == 1234567890123LL, "BIGINT converts from SQL correctly");
    
    TEST_END();
}

void test_float_conversion(void) {
    TEST_START("FLOAT Type Conversion");
    
    float value = 3.14159f;
    char buffer[32];
    
    int result = ORM_ConvertToSQL(ORM_TYPE_FLOAT, &value, buffer, sizeof(buffer));
    TEST_ASSERT(result > 0, "FLOAT conversion to SQL succeeds");
    
    float converted;
    ORM_ConvertFromSQL(ORM_TYPE_FLOAT, "3.14159", &converted);
    TEST_ASSERT(fabs(converted - 3.14159f) < 0.00001f, "FLOAT converts from SQL correctly");
    
    TEST_END();
}

void test_double_conversion(void) {
    TEST_START("DOUBLE Type Conversion");
    
    double value = 3.141592653589793;
    char buffer[32];
    
    int result = ORM_ConvertToSQL(ORM_TYPE_DOUBLE, &value, buffer, sizeof(buffer));
    TEST_ASSERT(result > 0, "DOUBLE conversion to SQL succeeds");
    
    double converted;
    ORM_ConvertFromSQL(ORM_TYPE_DOUBLE, "3.141592653589793", &converted);
    TEST_ASSERT(fabs(converted - 3.141592653589793) < 0.000000000000001, 
                "DOUBLE converts from SQL correctly");
    
    TEST_END();
}

void test_boolean_conversion(void) {
    TEST_START("BOOLEAN Type Conversion");
    
    int true_val = 1;
    int false_val = 0;
    char buffer[32];
    
    ORM_ConvertToSQL(ORM_TYPE_BOOLEAN, &true_val, buffer, sizeof(buffer));
    TEST_ASSERT(strcmp(buffer, "1") == 0, "TRUE converts to '1'");
    
    ORM_ConvertToSQL(ORM_TYPE_BOOLEAN, &false_val, buffer, sizeof(buffer));
    TEST_ASSERT(strcmp(buffer, "0") == 0, "FALSE converts to '0'");
    
    int converted;
    ORM_ConvertFromSQL(ORM_TYPE_BOOLEAN, "1", &converted);
    TEST_ASSERT(converted == 1, "Boolean '1' converts to 1");
    
    ORM_ConvertFromSQL(ORM_TYPE_BOOLEAN, "0", &converted);
    TEST_ASSERT(converted == 0, "Boolean '0' converts to 0");
    
    TEST_END();
}

void test_string_conversion(void) {
    TEST_START("STRING Type Conversion");
    
    char value[] = "Hello, World!";
    char buffer[128];
    
    int result = ORM_ConvertToSQL(ORM_TYPE_STRING, value, buffer, sizeof(buffer));
    TEST_ASSERT(result > 0, "STRING conversion to SQL succeeds");
    TEST_ASSERT(strcmp(buffer, "'Hello, World!'") == 0, 
                "STRING converts with quotes");
    
    char converted[128];
    result = ORM_ConvertFromSQL(ORM_TYPE_STRING, "'Hello, World!'", converted);
    TEST_ASSERT(result == ORM_SUCCESS, "STRING conversion from SQL succeeds");
    TEST_ASSERT(strcmp(converted, "Hello, World!") == 0, 
                "STRING converts from SQL correctly");
    
    TEST_END();
}

void test_string_escaping(void) {
    TEST_START("String Escaping");
    
    char input[] = "O'Brien";
    char escaped[64];
    
    int result = ORM_EscapeString(input, escaped, sizeof(escaped));
    TEST_ASSERT(result > 0, "String escaping succeeds");
    TEST_ASSERT(strcmp(escaped, "O''Brien") == 0, "Single quote escaped correctly");
    
    char unescaped[64];
    result = ORM_UnescapeString("O''Brien", unescaped, sizeof(unescaped));
    TEST_ASSERT(result > 0, "String unescaping succeeds");
    TEST_ASSERT(strcmp(unescaped, "O'Brien") == 0, "Single quote unescaped correctly");
    
    // Test backslash escaping
    char input2[] = "Path\\To\\File";
    ORM_EscapeString(input2, escaped, sizeof(escaped));
    TEST_ASSERT(strcmp(escaped, "Path\\\\To\\\\File") == 0, 
                "Backslash escaped correctly");
    
    TEST_END();
}

void test_null_conversion(void) {
    TEST_START("NULL Value Conversion");
    
    char buffer[32];
    
    int result = ORM_ConvertToSQL(ORM_TYPE_INT, NULL, buffer, sizeof(buffer));
    TEST_ASSERT(result > 0, "NULL conversion succeeds");
    TEST_ASSERT(strcmp(buffer, "NULL") == 0, "NULL converts to 'NULL'");
    
    char empty_string[] = "";
    result = ORM_ConvertToSQL(ORM_TYPE_STRING, empty_string, buffer, sizeof(buffer));
    TEST_ASSERT(strcmp(buffer, "NULL") == 0, "Empty string converts to NULL");
    
    TEST_END();
}

// ============================================================================
// Validation Tests
// ============================================================================

void test_string_validation(void) {
    TEST_START("String Validation");
    
    char* valid_str = "test";
    TEST_ASSERT(ORM_ValidateString(valid_str, 10, 0), 
                "Valid string passes validation");
    
    char* long_str = "this is a very long string";
    TEST_ASSERT(!ORM_ValidateString(long_str, 10, 0), 
                "Too long string fails validation");
    
    char* null_str = NULL;
    TEST_ASSERT(!ORM_ValidateString(null_str, 10, ORM_NOT_NULL), 
                "NULL fails NOT_NULL validation");
    
    char* empty_str = "";
    TEST_ASSERT(!ORM_ValidateString(empty_str, 10, ORM_NOT_NULL), 
                "Empty string fails NOT_NULL validation");
    
    TEST_END();
}

void test_numeric_validation(void) {
    TEST_START("Numeric Validation");
    
    ORM_NumericField field = {
        .base_type = ORM_TYPE_INT,
        .min_value = 0,
        .max_value = 100
    };
    
    int valid_val = 50;
    TEST_ASSERT(ORM_ValidateNumeric(&valid_val, &field), 
                "Valid numeric value passes");
    
    int too_low = -5;
    TEST_ASSERT(!ORM_ValidateNumeric(&too_low, &field), 
                "Too low value fails validation");
    
    int too_high = 150;
    TEST_ASSERT(!ORM_ValidateNumeric(&too_high, &field), 
                "Too high value fails validation");
    
    int min_val = 0;
    TEST_ASSERT(ORM_ValidateNumeric(&min_val, &field), 
                "Minimum value passes");
    
    int max_val = 100;
    TEST_ASSERT(ORM_ValidateNumeric(&max_val, &field), 
                "Maximum value passes");
    
    TEST_END();
}

void test_field_validation(void) {
    TEST_START("Field Validation");
    
    char* valid_str = "test";
    TEST_ASSERT(ORM_ValidateField(ORM_TYPE_STRING, valid_str, 0), 
                "Valid field passes validation");
    
    char* null_str = NULL;
    TEST_ASSERT(!ORM_ValidateField(ORM_TYPE_STRING, null_str, ORM_NOT_NULL), 
                "NULL fails NOT_NULL constraint");
    
    TEST_END();
}

// ============================================================================
// DateTime Tests
// ============================================================================

void test_datetime_parsing(void) {
    TEST_START("DateTime Parsing");
    
    // Test full datetime
    ORM_DateTime* dt = ORM_ParseDateTime("2024-01-15 14:30:00");
    TEST_ASSERT(dt != NULL, "DateTime parsing succeeds");
    TEST_ASSERT(dt->year == 2024, "Year parsed correctly");
    TEST_ASSERT(dt->month == 1, "Month parsed correctly");
    TEST_ASSERT(dt->day == 15, "Day parsed correctly");
    TEST_ASSERT(dt->hour == 14, "Hour parsed correctly");
    TEST_ASSERT(dt->minute == 30, "Minute parsed correctly");
    TEST_ASSERT(dt->second == 0, "Second parsed correctly");
    ORM_FreeDateTime(dt);
    
    // Test date only
    dt = ORM_ParseDateTime("2024-12-25");
    TEST_ASSERT(dt != NULL, "Date-only parsing succeeds");
    TEST_ASSERT(dt->year == 2024, "Date year correct");
    TEST_ASSERT(dt->month == 12, "Date month correct");
    TEST_ASSERT(dt->day == 25, "Date day correct");
    TEST_ASSERT(dt->hour == 0, "Time defaults to 00:00:00");
    ORM_FreeDateTime(dt);
    
    // Test time only
    dt = ORM_ParseDateTime("09:45:30");
    TEST_ASSERT(dt != NULL, "Time-only parsing succeeds");
    TEST_ASSERT(dt->hour == 9, "Time hour correct");
    TEST_ASSERT(dt->minute == 45, "Time minute correct");
    TEST_ASSERT(dt->second == 30, "Time second correct");
    ORM_FreeDateTime(dt);
    
    TEST_END();
}

void test_datetime_formatting(void) {
    TEST_START("DateTime Formatting");
    
    ORM_DateTime dt = {
        .year = 2024,
        .month = 3,
        .day = 15,
        .hour = 9,
        .minute = 30,
        .second = 45,
        .microsecond = 0
    };
    
    char buffer[64];
    
    // Test default format
    int result = ORM_FormatDateTime(&dt, NULL, buffer, sizeof(buffer));
    TEST_ASSERT(result > 0, "DateTime formatting succeeds");
    TEST_ASSERT(strcmp(buffer, "2024-03-15 09:30:45") == 0, 
                "Default format is correct");
    
    // Test custom format
    result = ORM_FormatDateTime(&dt, "%Y-%m-%d", buffer, sizeof(buffer));
    TEST_ASSERT(result > 0, "Custom formatting succeeds");
    TEST_ASSERT(strcmp(buffer, "2024-03-15") == 0, 
                "Custom format is correct");
    
    TEST_END();
}

void test_datetime_validation(void) {
    TEST_START("DateTime Validation");
    
    ORM_DateTime valid_dt = {2024, 3, 15, 14, 30, 45, 0};
    TEST_ASSERT(ORM_ValidateDateTime(&valid_dt), "Valid datetime passes");
    
    ORM_DateTime invalid_month = {2024, 13, 15, 14, 30, 45, 0};
    TEST_ASSERT(!ORM_ValidateDateTime(&invalid_month), "Invalid month fails");
    
    ORM_DateTime invalid_day = {2024, 3, 32, 14, 30, 45, 0};
    TEST_ASSERT(!ORM_ValidateDateTime(&invalid_day), "Invalid day fails");
    
    ORM_DateTime invalid_hour = {2024, 3, 15, 24, 30, 45, 0};
    TEST_ASSERT(!ORM_ValidateDateTime(&invalid_hour), "Invalid hour fails");
    
    TEST_END();
}

void test_datetime_conversion(void) {
    TEST_START("DateTime Timestamp Conversion");
    
    ORM_DateTime dt = {2024, 1, 1, 0, 0, 0, 0};
    
    time_t timestamp = ORM_DateTimeToTimestamp(&dt);
    TEST_ASSERT(timestamp > 0, "DateTime to timestamp succeeds");
    
    ORM_DateTime* converted = ORM_TimestampToDateTime(timestamp);
    TEST_ASSERT(converted != NULL, "Timestamp to DateTime succeeds");
    TEST_ASSERT(converted->year == 2024, "Converted year is correct");
    TEST_ASSERT(converted->month == 1, "Converted month is correct");
    TEST_ASSERT(converted->day == 1, "Converted day is correct");
    
    ORM_FreeDateTime(converted);
    
    TEST_END();
}

void test_current_datetime(void) {
    TEST_START("Current DateTime");
    
    ORM_DateTime* now = ORM_GetCurrentDateTime();
    TEST_ASSERT(now != NULL, "Getting current datetime succeeds");
    TEST_ASSERT(now->year >= 2024, "Current year is reasonable");
    TEST_ASSERT(now->month >= 1 && now->month <= 12, "Current month is valid");
    TEST_ASSERT(now->day >= 1 && now->day <= 31, "Current day is valid");
    
    ORM_FreeDateTime(now);
    
    TEST_END();
}

// ============================================================================
// Binary Data Tests
// ============================================================================

void test_binary_operations(void) {
    TEST_START("Binary Data Operations");
    
    unsigned char data[] = {0x01, 0x02, 0x03, 0x04, 0xFF};
    ORM_BinaryField* field = ORM_CreateBinaryField(data, sizeof(data));
    
    TEST_ASSERT(field != NULL, "Binary field creation succeeds");
    TEST_ASSERT(field->actual_size == sizeof(data), "Binary size is correct");
    TEST_ASSERT(memcmp(field->data, data, sizeof(data)) == 0, 
                "Binary data copied correctly");
    
    ORM_FreeBinaryField(field);
    TEST_ASSERT(1, "Binary field freed without crash");
    
    TEST_END();
}

void test_binary_hex_conversion(void) {
    TEST_START("Binary to Hex Conversion");
    
    unsigned char data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    char hex[32];
    
    int result = ORM_BinaryToHex(data, sizeof(data), hex, sizeof(hex));
    TEST_ASSERT(result > 0, "Binary to hex conversion succeeds");
    TEST_ASSERT(strcmp(hex, "DEADBEEF") == 0, "Hex conversion is correct");
    
    unsigned char converted[8];
    result = ORM_HexToBinary("DEADBEEF", converted, sizeof(converted));
    TEST_ASSERT(result == 4, "Hex to binary conversion succeeds");
    TEST_ASSERT(memcmp(converted, data, 4) == 0, "Binary conversion is correct");
    
    TEST_END();
}

// ============================================================================
// Main Test Runner
// ============================================================================

void print_test_summary(void) {
    printf("\n");
    printf("================================================================================\n");
    printf("TEST SUMMARY\n");
    printf("================================================================================\n");
    printf("Total tests:  %d\n", tests_run);
    printf(COLOR_GREEN "Passed:       %d" COLOR_RESET "\n", tests_passed);
    
    if (tests_failed > 0) {
        printf(COLOR_RED "Failed:       %d" COLOR_RESET "\n", tests_failed);
    } else {
        printf("Failed:       %d\n", tests_failed);
    }
    
    printf("================================================================================\n");
    
    if (tests_failed == 0) {
        printf(COLOR_GREEN "✓ ALL TESTS PASSED!" COLOR_RESET "\n");
    } else {
        printf(COLOR_RED "✗ SOME TESTS FAILED!" COLOR_RESET "\n");
    }
    printf("================================================================================\n");
}

int main(void) {
    printf("================================================================================\n");
    printf("ORM FIELD TYPES SYSTEM TESTS\n");
    printf("================================================================================\n");
    
    // Run tests
    test_type_information();
    test_sql_type_names();
    test_field_sizes();
    test_int_conversion();
    test_bigint_conversion();
    test_float_conversion();
    test_double_conversion();
    test_boolean_conversion();
    test_string_conversion();
    test_string_escaping();
    test_null_conversion();
    test_string_validation();
    test_numeric_validation();
    test_field_validation();
    test_datetime_parsing();
    test_datetime_formatting();
    test_datetime_validation();
    test_datetime_conversion();
    test_current_datetime();
    test_binary_operations();
    test_binary_hex_conversion();
    
    // Print summary
    print_test_summary();
    
    return (tests_failed == 0) ? 0 : 1;
}
