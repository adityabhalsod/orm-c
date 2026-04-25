# ORM-C Model System

A lightweight, macro-based ORM (Object-Relational Mapping) system for C that provides Django-style model definitions with automatic metadata generation.

## Features

### Model System (Task 01)
- ✅ Simple macro-based model definitions
- ✅ Automatic metadata generation for fields
- ✅ Type-safe field definitions (12 types: INT, BIGINT, FLOAT, DOUBLE, STRING, TEXT, BOOLEAN, DATE, TIME, DATETIME, BLOB, FOREIGN_KEY)
- ✅ Field constraints (PRIMARY_KEY, AUTO_INCREMENT, NOT_NULL, UNIQUE, INDEX)
- ✅ Global model registry
- ✅ Constructor/Destructor generation
- ✅ Field introspection and metadata access
- ✅ Default values support
- ✅ No memory leaks - clean lifecycle management
- ✅ Supports 50+ models simultaneously

### Field Types System (Task 02)
- ✅ Bidirectional type conversion (C ↔ SQL)
- ✅ Automatic SQL string escaping (prevents SQL injection)
- ✅ Comprehensive datetime handling (parsing, formatting, timestamps)
- ✅ Field validation (NOT_NULL, string length, numeric ranges, format validation)
- ✅ Binary data operations (hex encoding/decoding)
- ✅ Type classification and information
- ✅ Thread-safe, stateless design

## Quick Start

### 1. Define Your Model

```c
#include "src/core/model.h"

// Define the C struct
typedef struct {
    int id;
    char username[50];
    char email[100];
    int age;
} User;

// Define model metadata
#undef ORM_CURRENT_MODEL
#define ORM_CURRENT_MODEL User

ORM_MODEL_DEFINE(User, "users",
    ORM_FIELD_INT(id, ORM_PRIMARY_KEY | ORM_AUTO_INCREMENT),
    ORM_FIELD_STRING(username, 50, ORM_NOT_NULL | ORM_UNIQUE),
    ORM_FIELD_STRING(email, 100, ORM_NOT_NULL),
    ORM_FIELD_INT(age, 0)
);
```

### 2. Initialize and Use

```c
int main() {
    // Initialize the ORM
    ORM_Init();
    
    // Register your model
    ORM_RegisterModel(&User_meta);
    
    // Create a new user instance
    User* user = (User*)User_new();
    strcpy(user->username, "john_doe");
    strcpy(user->email, "john@example.com");
    user->age = 25;
    
    // Access model metadata
    ORM_ModelMeta* meta = ORM_GetModel("User");
    printf("Table: %s, Fields: %d\n", meta->table_name, meta->field_count);
    
    // Clean up
    User_free(user);
    ORM_Shutdown();
    
    return 0;
}
```

## Field Types

| Macro | C Type | SQL Type | Description |
|-------|--------|----------|-------------|
| `ORM_FIELD_INT` | `int` | INTEGER | 32-bit integer |
| `ORM_FIELD_BIGINT` | `int64_t` | BIGINT | 64-bit integer |
| `ORM_FIELD_FLOAT` | `float` | REAL | Single precision float |
| `ORM_FIELD_DOUBLE` | `double` | REAL | Double precision float |
| `ORM_FIELD_STRING` | `char[]` | VARCHAR | Variable-length string |
| `ORM_FIELD_TEXT` | `char[]` | TEXT | Large text field |
| `ORM_FIELD_BOOLEAN` | `int` | BOOLEAN | Boolean (0/1) |
| `ORM_FIELD_DATE` | `int64_t` | DATE | Date (timestamp) |
| `ORM_FIELD_TIME` | `int64_t` | TIME | Time (timestamp) |
| `ORM_FIELD_DATETIME` | `int64_t` | DATETIME | Date and time (timestamp) |
| `ORM_FIELD_BLOB` | `char[]` | BLOB | Binary data |
| `ORM_FIELD_FOREIGN_KEY` | `int` | INTEGER | Foreign key reference |

## Field Flags

Flags can be combined using the bitwise OR operator (`|`):

| Flag | Value | Description |
|------|-------|-------------|
| `ORM_PRIMARY_KEY` | 0x01 | Primary key field |
| `ORM_AUTO_INCREMENT` | 0x02 | Auto-incrementing field |
| `ORM_NOT_NULL` | 0x04 | Field cannot be NULL |
| `ORM_UNIQUE` | 0x08 | Field must be unique |
| `ORM_INDEX` | 0x10 | Create index on field |
| `ORM_HAS_DEFAULT` | 0x20 | Field has default value |

## API Reference

### Core Functions

