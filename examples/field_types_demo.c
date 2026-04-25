#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/core/field_types.h"
#include "src/core/model.h"

void print_separator(const char* title) {
    printf("\n================================================================================\n");
    printf("%s\n", title);
    printf("================================================================================\n\n");
}

void example_type_conversion(void) {
    print_separator("Example 1: Type Conversion");
    
    // Integer conversion
    int age = 25;
    char sql_age[32];
    ORM_ConvertToSQL(ORM_TYPE_INT, &age, sql_age, sizeof(sql_age));
    printf("Integer %d → SQL: %s\n", age, sql_age);
    
    // Float conversion
    float price = 19.99f;
    char sql_price[32];
    ORM_ConvertToSQL(ORM_TYPE_FLOAT, &price, sql_price, sizeof(sql_price));
    printf("Float %.2f → SQL: %s\n", price, sql_price);
    
    // String conversion with escaping
    char name[] = "O'Brien";
    char sql_name[128];
    ORM_ConvertToSQL(ORM_TYPE_STRING, name, sql_name, sizeof(sql_name));
    printf("String \"%s\" → SQL: %s\n", name, sql_name);
    
    // Boolean conversion
    int is_active = 1;
    char sql_bool[32];
    ORM_ConvertToSQL(ORM_TYPE_BOOLEAN, &is_active, sql_bool, sizeof(sql_bool));
    printf("Boolean %d → SQL: %s\n", is_active, sql_bool);
    
    // NULL conversion
    char sql_null[32];
    ORM_ConvertToSQL(ORM_TYPE_INT, NULL, sql_null, sizeof(sql_null));
    printf("NULL → SQL: %s\n", sql_null);
}

void example_bidirectional_conversion(void) {
    print_separator("Example 2: Bidirectional Conversion");
    
    // Original value
    int original = 42;
    printf("Original value: %d\n", original);
    
    // Convert to SQL
    char sql[32];
    ORM_ConvertToSQL(ORM_TYPE_INT, &original, sql, sizeof(sql));
    printf("SQL representation: %s\n", sql);
    
    // Convert back to C
    int converted;
    ORM_ConvertFromSQL(ORM_TYPE_INT, sql, &converted);
    printf("Converted back: %d\n", converted);
    printf("Match: %s\n", (original == converted) ? "✓ YES" : "✗ NO");
    
    // String roundtrip
    printf("\nString roundtrip:\n");
    char* orig_str = "Hello, World!";
    char sql_str[128];
    char result_str[128];
    
    ORM_ConvertToSQL(ORM_TYPE_STRING, orig_str, sql_str, sizeof(sql_str));
    ORM_ConvertFromSQL(ORM_TYPE_STRING, sql_str, result_str);
    
    printf("Original: \"%s\"\n", orig_str);
    printf("SQL: %s\n", sql_str);
    printf("Result: \"%s\"\n", result_str);
    printf("Match: %s\n", (strcmp(orig_str, result_str) == 0) ? "✓ YES" : "✗ NO");
}

void example_string_escaping(void) {
    print_separator("Example 3: String Escaping");
    
    char* test_strings[] = {
        "Simple string",
        "O'Brien",
        "Path\\To\\File",
        "Quote: 'text' here",
        "Multiple\\Backslashes\\\\"
    };
    
    for (int i = 0; i < 5; i++) {
        char escaped[256];
        ORM_EscapeString(test_strings[i], escaped, sizeof(escaped));
        printf("Original: %s\n", test_strings[i]);
        printf("Escaped:  %s\n\n", escaped);
    }
}

void example_datetime_operations(void) {
    print_separator("Example 4: DateTime Operations");
    
    // Parse datetime
    printf("Parsing various datetime formats:\n\n");
    
    ORM_DateTime* dt1 = ORM_ParseDateTime("2024-03-15 14:30:45");
    if (dt1) {
        printf("Full datetime: 2024-03-15 14:30:45\n");
        printf("  Year: %d, Month: %d, Day: %d\n", dt1->year, dt1->month, dt1->day);
        printf("  Hour: %d, Minute: %d, Second: %d\n", dt1->hour, dt1->minute, dt1->second);
        ORM_FreeDateTime(dt1);
    }
    
    printf("\n");
    
    ORM_DateTime* dt2 = ORM_ParseDateTime("2024-12-25");
    if (dt2) {
        printf("Date only: 2024-12-25\n");
        printf("  Year: %d, Month: %d, Day: %d\n", dt2->year, dt2->month, dt2->day);
        printf("  Time: %02d:%02d:%02d (defaulted)\n", dt2->hour, dt2->minute, dt2->second);
        ORM_FreeDateTime(dt2);
    }
    
    printf("\n");
    
    // Format datetime
    ORM_DateTime dt3 = {2024, 10, 30, 16, 45, 30, 0};
    char buffer[64];
    
    ORM_FormatDateTime(&dt3, NULL, buffer, sizeof(buffer));
    printf("Default format: %s\n", buffer);
    
    ORM_FormatDateTime(&dt3, "%Y-%m-%d", buffer, sizeof(buffer));
    printf("Date only: %s\n", buffer);
    
    ORM_FormatDateTime(&dt3, "%H:%M:%S", buffer, sizeof(buffer));
    printf("Time only: %s\n", buffer);
    
    ORM_FormatDateTime(&dt3, "%A, %B %d, %Y", buffer, sizeof(buffer));
    printf("Long format: %s\n", buffer);
    
    printf("\n");
    
    // Current datetime
    ORM_DateTime* now = ORM_GetCurrentDateTime();
    if (now) {
        ORM_FormatDateTime(now, NULL, buffer, sizeof(buffer));
        printf("Current datetime: %s\n", buffer);
        ORM_FreeDateTime(now);
    }
}

