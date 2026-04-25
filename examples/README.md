# ORM-C Examples

This directory contains comprehensive examples demonstrating the features of the ORM-C library.

## Examples Overview

### 1. basic_usage.c
**Purpose**: Introduction to basic ORM concepts and operations

**Features Demonstrated**:
- Creating model instances
- Model registration
- Field access and manipulation
- Using generated constructors/destructors
- Model introspection
- Working with multiple models

**Compile & Run**:
```bash
gcc -o build/bin/basic_usage examples/basic_usage.c src/core/model.c -I. -Wall -Wextra
./build/bin/basic_usage
```

### 2. advanced_features.c
**Purpose**: Showcase all advanced ORM features including new enhancements

**Features Demonstrated**:
- **All 12 field types**: INT, BIGINT, FLOAT, DOUBLE, STRING, TEXT, BOOLEAN, DATE, TIME, DATETIME, BLOB, FOREIGN_KEY
- **Default value system**: Setting and applying default values
- **CRUD function stubs**: Save, Get, Objects, Delete (placeholders)
- **Dynamic field access**: Generic field manipulation
- **Model introspection**: Exploring model metadata
- **Registry operations**: Working with multiple models
- **Constraint checking**: Verifying field flags and constraints

**Compile & Run**:
```bash
gcc -o build/bin/advanced_features examples/advanced_features.c src/core/model.c -I. -Wall -Wextra
./build/bin/advanced_features
```

## Quick Start

### Defining a Model

```c
#include "src/core/model.h"

// 1. Define your struct
typedef struct {
    int id;
    char username[50];
    char email[100];
    int age;
} User;

// 2. Set the current model context
#undef ORM_CURRENT_MODEL
#define ORM_CURRENT_MODEL User

// 3. Define the model with ORM_MODEL_DEFINE
ORM_MODEL_DEFINE(User, "users",
    ORM_FIELD_INT(id, ORM_PRIMARY_KEY | ORM_AUTO_INCREMENT),
    ORM_FIELD_STRING(username, 50, ORM_NOT_NULL | ORM_UNIQUE),
    ORM_FIELD_STRING(email, 100, ORM_NOT_NULL),
    ORM_FIELD_INT(age, 0)
);
```

This automatically generates:
- `User_new()` - Constructor
- `User_free()` - Destructor
- `User_save()` - Save to database (stub)
- `User_get()` - Retrieve by ID (stub)
- `User_objects()` - Create QuerySet (stub)
- `User_delete()` - Delete from database (stub)
- `User_meta` - Model metadata
- `User_fields[]` - Field metadata array

### Using the Model

```c
int main(void) {
    // Initialize ORM
    ORM_Init();
    
    // Register the model
    ORM_RegisterModel(&User_meta);
    
    // Create an instance
    User* user = User_new();
    user->id = 1;
    strcpy(user->username, "john_doe");
    strcpy(user->email, "john@example.com");
    user->age = 25;
    
    // Use the instance
    printf("User: %s (%s), Age: %d\n", 
           user->username, user->email, user->age);
    
    // Clean up
    User_free(user);
    ORM_Shutdown();
    
    return 0;
}
```

## Available Field Types

| Macro | C Type | Database Type | Description |
|-------|--------|---------------|-------------|
| `ORM_FIELD_INT` | `int` | INTEGER | 32-bit integers |
| `ORM_FIELD_BIGINT` | `int64_t` | BIGINT | 64-bit integers |
| `ORM_FIELD_FLOAT` | `float` | REAL | Single precision |
| `ORM_FIELD_DOUBLE` | `double` | REAL | Double precision |
| `ORM_FIELD_STRING` | `char[]` | VARCHAR | Fixed-size strings |
| `ORM_FIELD_TEXT` | `char[]` | TEXT | Large text |
| `ORM_FIELD_BOOLEAN` | `int` | BOOLEAN | Boolean values |
| `ORM_FIELD_DATE` | `int64_t` | DATE | Date only |
| `ORM_FIELD_TIME` | `int64_t` | TIME | Time only |
| `ORM_FIELD_DATETIME` | `int64_t` | DATETIME | Date and time |
| `ORM_FIELD_BLOB` | `char[]` | BLOB | Binary data |
| `ORM_FIELD_FOREIGN_KEY` | `int` | INTEGER | Foreign key reference |

## Field Flags (Constraints)

Combine flags with bitwise OR (`|`):

| Flag | Value | Description |
|------|-------|-------------|
| `ORM_PRIMARY_KEY` | 0x01 | Primary key field |
| `ORM_AUTO_INCREMENT` | 0x02 | Auto-incrementing |
| `ORM_NOT_NULL` | 0x04 | Cannot be NULL |
| `ORM_UNIQUE` | 0x08 | Must be unique |
| `ORM_INDEX` | 0x10 | Should be indexed |
| `ORM_HAS_DEFAULT` | 0x20 | Has default value |

Example:
```c
ORM_FIELD_INT(id, ORM_PRIMARY_KEY | ORM_AUTO_INCREMENT)
ORM_FIELD_STRING(email, 100, ORM_NOT_NULL | ORM_UNIQUE)
```

## Default Values

Set default values for fields:

```c
ORM_ModelMeta* meta = ORM_GetModel("User");
ORM_FieldMeta* age_field = ORM_GetField(meta, "age");

int default_age = 18;
ORM_SetDefaultValue(age_field, &default_age, sizeof(int));

// Later, when creating instances:
User* user = User_new();
ORM_ApplyDefaults(meta, user);  // user->age is now 18
```

## Model Introspection

Access model metadata at runtime:

```c
ORM_ModelMeta* meta = ORM_GetModel("User");

printf("Model: %s\n", meta->model_name);
printf("Table: %s\n", meta->table_name);
printf("Fields: %d\n", meta->field_count);

// Iterate through fields
for (int i = 0; i < meta->field_count; i++) {
    ORM_FieldMeta* field = &meta->fields[i];
    printf("  %s: %s\n", field->field_name, 
           ORM_FieldTypeToString(field->type));
}
```

## Dynamic Field Access

Access fields generically without knowing the struct layout:

```c
ORM_ModelMeta* meta = ORM_GetModel("User");
ORM_FieldMeta* username_field = ORM_GetField(meta, "username");

User* user = User_new();
char* username_ptr = (char*)ORM_GetFieldValue(user, username_field);
strcpy(username_ptr, "dynamic_user");
```

## Complete Example: All Field Types

```c
typedef struct {
    int id;
    char title[200];
    char content[2000];
    int is_published;
    int64_t created_at;
    int64_t view_count;
    float rating;
    double revenue;
    char metadata[512];
    int author_id;
} BlogPost;

#undef ORM_CURRENT_MODEL
#define ORM_CURRENT_MODEL BlogPost

ORM_MODEL_DEFINE(BlogPost, "blog_posts",
    ORM_FIELD_INT(id, ORM_PRIMARY_KEY | ORM_AUTO_INCREMENT),
    ORM_FIELD_STRING(title, 200, ORM_NOT_NULL),
    ORM_FIELD_TEXT(content, 2000, ORM_NOT_NULL),
    ORM_FIELD_BOOLEAN(is_published, 0),
    ORM_FIELD_DATETIME(created_at, ORM_NOT_NULL),
    ORM_FIELD_BIGINT(view_count, 0),
    ORM_FIELD_FLOAT(rating, 0),
    ORM_FIELD_DOUBLE(revenue, 0),
    ORM_FIELD_BLOB(metadata, 512, 0),
    ORM_FIELD_FOREIGN_KEY(author_id, 0)
);
```

## Tips and Best Practices

1. **Always initialize ORM**: Call `ORM_Init()` before using any ORM functions
2. **Register models**: Call `ORM_RegisterModel()` for each model after `ORM_Init()`
3. **Clean up**: Call `ORM_Shutdown()` to free all resources
4. **Use the macros**: Don't manually create metadata - use `ORM_MODEL_DEFINE`
5. **Check return codes**: Most functions return error codes - check them!
6. **Free instances**: Always call `ModelType_free()` on instances you create
7. **Redefine ORM_CURRENT_MODEL**: Before each `ORM_MODEL_DEFINE`, set the current model

## Error Codes

| Code | Constant | Meaning |
|------|----------|---------|
| 0 | `ORM_SUCCESS` | Operation succeeded |
| -1 | `ORM_ERROR` | General error |
| -2 | `ORM_ERROR_MEMORY` | Memory allocation failed |
| -3 | `ORM_ERROR_NOT_FOUND` | Model/field not found |
| -4 | `ORM_ERROR_DUPLICATE` | Duplicate model name |
| -5 | `ORM_ERROR_INVALID_PARAM` | Invalid parameter |

## API Reference

### Core Functions

- `int ORM_Init(void)` - Initialize the ORM system
- `void ORM_Shutdown(void)` - Shutdown and cleanup
- `int ORM_RegisterModel(ORM_ModelMeta* meta)` - Register a model
- `ORM_ModelMeta* ORM_GetModel(const char* name)` - Get model by name
- `int ORM_UnregisterModel(const char* name)` - Unregister a model
- `int ORM_GetModelCount(void)` - Get number of registered models

### Field Functions

- `ORM_FieldMeta* ORM_GetField(ORM_ModelMeta* meta, const char* field_name)` - Get field by name
- `ORM_FieldMeta* ORM_GetPrimaryKey(ORM_ModelMeta* meta)` - Get primary key field
- `int ORM_FieldHasFlag(ORM_FieldMeta* field, int flag)` - Check if field has flag
- `void* ORM_GetFieldValue(void* obj, ORM_FieldMeta* field)` - Get field value pointer
- `const char* ORM_FieldTypeToString(ORM_FieldType type)` - Convert type to string

### Default Value Functions

- `int ORM_SetDefaultValue(ORM_FieldMeta* field, void* value, size_t value_size)` - Set default value
- `int ORM_ApplyDefaults(ORM_ModelMeta* meta, void* obj)` - Apply all defaults to instance

## Next Steps

Once database backends are implemented (Tasks 06, 13, 14), the CRUD functions will be fully functional:
- `ModelType_save(obj)` - Persist to database
- `ModelType_get(id)` - Retrieve by primary key
- `ModelType_objects()` - Create QuerySet for complex queries
- `ModelType_delete(obj)` - Remove from database

## Support

For more information, see:
- Main README: `../README.md`
- Task completion: `../docs/tasks/task-01-completion.md`
- Tests: `../tests/test_model.c`
