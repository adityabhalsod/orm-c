#ifndef ORM_FIELD_TYPES_H
#define ORM_FIELD_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "model.h"

// ============================================================================
// Field Value Union
// ============================================================================

/**
 * Union for storing field values of different types
 * Allows type-safe access to values based on field type
 */
typedef union {
    int int_val;
    int64_t bigint_val;
    float float_val;
    double double_val;
    char* string_val;
    bool bool_val;
    time_t datetime_val;
    void* binary_val;
} ORM_FieldValue;

// ============================================================================
// Type-Specific Metadata Structures
// ============================================================================

/**
 * String/Text field metadata
 */
typedef struct {
    ORM_FieldType base_type;
    size_t max_length;
    const char* charset;      // e.g., "utf8", "ascii"
    const char* collation;    // e.g., "utf8_general_ci"
} ORM_StringField;

/**
 * Numeric field metadata with precision and range
 */
typedef struct {
    ORM_FieldType base_type;
    int precision;            // Total number of digits
    int scale;                // Digits after decimal point
    int64_t min_value;        // Minimum allowed value
    int64_t max_value;        // Maximum allowed value
} ORM_NumericField;

/**
 * Date/Time structure for temporal values
 */
typedef struct {
    int year;
    int month;              // 1-12
    int day;                // 1-31
    int hour;               // 0-23
    int minute;             // 0-59
    int second;             // 0-59
    int microsecond;        // 0-999999
} ORM_DateTime;

/**
 * Binary/BLOB field metadata
 */
typedef struct {
    ORM_FieldType base_type;
    size_t max_size;        // Maximum binary data size
    void* data;             // Pointer to binary data
    size_t actual_size;     // Actual data size
} ORM_BinaryField;

// ============================================================================
// Type Conversion Functions
// ============================================================================

/**
 * Convert a C value to SQL string representation
 * @param type Field type
 * @param value Pointer to the value
 * @param buffer Output buffer for SQL string
 * @param size Size of output buffer
 * @return Number of characters written, or -1 on error
 */
int ORM_ConvertToSQL(ORM_FieldType type, const void* value, char* buffer, size_t size);

/**
 * Convert SQL string representation to C value
 * @param type Field type
 * @param sql_value SQL string representation
 * @param out Output buffer for converted value
 * @return ORM_SUCCESS on success, error code otherwise
 */
int ORM_ConvertFromSQL(ORM_FieldType type, const char* sql_value, void* out);

/**
 * Get the size needed to store a value of given type
 * @param type Field type
 * @param metadata Optional type-specific metadata
 * @return Size in bytes
 */
size_t ORM_GetFieldSize(ORM_FieldType type, const void* metadata);

// ============================================================================
// Validation Functions
// ============================================================================

/**
 * Validate a field value against type and constraints
 * @param type Field type
 * @param value Pointer to the value
 * @param flags Constraint flags
 * @return true if valid, false otherwise
 */
bool ORM_ValidateField(ORM_FieldType type, const void* value, int flags);

/**
 * Validate a string field
 * @param str String to validate
 * @param max_len Maximum allowed length
 * @param flags Constraint flags
 * @return true if valid, false otherwise
 */
bool ORM_ValidateString(const char* str, size_t max_len, int flags);

/**
 * Validate a numeric field
 * @param value Pointer to numeric value
 * @param field Numeric field metadata
 * @return true if valid, false otherwise
 */
bool ORM_ValidateNumeric(const void* value, ORM_NumericField* field);

/**
 * Validate a datetime value
 * @param dt DateTime structure
 * @return true if valid, false otherwise
 */
bool ORM_ValidateDateTime(const ORM_DateTime* dt);

// ============================================================================
// String Operations
// ============================================================================

/**
 * Escape special characters in string for SQL
 * @param input Input string
 * @param output Output buffer
 * @param out_size Size of output buffer
 * @return Number of characters written, or -1 on error
 */
int ORM_EscapeString(const char* input, char* output, size_t out_size);

/**
 * Unescape SQL string
 * @param input Escaped SQL string
 * @param output Output buffer
 * @param out_size Size of output buffer
 * @return Number of characters written, or -1 on error
 */
