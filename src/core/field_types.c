#include "field_types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

// ============================================================================
// Type Information Functions
// ============================================================================

bool ORM_IsNumericType(ORM_FieldType type) {
    return (type == ORM_TYPE_INT || 
            type == ORM_TYPE_BIGINT || 
            type == ORM_TYPE_FLOAT || 
            type == ORM_TYPE_DOUBLE);
}

bool ORM_IsStringType(ORM_FieldType type) {
    return (type == ORM_TYPE_STRING || type == ORM_TYPE_TEXT);
}

bool ORM_IsTemporalType(ORM_FieldType type) {
    return (type == ORM_TYPE_DATE || 
            type == ORM_TYPE_TIME || 
            type == ORM_TYPE_DATETIME);
}

const char* ORM_GetSQLTypeName(ORM_FieldType type, const void* metadata) {
    (void)metadata; // May be used in future for size specifications
    
    switch (type) {
        case ORM_TYPE_INT:       return "INTEGER";
        case ORM_TYPE_BIGINT:    return "BIGINT";
        case ORM_TYPE_FLOAT:     return "REAL";
        case ORM_TYPE_DOUBLE:    return "DOUBLE";
        case ORM_TYPE_STRING:    return "VARCHAR";
        case ORM_TYPE_TEXT:      return "TEXT";
        case ORM_TYPE_BOOLEAN:   return "BOOLEAN";
        case ORM_TYPE_DATE:      return "DATE";
        case ORM_TYPE_TIME:      return "TIME";
        case ORM_TYPE_DATETIME:  return "DATETIME";
        case ORM_TYPE_BLOB:      return "BLOB";
        case ORM_TYPE_FOREIGN_KEY: return "INTEGER";
        default:                 return "UNKNOWN";
    }
}

size_t ORM_GetFieldSize(ORM_FieldType type, const void* metadata) {
    switch (type) {
        case ORM_TYPE_INT:
        case ORM_TYPE_BOOLEAN:
        case ORM_TYPE_FOREIGN_KEY:
            return sizeof(int);
            
        case ORM_TYPE_BIGINT:
        case ORM_TYPE_DATE:
        case ORM_TYPE_TIME:
        case ORM_TYPE_DATETIME:
            return sizeof(int64_t);
            
        case ORM_TYPE_FLOAT:
            return sizeof(float);
            
        case ORM_TYPE_DOUBLE:
            return sizeof(double);
            
        case ORM_TYPE_STRING:
        case ORM_TYPE_TEXT:
        case ORM_TYPE_BLOB:
            if (metadata) {
                const ORM_StringField* sf = (const ORM_StringField*)metadata;
                return sf->max_length;
            }
            return 256; // Default size
            
        default:
            return 0;
    }
}

// ============================================================================
// Type Conversion Functions
// ============================================================================