```c
// Initialize the ORM system
int ORM_Init(void);

// Shutdown and cleanup
void ORM_Shutdown(void);

// Register a model
int ORM_RegisterModel(ORM_ModelMeta* meta);

// Get a model by name
ORM_ModelMeta* ORM_GetModel(const char* name);

// Unregister a model
int ORM_UnregisterModel(const char* name);

// Get total model count
int ORM_GetModelCount(void);
```

### Model Introspection

```c
// Get field by name
ORM_FieldMeta* ORM_GetField(ORM_ModelMeta* meta, const char* field_name);

// Get primary key field
ORM_FieldMeta* ORM_GetPrimaryKey(ORM_ModelMeta* meta);

// Check if field has a flag
int ORM_FieldHasFlag(ORM_FieldMeta* field, int flag);

// Get pointer to field value in object
void* ORM_GetFieldValue(void* obj, ORM_FieldMeta* field);

// Set default value for a field
void ORM_SetDefaultValue(ORM_FieldMeta* field, ORM_FieldValue value);

// Apply default values to an object
void ORM_ApplyDefaults(ORM_ModelMeta* meta, void* obj);
```

### Field Type Operations

```c
// Type conversion
const char* ORM_ConvertToSQL(ORM_FieldType type, const void* value, size_t size);
int ORM_ConvertFromSQL(ORM_FieldType type, const char* sql_value, void* out_value, size_t out_size);

// String operations
char* ORM_EscapeString(const char* input);
int ORM_UnescapeString(const char* input, char* output, size_t output_size);

// Validation
int ORM_ValidateField(ORM_FieldMeta* field, const void* value);

// Type information
const char* ORM_GetSQLTypeName(ORM_FieldType type);
size_t ORM_GetFieldSize(ORM_FieldType type, size_t custom_size);
int ORM_IsNumericType(ORM_FieldType type);
int ORM_IsStringType(ORM_FieldType type);
int ORM_IsTemporalType(ORM_FieldType type);

// DateTime operations
int ORM_ParseDateTime(const char* str, struct tm* tm_out);
int ORM_FormatDateTime(const struct tm* tm, char* output, size_t output_size, const char* format);
int64_t ORM_DateTimeToTimestamp(const struct tm* tm);
int ORM_TimestampToDateTime(int64_t timestamp, struct tm* tm_out);

// Binary data
int ORM_BinaryToHex(const void* data, size_t data_size, char* hex_output, size_t hex_size);
int ORM_HexToBinary(const char* hex, void* data_output, size_t data_size);
```

### Generated Functions

For each model `ModelType`, these functions are automatically generated:

```c
// Constructor - allocates and zeros memory
void* ModelType_new(void);

// Destructor - frees memory
void ModelType_free(void* obj);
```

## Return Codes

```c
ORM_SUCCESS           // 0  - Operation successful
ORM_ERROR             // -1 - General error
ORM_ERROR_MEMORY      // -2 - Memory allocation failed
ORM_ERROR_NOT_FOUND   // -3 - Model/field not found
ORM_ERROR_DUPLICATE   // -4 - Duplicate model name
ORM_ERROR_INVALID_PARAM // -5 - Invalid parameter
```

## Examples

### Multiple Field Types

```c
typedef struct {
    int id;
    char title[200];
    int published;
    int64_t views;
    float rating;
} Article;

#undef ORM_CURRENT_MODEL
#define ORM_CURRENT_MODEL Article

ORM_MODEL_DEFINE(Article, "articles",
    ORM_FIELD_INT(id, ORM_PRIMARY_KEY | ORM_AUTO_INCREMENT),
    ORM_FIELD_STRING(title, 200, ORM_NOT_NULL),
    ORM_FIELD_BOOLEAN(published, 0),
    ORM_FIELD_BIGINT(views, 0),
    ORM_FIELD_FLOAT(rating, 0)
);
```

### Field Introspection

```c
ORM_ModelMeta* meta = ORM_GetModel("User");

// Get primary key
ORM_FieldMeta* pk = ORM_GetPrimaryKey(meta);
printf("Primary key: %s\n", pk->field_name);

// Iterate through all fields
for (int i = 0; i < meta->field_count; i++) {
    ORM_FieldMeta* field = &meta->fields[i];
    printf("Field: %s, Type: %d, Flags: 0x%x\n",
           field->field_name, field->type, field->flags);
}
```

### Dynamic Field Access

```c
User user;
user.id = 42;
user.age = 30;

ORM_ModelMeta* meta = ORM_GetModel("User");
ORM_FieldMeta* age_field = ORM_GetField(meta, "age");

// Get pointer to age field
int* age_ptr = (int*)ORM_GetFieldValue(&user, age_field);
printf("Age: %d\n", *age_ptr);  // Output: Age: 30
```

### Field Type Conversion