int ORM_UnescapeString(const char* input, char* output, size_t out_size);

// ============================================================================
// DateTime Operations
// ============================================================================

/**
 * Parse a datetime string into ORM_DateTime structure
 * Supported formats: "YYYY-MM-DD HH:MM:SS", "YYYY-MM-DD", etc.
 * @param str DateTime string
 * @return Pointer to allocated ORM_DateTime, or NULL on error
 */
ORM_DateTime* ORM_ParseDateTime(const char* str);

/**
 * Format a datetime structure as string
 * @param dt DateTime structure
 * @param format Format string (strftime compatible)
 * @param buffer Output buffer
 * @param size Size of output buffer
 * @return Number of characters written, or -1 on error
 */
int ORM_FormatDateTime(const ORM_DateTime* dt, const char* format, char* buffer, size_t size);

/**
 * Convert ORM_DateTime to Unix timestamp
 * @param dt DateTime structure
 * @return Unix timestamp (seconds since epoch)
 */
time_t ORM_DateTimeToTimestamp(const ORM_DateTime* dt);

/**
 * Convert Unix timestamp to ORM_DateTime
 * @param timestamp Unix timestamp
 * @return Pointer to allocated ORM_DateTime, or NULL on error
 */
ORM_DateTime* ORM_TimestampToDateTime(time_t timestamp);

/**
 * Free a dynamically allocated ORM_DateTime structure
 * @param dt DateTime structure to free
 */
void ORM_FreeDateTime(ORM_DateTime* dt);

/**
 * Get current datetime
 * @return Pointer to allocated ORM_DateTime with current time
 */
ORM_DateTime* ORM_GetCurrentDateTime(void);

// ============================================================================
// Default Value Handling
// ============================================================================

/**
 * Set a default value for a field based on type
 * @param type Field type
 * @param field Pointer to field storage
 * @param default_val Pointer to default value
 * @return ORM_SUCCESS on success, error code otherwise
 */
int ORM_SetFieldDefaultValue(ORM_FieldType type, void* field, const void* default_val);

/**
 * Check if a value is NULL/empty
 * @param type Field type
 * @param value Pointer to value
 * @return true if NULL/empty, false otherwise
 */
bool ORM_IsNullValue(ORM_FieldType type, const void* value);

// ============================================================================
// Type Information Functions
// ============================================================================

/**
 * Check if a type is numeric
 * @param type Field type
 * @return true if numeric, false otherwise
 */
bool ORM_IsNumericType(ORM_FieldType type);

/**
 * Check if a type is string-based
 * @param type Field type
 * @return true if string-based, false otherwise
 */
bool ORM_IsStringType(ORM_FieldType type);

/**
 * Check if a type is temporal (date/time)
 * @param type Field type
 * @return true if temporal, false otherwise
 */
bool ORM_IsTemporalType(ORM_FieldType type);

/**
 * Get SQL type name for a field type
 * @param type Field type
 * @param metadata Optional type-specific metadata
 * @return SQL type name string
 */
const char* ORM_GetSQLTypeName(ORM_FieldType type, const void* metadata);

// ============================================================================
// Binary Data Operations
// ============================================================================

/**
 * Create a binary field value
 * @param data Binary data
 * @param size Size of data
 * @return Pointer to allocated ORM_BinaryField, or NULL on error
 */
ORM_BinaryField* ORM_CreateBinaryField(const void* data, size_t size);

/**
 * Free a binary field
 * @param field Binary field to free
 */
void ORM_FreeBinaryField(ORM_BinaryField* field);

/**
 * Convert binary data to hexadecimal string
 * @param data Binary data
 * @param size Size of data
 * @param buffer Output buffer
 * @param buf_size Size of output buffer
 * @return Number of characters written, or -1 on error
 */
int ORM_BinaryToHex(const void* data, size_t size, char* buffer, size_t buf_size);

/**
 * Convert hexadecimal string to binary data
 * @param hex Hexadecimal string
 * @param buffer Output buffer for binary data
 * @param buf_size Size of output buffer
 * @return Number of bytes written, or -1 on error
 */
int ORM_HexToBinary(const char* hex, void* buffer, size_t buf_size);

#endif // ORM_FIELD_TYPES_H