int ORM_ConvertToSQL(ORM_FieldType type, const void* value, char* buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return -1;
    }
    
    // Handle NULL values
    if (value == NULL) {
        return snprintf(buffer, size, "NULL");
    }
    
    switch (type) {
        case ORM_TYPE_INT:
        case ORM_TYPE_FOREIGN_KEY:
            return snprintf(buffer, size, "%d", *(const int*)value);
            
        case ORM_TYPE_BIGINT:
        case ORM_TYPE_DATE:
        case ORM_TYPE_TIME:
        case ORM_TYPE_DATETIME:
            return snprintf(buffer, size, "%lld", (long long)*(const int64_t*)value);
            
        case ORM_TYPE_FLOAT:
            return snprintf(buffer, size, "%.6f", *(const float*)value);
            
        case ORM_TYPE_DOUBLE:
            return snprintf(buffer, size, "%.15f", *(const double*)value);
            
        case ORM_TYPE_BOOLEAN:
            return snprintf(buffer, size, "%d", *(const int*)value ? 1 : 0);
            
        case ORM_TYPE_STRING:
        case ORM_TYPE_TEXT: {
            const char* str = (const char*)value;
            if (str[0] == '\0') {
                return snprintf(buffer, size, "NULL");
            }
            
            // Allocate buffer for escaped string
            size_t str_len = strlen(str);
            char* escaped = (char*)malloc(str_len * 2 + 3); // Worst case: all chars escaped + quotes + null
            if (!escaped) {
                return -1;
            }
            
            // Add opening quote
            escaped[0] = '\'';
            size_t pos = 1;
            
            // Escape the string
            for (size_t i = 0; i < str_len; i++) {
                if (str[i] == '\'') {
                    escaped[pos++] = '\'';
                    escaped[pos++] = '\'';
                } else if (str[i] == '\\') {
                    escaped[pos++] = '\\';
                    escaped[pos++] = '\\';
                } else {
                    escaped[pos++] = str[i];
                }
            }
            
            // Add closing quote and null terminator
            escaped[pos++] = '\'';
            escaped[pos] = '\0';
            
            int result = snprintf(buffer, size, "%s", escaped);
            free(escaped);
            return result;
        }
            
        case ORM_TYPE_BLOB: {
            // Convert binary to hex string with X'' notation
            const ORM_BinaryField* bf = (const ORM_BinaryField*)value;
            if (bf == NULL || bf->data == NULL || bf->actual_size == 0) {
                return snprintf(buffer, size, "NULL");
            }
            
            char* hex = (char*)malloc(bf->actual_size * 2 + 4); // X'' + hex + null
            if (!hex) {
                return -1;
            }
            
            hex[0] = 'X';
            hex[1] = '\'';
            
            for (size_t i = 0; i < bf->actual_size; i++) {
                sprintf(&hex[2 + i * 2], "%02X", ((unsigned char*)bf->data)[i]);
            }
            
            hex[2 + bf->actual_size * 2] = '\'';
            hex[3 + bf->actual_size * 2] = '\0';
            
            int result = snprintf(buffer, size, "%s", hex);
            free(hex);
            return result;
        }
            
        default:
            return snprintf(buffer, size, "NULL");
    }
}

int ORM_ConvertFromSQL(ORM_FieldType type, const char* sql_value, void* out) {
    if (sql_value == NULL || out == NULL) {
        return ORM_ERROR_INVALID_PARAM;
    }
    
    // Handle NULL values
    if (strcmp(sql_value, "NULL") == 0 || sql_value[0] == '\0') {
        memset(out, 0, ORM_GetFieldSize(type, NULL));
        return ORM_SUCCESS;
    }
    
    switch (type) {
        case ORM_TYPE_INT:
        case ORM_TYPE_FOREIGN_KEY:
            *(int*)out = atoi(sql_value);
            return ORM_SUCCESS;
            
        case ORM_TYPE_BIGINT:
        case ORM_TYPE_DATE:
        case ORM_TYPE_TIME:
        case ORM_TYPE_DATETIME:
            *(int64_t*)out = strtoll(sql_value, NULL, 10);
            return ORM_SUCCESS;
            
        case ORM_TYPE_FLOAT:
            *(float*)out = strtof(sql_value, NULL);
            return ORM_SUCCESS;
            
        case ORM_TYPE_DOUBLE:
            *(double*)out = strtod(sql_value, NULL);
            return ORM_SUCCESS;
            
        case ORM_TYPE_BOOLEAN:
            *(int*)out = (atoi(sql_value) != 0) ? 1 : 0;
            return ORM_SUCCESS;
            
        case ORM_TYPE_STRING:
        case ORM_TYPE_TEXT: {
            // Remove quotes if present
            const char* start = sql_value;
            size_t len = strlen(sql_value);
            
            if (len >= 2 && sql_value[0] == '\'' && sql_value[len-1] == '\'') {
                start = sql_value + 1;
                len -= 2;
            }
            
            // Unescape the string
            char* dest = (char*)out;
            size_t pos = 0;
            
            for (size_t i = 0; i < len; i++) {
                if (start[i] == '\'' && i + 1 < len && start[i+1] == '\'') {
                    dest[pos++] = '\'';
                    i++; // Skip next quote
                } else if (start[i] == '\\' && i + 1 < len && start[i+1] == '\\') {
                    dest[pos++] = '\\';
                    i++; // Skip next backslash
                } else {
                    dest[pos++] = start[i];
                }
            }
            dest[pos] = '\0';
            
            return ORM_SUCCESS;
        }
            
        default:
            return ORM_ERROR;
    }
}

// ============================================================================
// Validation Functions
// ============================================================================