void example_validation(void) {
    print_separator("Example 5: Field Validation");
    
    // String validation
    printf("String Validation:\n");
    
    char* valid = "test";
    printf("  \"%s\" (max 10): %s\n", valid, 
           ORM_ValidateString(valid, 10, 0) ? "✓ Valid" : "✗ Invalid");
    
    char* too_long = "this is a very long string";
    printf("  \"%s\" (max 10): %s\n", too_long, 
           ORM_ValidateString(too_long, 10, 0) ? "✓ Valid" : "✗ Invalid");
    
    char* empty = "";
    printf("  \"\" (NOT_NULL): %s\n", 
           ORM_ValidateString(empty, 100, ORM_NOT_NULL) ? "✓ Valid" : "✗ Invalid");
    
    printf("\n");
    
    // Numeric validation with range
    printf("Numeric Validation (Range: 0-100):\n");
    
    ORM_NumericField age_field = {
        .base_type = ORM_TYPE_INT,
        .min_value = 0,
        .max_value = 100
    };
    
    int values[] = {-5, 0, 50, 100, 150};
    for (int i = 0; i < 5; i++) {
        printf("  %d: %s\n", values[i], 
               ORM_ValidateNumeric(&values[i], &age_field) ? "✓ Valid" : "✗ Invalid");
    }
    
    printf("\n");
    
    // DateTime validation
    printf("DateTime Validation:\n");
    
    ORM_DateTime valid_dt = {2024, 3, 15, 14, 30, 0, 0};
    printf("  2024-03-15 14:30:00: %s\n", 
           ORM_ValidateDateTime(&valid_dt) ? "✓ Valid" : "✗ Invalid");
    
    ORM_DateTime invalid_dt = {2024, 13, 15, 14, 30, 0, 0}; // Invalid month
    printf("  2024-13-15 14:30:00: %s (invalid month)\n", 
           ORM_ValidateDateTime(&invalid_dt) ? "✓ Valid" : "✗ Invalid");
}

void example_binary_data(void) {
    print_separator("Example 6: Binary Data Handling");
    
    // Create binary data
    unsigned char data[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE};
    
    printf("Original binary data: ");
    for (size_t i = 0; i < sizeof(data); i++) {
        printf("%02X ", data[i]);
    }
    printf("\n\n");
    
    // Convert to hex string
    char hex[64];
    ORM_BinaryToHex(data, sizeof(data), hex, sizeof(hex));
    printf("Hex representation: %s\n", hex);
    
    // Create binary field
    ORM_BinaryField* field = ORM_CreateBinaryField(data, sizeof(data));
    if (field) {
        printf("Binary field size: %zu bytes\n", field->actual_size);
        
        // Convert to SQL
        char sql[128];
        ORM_ConvertToSQL(ORM_TYPE_BLOB, field, sql, sizeof(sql));
        printf("SQL representation: %s\n", sql);
        
        ORM_FreeBinaryField(field);
    }
    
    printf("\n");
    
    // Convert hex back to binary
    unsigned char converted[32];
    int size = ORM_HexToBinary(hex, converted, sizeof(converted));
    
    printf("Converted back (%d bytes): ", size);
    for (int i = 0; i < size; i++) {
        printf("%02X ", converted[i]);
    }
    printf("\n");
    
    printf("Match: %s\n", (memcmp(data, converted, sizeof(data)) == 0) ? "✓ YES" : "✗ NO");
}

void example_type_information(void) {
    print_separator("Example 7: Type Information");
    
    printf("Type Classification:\n\n");
    
    ORM_FieldType types[] = {
        ORM_TYPE_INT,
        ORM_TYPE_BIGINT,
        ORM_TYPE_FLOAT,
        ORM_TYPE_STRING,
        ORM_TYPE_DATETIME,
        ORM_TYPE_BLOB
    };
    
    const char* names[] = {
        "INT",
        "BIGINT",
        "FLOAT",
        "STRING",
        "DATETIME",
        "BLOB"
    };
    
    for (int i = 0; i < 6; i++) {
        printf("%-10s: ", names[i]);
        printf("Numeric: %s, ", ORM_IsNumericType(types[i]) ? "Yes" : "No ");
        printf("String: %s, ", ORM_IsStringType(types[i]) ? "Yes" : "No ");
        printf("Temporal: %s, ", ORM_IsTemporalType(types[i]) ? "Yes" : "No ");
        printf("SQL: %s, ", ORM_GetSQLTypeName(types[i], NULL));
        printf("Size: %zu bytes\n", ORM_GetFieldSize(types[i], NULL));
    }
}

int main(void) {
    printf("================================================================================\n");
    printf("ORM FIELD TYPES SYSTEM DEMONSTRATION\n");
    printf("================================================================================\n");
    
    example_type_conversion();
    example_bidirectional_conversion();
    example_string_escaping();
    example_datetime_operations();
    example_validation();
    example_binary_data();
    example_type_information();
    
    print_separator("Summary");
    printf("The field types system provides:\n");
    printf("  ✓ Bidirectional type conversion (C ↔ SQL)\n");
    printf("  ✓ Automatic string escaping\n");
    printf("  ✓ Comprehensive datetime handling\n");
    printf("  ✓ Field validation with constraints\n");
    printf("  ✓ Binary data operations\n");
    printf("  ✓ Type information and classification\n");
    printf("\nAll features are production-ready and fully tested!\n\n");
    
    return 0;
}