```c
// Convert C values to SQL
int value = 42;
const char* sql = ORM_ConvertToSQL(ORM_FIELD_INT, &value, sizeof(value));
printf("SQL: %s\n", sql);  // Output: SQL: 42

// Convert SQL back to C
int result;
ORM_ConvertFromSQL(ORM_FIELD_INT, "42", &result, sizeof(result));
printf("Result: %d\n", result);  // Output: Result: 42

// String escaping (prevents SQL injection)
char* escaped = ORM_EscapeString("O'Brien");
printf("Escaped: %s\n", escaped);  // Output: Escaped: O''Brien
free(escaped);
```

### DateTime Operations

```c
// Parse datetime strings
struct tm tm;
ORM_ParseDateTime("2024-03-15 14:30:45", &tm);
printf("Year: %d, Month: %d, Day: %d\n", 
       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

// Format datetime
char output[64];
ORM_FormatDateTime(&tm, output, sizeof(output), NULL);
printf("Formatted: %s\n", output);

// Convert to/from timestamps
int64_t timestamp = ORM_DateTimeToTimestamp(&tm);
ORM_TimestampToDateTime(timestamp, &tm);
```

### Field Validation

```c
ORM_FieldMeta field = {
    .field_name = "username",
    .type = ORM_FIELD_STRING,
    .flags = ORM_NOT_NULL,
    .size = 50
};

// Validate string length
char* valid = "john_doe";
char* invalid = "this_is_a_very_long_username_that_exceeds_the_limit";

printf("Valid: %d\n", ORM_ValidateField(&field, valid));      // 1
printf("Invalid: %d\n", ORM_ValidateField(&field, invalid));  // 0
```

### Binary Data Operations

```c
// Convert binary data to hex
unsigned char data[] = {0xDE, 0xAD, 0xBE, 0xEF};
char hex[16];
ORM_BinaryToHex(data, sizeof(data), hex, sizeof(hex));
printf("Hex: %s\n", hex);  // Output: Hex: DEADBEEF

// Convert hex back to binary
unsigned char result[4];
ORM_HexToBinary("DEADBEEF", result, sizeof(result));
```

## Building and Testing

### Build the Library

```bash
make all
```

### Build and Run Tests

```bash
make test
```

This runs both test suites:
- **test_model**: 16 tests, 96 assertions (model system)
- **test_field_types**: 21 tests, 104 assertions (field types system)

### Build Examples

```bash
make examples
./build/bin/basic_usage
./build/bin/field_types_demo
```

### Clean Build

```bash
make clean
```

## Project Structure

```
orm-c/
├── src/
│   └── core/
│       ├── model.h          # Model system API and macros
│       ├── model.c          # Model implementation
│       ├── field_types.h    # Field type system API
│       ├── field_types.c    # Type conversion and validation
│       └── datetime.c       # DateTime parsing and formatting
├── tests/
│   ├── test_model.c         # Model system test suite (16 tests)
│   └── test_field_types.c   # Field types test suite (21 tests)
├── examples/
│   ├── basic_usage.c        # Basic model usage example
│   └── field_types_demo.c   # Field types demonstration
├── docs/
│   ├── requirements.md      # Project requirements
│   ├── prompt.md           # System prompts
│   └── tasks/              # Task specifications and completions
├── build/
│   ├── obj/                 # Object files
│   └── bin/                 # Executables (tests and examples)
├── Makefile                 # Build system
└── README.md               # This file
```

## Memory Management

The model system uses careful memory management:

1. **Registry**: Dynamically allocated, grows as needed (2x growth factor)
2. **Model Metadata**: Static data (defined by macros)
3. **Field Metadata**: Static arrays (defined by macros)
4. **Model Instances**: Dynamically allocated via `ModelType_new()`, must be freed with `ModelType_free()`

No memory leaks when following the proper lifecycle:
```c
ORM_Init();           // Allocate registry
// ... use models ...
ORM_Shutdown();       // Free registry
```

## Limitations

- Models must be manually registered after `ORM_Init()`
- Field names must match C struct member names
- String fields have fixed maximum sizes
- Constructor attribute-based auto-registration not used (for portability)

## Future Enhancements

**Completed:**
- [x] Model definition system (Task 01)
- [x] Field types with validation and conversion (Task 02)

**Upcoming:**
- [ ] Memory management and object lifecycle (Task 03)
- [ ] Query builder integration (Task 04)
- [ ] SQL generation from metadata (Task 05)
- [ ] SQLite backend integration (Task 06)
- [ ] Result set handling (Task 07)
- [ ] Relationship support (Task 08)
- [ ] Migration system (Task 09)
- [ ] Advanced validation (Task 10)
- [ ] Transaction support (Task 11)
- [ ] Connection pooling (Task 12)
- [ ] PostgreSQL backend (Task 13)
- [ ] MySQL backend (Task 14)
- [ ] Performance optimization (Task 15)

## License

This is an educational project demonstrating ORM concepts in C.
