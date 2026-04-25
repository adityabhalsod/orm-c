#ifndef ORM_MODEL_H
#define ORM_MODEL_H

#include <stddef.h>
#include <stdint.h>

// Forward declarations
typedef struct ORM_ModelMeta ORM_ModelMeta;
typedef struct ORM_FieldMeta ORM_FieldMeta;
typedef struct ORM_Registry ORM_Registry;
typedef struct QuerySet QuerySet;

// Return codes
#define ORM_SUCCESS 0
#define ORM_ERROR -1
#define ORM_ERROR_MEMORY -2
#define ORM_ERROR_NOT_FOUND -3
#define ORM_ERROR_DUPLICATE -4
#define ORM_ERROR_INVALID_PARAM -5

// Field types
typedef enum {
    ORM_TYPE_INT,
    ORM_TYPE_BIGINT,
    ORM_TYPE_FLOAT,
    ORM_TYPE_DOUBLE,
    ORM_TYPE_STRING,
    ORM_TYPE_TEXT,
    ORM_TYPE_BOOLEAN,
    ORM_TYPE_DATETIME,
    ORM_TYPE_DATE,
    ORM_TYPE_TIME,
    ORM_TYPE_BLOB,
    ORM_TYPE_FOREIGN_KEY
} ORM_FieldType;

// Field flags (can be combined with bitwise OR)
#define ORM_PRIMARY_KEY    (1 << 0)   // 0x01
#define ORM_AUTO_INCREMENT (1 << 1)   // 0x02
#define ORM_NOT_NULL       (1 << 2)   // 0x04
#define ORM_UNIQUE         (1 << 3)   // 0x08
#define ORM_INDEX          (1 << 4)   // 0x10
#define ORM_HAS_DEFAULT    (1 << 5)   // 0x20

// Field metadata structure
struct ORM_FieldMeta {
    const char* field_name;      // Name in C struct
    const char* column_name;     // Name in database
    ORM_FieldType type;          // Field type
    size_t offset;               // Offset in struct
    size_t size;                 // Field size in bytes
    int flags;                   // Constraint flags
    void* default_value;         // Default value (if any)
};

// Model metadata structure
struct ORM_ModelMeta {
    const char* model_name;      // C struct name
    const char* table_name;      // Database table name
    size_t struct_size;          // Size of the struct
    int field_count;             // Number of fields
    ORM_FieldMeta* fields;       // Array of field metadata
    void* (*constructor)(void);  // Constructor function
    void (*destructor)(void*);   // Destructor function
};

// Model registry structure
struct ORM_Registry {
    ORM_ModelMeta** models;      // Array of model pointers
    int count;                   // Current number of models
    int capacity;                // Allocated capacity
};

// Global registry instance
extern ORM_Registry* g_orm_registry;

// ============================================================================
// Core ORM Functions
// ============================================================================

/**
 * Initialize the ORM system
 * Must be called before using any ORM functions
 */
int ORM_Init(void);

/**
 * Shutdown the ORM system and free all resources
 */
void ORM_Shutdown(void);

// ============================================================================
// Model Registry Functions
// ============================================================================

/**
 * Register a model with the ORM system
 * @param meta Pointer to the model metadata
 * @return ORM_SUCCESS on success, error code otherwise
 */
int ORM_RegisterModel(ORM_ModelMeta* meta);

/**
 * Get a model by name
 * @param name Model name to lookup
 * @return Pointer to model metadata or NULL if not found
 */
ORM_ModelMeta* ORM_GetModel(const char* name);

/**
 * Unregister a model from the ORM system
 * @param name Model name to unregister
 * @return ORM_SUCCESS on success, error code otherwise
 */
int ORM_UnregisterModel(const char* name);

/**
 * Get the number of registered models
 * @return Number of registered models
 */
int ORM_GetModelCount(void);

// ============================================================================
// Model Utility Functions
// ============================================================================

/**
 * Get field metadata by name
 * @param meta Model metadata
 * @param field_name Field name to lookup
 * @return Pointer to field metadata or NULL if not found
 */
ORM_FieldMeta* ORM_GetField(ORM_ModelMeta* meta, const char* field_name);

/**
 * Get the primary key field of a model
 * @param meta Model metadata
 * @return Pointer to primary key field or NULL if not found
 */
ORM_FieldMeta* ORM_GetPrimaryKey(ORM_ModelMeta* meta);

/**
 * Check if a field has a specific flag set
 * @param field Field metadata
 * @param flag Flag to check
 * @return 1 if flag is set, 0 otherwise
 */