bool ORM_IsNullValue(ORM_FieldType type, const void* value) {
    if (value == NULL) {
        return true;
    }
    
    switch (type) {
        case ORM_TYPE_STRING:
        case ORM_TYPE_TEXT:
            return ((const char*)value)[0] == '\0';
            
        case ORM_TYPE_INT:
        case ORM_TYPE_FOREIGN_KEY:
        case ORM_TYPE_BOOLEAN:
            // For integers, we can't reliably detect NULL without additional metadata
            // This is a limitation - consider 0 as non-NULL
            return false;
            
        default:
            return false;
    }
}

bool ORM_ValidateField(ORM_FieldType type, const void* value, int flags) {
    // Check NOT_NULL constraint
    if (flags & ORM_NOT_NULL) {
        if (ORM_IsNullValue(type, value)) {
            return false;
        }
    }
    
    // Type-specific validation
    switch (type) {
        case ORM_TYPE_STRING:
        case ORM_TYPE_TEXT:
            if (value != NULL) {
                return ORM_ValidateString((const char*)value, SIZE_MAX, flags);
            }
            return true;
            
        case ORM_TYPE_INT:
        case ORM_TYPE_BIGINT:
        case ORM_TYPE_FLOAT:
        case ORM_TYPE_DOUBLE:
            // Basic numeric validation (could be enhanced with range checks)
            return value != NULL || !(flags & ORM_NOT_NULL);
            
        default:
            return true;
    }
}

bool ORM_ValidateString(const char* str, size_t max_len, int flags) {
    if (str == NULL) {
        return !(flags & ORM_NOT_NULL);
    }
    
    size_t len = strlen(str);
    
    // Check length constraint
    if (len > max_len) {
        return false;
    }
    
    // Empty string check with NOT_NULL
    if (len == 0 && (flags & ORM_NOT_NULL)) {
        return false;
    }
    
    return true;
}

bool ORM_ValidateNumeric(const void* value, ORM_NumericField* field) {
    if (value == NULL || field == NULL) {
        return false;
    }
    
    int64_t num_value;
    
    switch (field->base_type) {
        case ORM_TYPE_INT:
        case ORM_TYPE_FOREIGN_KEY:
            num_value = *(const int*)value;
            break;
            
        case ORM_TYPE_BIGINT:
            num_value = *(const int64_t*)value;
            break;
            
        case ORM_TYPE_FLOAT:
            num_value = (int64_t)*(const float*)value;
            break;
            
        case ORM_TYPE_DOUBLE:
            num_value = (int64_t)*(const double*)value;
            break;
            
        default:
            return false;
    }
    
    // Check range
    if (num_value < field->min_value || num_value > field->max_value) {
        return false;
    }
    
    return true;
}

bool ORM_ValidateDateTime(const ORM_DateTime* dt) {
    if (dt == NULL) {
        return false;
    }
    
    // Validate year (reasonable range)
    if (dt->year < 1900 || dt->year > 2100) {
        return false;
    }
    
    // Validate month
    if (dt->month < 1 || dt->month > 12) {
        return false;
    }
    
    // Validate day (simple check, doesn't account for month-specific days)
    if (dt->day < 1 || dt->day > 31) {
        return false;
    }
    
    // Validate hour
    if (dt->hour < 0 || dt->hour > 23) {
        return false;
    }
    
    // Validate minute
    if (dt->minute < 0 || dt->minute > 59) {
        return false;
    }
    
    // Validate second
    if (dt->second < 0 || dt->second > 59) {
        return false;
    }
    
    // Validate microsecond
    if (dt->microsecond < 0 || dt->microsecond > 999999) {
        return false;
    }
    
    return true;
}

// ============================================================================
// String Operations
// ============================================================================

int ORM_EscapeString(const char* input, char* output, size_t out_size) {
    if (input == NULL || output == NULL || out_size == 0) {
        return -1;
    }
    
    size_t in_len = strlen(input);
    size_t out_pos = 0;
    
    for (size_t i = 0; i < in_len && out_pos < out_size - 1; i++) {
        if (input[i] == '\'') {
            if (out_pos + 2 >= out_size) {
                return -1; // Buffer too small
            }
            output[out_pos++] = '\'';
            output[out_pos++] = '\'';
        } else if (input[i] == '\\') {
            if (out_pos + 2 >= out_size) {
                return -1; // Buffer too small
            }
            output[out_pos++] = '\\';
            output[out_pos++] = '\\';
        } else {
            output[out_pos++] = input[i];
        }
    }
    
    output[out_pos] = '\0';
    return (int)out_pos;
}