int ORM_FieldHasFlag(ORM_FieldMeta* field, int flag);

/**
 * Get field value as void pointer
 * @param obj Instance of the model
 * @param field Field metadata
 * @return Pointer to field value in the object
 */
void* ORM_GetFieldValue(void* obj, ORM_FieldMeta* field);

/**
 * Convert field type enum to string
 * @param type Field type
 * @return String representation of the type
 */
const char* ORM_FieldTypeToString(ORM_FieldType type);

/**
 * Apply default values to a model instance
 * @param meta Model metadata
 * @param obj Instance to apply defaults to
 * @return ORM_SUCCESS on success, error code otherwise
 */
int ORM_ApplyDefaults(ORM_ModelMeta* meta, void* obj);

/**
 * Set a default value for a field
 * @param field Field metadata
 * @param value Pointer to the default value
 * @param value_size Size of the default value
 * @return ORM_SUCCESS on success, error code otherwise
 */
int ORM_SetDefaultValue(ORM_FieldMeta* field, void* value, size_t value_size);

// ============================================================================
// Macro Helpers for Field Definitions
// ============================================================================

// Helper macro to get the current model type in field macros
#define ORM_CURRENT_MODEL ModelType

// Integer field
#define ORM_FIELD_INT(name, field_flags) \
    { \
        .field_name = #name, \
        .column_name = #name, \
        .type = ORM_TYPE_INT, \
        .offset = offsetof(ORM_CURRENT_MODEL, name), \
        .size = sizeof(int), \
        .flags = (field_flags), \
        .default_value = NULL \
    }

// Big integer field
#define ORM_FIELD_BIGINT(name, field_flags) \
    { \
        .field_name = #name, \
        .column_name = #name, \
        .type = ORM_TYPE_BIGINT, \
        .offset = offsetof(ORM_CURRENT_MODEL, name), \
        .size = sizeof(int64_t), \
        .flags = (field_flags), \
        .default_value = NULL \
    }

// Float field
#define ORM_FIELD_FLOAT(name, field_flags) \
    { \
        .field_name = #name, \
        .column_name = #name, \
        .type = ORM_TYPE_FLOAT, \
        .offset = offsetof(ORM_CURRENT_MODEL, name), \
        .size = sizeof(float), \
        .flags = (field_flags), \
        .default_value = NULL \
    }

// Double field
#define ORM_FIELD_DOUBLE(name, field_flags) \
    { \
        .field_name = #name, \
        .column_name = #name, \
        .type = ORM_TYPE_DOUBLE, \
        .offset = offsetof(ORM_CURRENT_MODEL, name), \
        .size = sizeof(double), \
        .flags = (field_flags), \
        .default_value = NULL \
    }

// String field
#define ORM_FIELD_STRING(name, max_size, field_flags) \
    { \
        .field_name = #name, \
        .column_name = #name, \
        .type = ORM_TYPE_STRING, \
        .offset = offsetof(ORM_CURRENT_MODEL, name), \
        .size = (max_size), \
        .flags = (field_flags), \
        .default_value = NULL \
    }

// Boolean field
#define ORM_FIELD_BOOLEAN(name, field_flags) \
    { \
        .field_name = #name, \
        .column_name = #name, \
        .type = ORM_TYPE_BOOLEAN, \
        .offset = offsetof(ORM_CURRENT_MODEL, name), \
        .size = sizeof(int), \
        .flags = (field_flags), \
        .default_value = NULL \
    }

// Text field (large text)
#define ORM_FIELD_TEXT(name, max_size, field_flags) \
    { \
        .field_name = #name, \
        .column_name = #name, \
        .type = ORM_TYPE_TEXT, \
        .offset = offsetof(ORM_CURRENT_MODEL, name), \
        .size = (max_size), \
        .flags = (field_flags), \
        .default_value = NULL \
    }

// Date field
#define ORM_FIELD_DATE(name, field_flags) \
    { \
        .field_name = #name, \
        .column_name = #name, \
        .type = ORM_TYPE_DATE, \
        .offset = offsetof(ORM_CURRENT_MODEL, name), \
        .size = sizeof(int64_t), \
        .flags = (field_flags), \
        .default_value = NULL \
    }

// Time field
#define ORM_FIELD_TIME(name, field_flags) \
    { \
        .field_name = #name, \
        .column_name = #name, \
        .type = ORM_TYPE_TIME, \
        .offset = offsetof(ORM_CURRENT_MODEL, name), \
        .size = sizeof(int64_t), \
        .flags = (field_flags), \
        .default_value = NULL \
    }