int ORM_UnescapeString(const char* input, char* output, size_t out_size) {
    if (input == NULL || output == NULL || out_size == 0) {
        return -1;
    }
    
    size_t in_len = strlen(input);
    size_t out_pos = 0;
    
    for (size_t i = 0; i < in_len && out_pos < out_size - 1; i++) {
        if (input[i] == '\'' && i + 1 < in_len && input[i+1] == '\'') {
            output[out_pos++] = '\'';
            i++; // Skip next quote
        } else if (input[i] == '\\' && i + 1 < in_len && input[i+1] == '\\') {
            output[out_pos++] = '\\';
            i++; // Skip next backslash
        } else {
            output[out_pos++] = input[i];
        }
    }
    
    output[out_pos] = '\0';
    return (int)out_pos;
}

// ============================================================================
// Default Value Handling
// ============================================================================

int ORM_SetFieldDefaultValue(ORM_FieldType type, void* field, const void* default_val) {
    if (field == NULL || default_val == NULL) {
        return ORM_ERROR_INVALID_PARAM;
    }
    
    switch (type) {
        case ORM_TYPE_INT:
        case ORM_TYPE_FOREIGN_KEY:
        case ORM_TYPE_BOOLEAN:
            *(int*)field = *(const int*)default_val;
            return ORM_SUCCESS;
            
        case ORM_TYPE_BIGINT:
        case ORM_TYPE_DATE:
        case ORM_TYPE_TIME:
        case ORM_TYPE_DATETIME:
            *(int64_t*)field = *(const int64_t*)default_val;
            return ORM_SUCCESS;
            
        case ORM_TYPE_FLOAT:
            *(float*)field = *(const float*)default_val;
            return ORM_SUCCESS;
            
        case ORM_TYPE_DOUBLE:
            *(double*)field = *(const double*)default_val;
            return ORM_SUCCESS;
            
        case ORM_TYPE_STRING:
        case ORM_TYPE_TEXT:
            strncpy((char*)field, (const char*)default_val, 255);
            ((char*)field)[255] = '\0';
            return ORM_SUCCESS;
            
        default:
            return ORM_ERROR;
    }
}

// ============================================================================
// Binary Data Operations
// ============================================================================

ORM_BinaryField* ORM_CreateBinaryField(const void* data, size_t size) {
    if (data == NULL || size == 0) {
        return NULL;
    }
    
    ORM_BinaryField* field = (ORM_BinaryField*)malloc(sizeof(ORM_BinaryField));
    if (field == NULL) {
        return NULL;
    }
    
    field->base_type = ORM_TYPE_BLOB;
    field->max_size = size;
    field->actual_size = size;
    field->data = malloc(size);
    
    if (field->data == NULL) {
        free(field);
        return NULL;
    }
    
    memcpy(field->data, data, size);
    return field;
}

void ORM_FreeBinaryField(ORM_BinaryField* field) {
    if (field != NULL) {
        if (field->data != NULL) {
            free(field->data);
        }
        free(field);
    }
}

int ORM_BinaryToHex(const void* data, size_t size, char* buffer, size_t buf_size) {
    if (data == NULL || buffer == NULL || buf_size < size * 2 + 1) {
        return -1;
    }
    
    const unsigned char* bytes = (const unsigned char*)data;
    for (size_t i = 0; i < size; i++) {
        sprintf(&buffer[i * 2], "%02X", bytes[i]);
    }
    
    buffer[size * 2] = '\0';
    return (int)(size * 2);
}

int ORM_HexToBinary(const char* hex, void* buffer, size_t buf_size) {
    if (hex == NULL || buffer == NULL) {
        return -1;
    }
    
    size_t hex_len = strlen(hex);
    if (hex_len % 2 != 0 || hex_len / 2 > buf_size) {
        return -1;
    }
    
    unsigned char* bytes = (unsigned char*)buffer;
    for (size_t i = 0; i < hex_len; i += 2) {
        unsigned int byte;
        if (sscanf(&hex[i], "%2x", &byte) != 1) {
            return -1;
        }
        bytes[i / 2] = (unsigned char)byte;
    }
    
    return (int)(hex_len / 2);
}