// DateTime field
#define ORM_FIELD_DATETIME(name, field_flags) \
    { \
        .field_name = #name, \
        .column_name = #name, \
        .type = ORM_TYPE_DATETIME, \
        .offset = offsetof(ORM_CURRENT_MODEL, name), \
        .size = sizeof(int64_t), \
        .flags = (field_flags), \
        .default_value = NULL \
    }

// Blob field (binary data)
#define ORM_FIELD_BLOB(name, max_size, field_flags) \
    { \
        .field_name = #name, \
        .column_name = #name, \
        .type = ORM_TYPE_BLOB, \
        .offset = offsetof(ORM_CURRENT_MODEL, name), \
        .size = (max_size), \
        .flags = (field_flags), \
        .default_value = NULL \
    }

// Foreign key field
#define ORM_FIELD_FOREIGN_KEY(name, field_flags) \
    { \
        .field_name = #name, \
        .column_name = #name, \
        .type = ORM_TYPE_FOREIGN_KEY, \
        .offset = offsetof(ORM_CURRENT_MODEL, name), \
        .size = sizeof(int), \
        .flags = (field_flags), \
        .default_value = NULL \
    }

// ============================================================================
// Main Model Definition Macro
// ============================================================================

/**
 * Define and register a model with the ORM system
 * This macro creates the metadata, field definitions, and constructor/destructor
 * Note: Models must be manually registered using ORM_RegisterModel() after ORM_Init()
 * 
 * Usage:
 * ORM_MODEL_DEFINE(User, "users",
 *     ORM_FIELD_INT(id, ORM_PRIMARY_KEY | ORM_AUTO_INCREMENT),
 *     ORM_FIELD_STRING(username, 50, ORM_NOT_NULL)
 * );
 */
#define ORM_MODEL_DEFINE(ModelType, table_name_str, ...) \
    /* Forward declare constructor and destructor */ \
    void* ModelType##_new(void); \
    void ModelType##_free(void* obj); \
    \
    /* Forward declare CRUD functions */ \
    int ModelType##_save(ModelType* obj); \
    ModelType* ModelType##_get(int id); \
    QuerySet* ModelType##_objects(void); \
    int ModelType##_delete(ModelType* obj); \
    \
    /* Define field metadata array */ \
    ORM_FieldMeta ModelType##_fields[] = { __VA_ARGS__ }; \
    \
    /* Define model metadata */ \
    ORM_ModelMeta ModelType##_meta = { \
        #ModelType, \
        table_name_str, \
        sizeof(ModelType), \
        sizeof(ModelType##_fields) / sizeof(ORM_FieldMeta), \
        ModelType##_fields, \
        ModelType##_new, \
        ModelType##_free \
    }; \
    \
    /* Implement constructor */ \
    void* ModelType##_new(void) { \
        ModelType* obj = (ModelType*)calloc(1, sizeof(ModelType)); \
        return obj; \
    } \
    \
    /* Implement destructor */ \
    void ModelType##_free(void* obj) { \
        if (obj) { \
            free(obj); \
        } \
    } \
    \
    /* Implement CRUD function stubs (placeholders for database integration) */ \
    int ModelType##_save(ModelType* obj) { \
        /* TODO: Implement database save operation */ \
        /* This will be implemented when database backend is available */ \
        (void)obj; /* Suppress unused parameter warning */ \
        return ORM_ERROR; /* Not yet implemented */ \
    } \
    \
    ModelType* ModelType##_get(int id) { \
        /* TODO: Implement database get operation */ \
        /* This will be implemented when database backend is available */ \
        (void)id; /* Suppress unused parameter warning */ \
        return NULL; /* Not yet implemented */ \
    } \
    \
    QuerySet* ModelType##_objects(void) { \
        /* TODO: Implement QuerySet creation */ \
        /* This will be implemented with the query builder */ \
        return NULL; /* Not yet implemented */ \
    } \
    \
    int ModelType##_delete(ModelType* obj) { \
        /* TODO: Implement database delete operation */ \
        /* This will be implemented when database backend is available */ \
        (void)obj; /* Suppress unused parameter warning */ \
        return ORM_ERROR; /* Not yet implemented */ \
    }

/**
 * Declare CRUD functions for a model
 * This is automatically called by ORM_MODEL_DEFINE
 * Can be used separately if you want to manually declare functions
 */
#define ORM_DECLARE_CRUD(ModelType) \
    int ModelType##_save(ModelType* obj); \
    ModelType* ModelType##_get(int id); \
    QuerySet* ModelType##_objects(void); \
    int ModelType##_delete(ModelType* obj);

#endif // ORM_MODEL_H
