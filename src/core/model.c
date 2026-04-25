#include "model.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Global registry instance
ORM_Registry* g_orm_registry = NULL;

// Default initial capacity for the registry
#define REGISTRY_INITIAL_CAPACITY 16
#define REGISTRY_GROWTH_FACTOR 2

// ============================================================================
// Core ORM Functions
// ============================================================================

int ORM_Init(void) {
    if (g_orm_registry != NULL) {
        // Already initialized
        return ORM_SUCCESS;
    }
    
    g_orm_registry = (ORM_Registry*)malloc(sizeof(ORM_Registry));
    if (g_orm_registry == NULL) {
        return ORM_ERROR_MEMORY;
    }
    
    g_orm_registry->capacity = REGISTRY_INITIAL_CAPACITY;
    g_orm_registry->count = 0;
    g_orm_registry->models = (ORM_ModelMeta**)calloc(
        g_orm_registry->capacity, 
        sizeof(ORM_ModelMeta*)
    );
    
    if (g_orm_registry->models == NULL) {
        free(g_orm_registry);
        g_orm_registry = NULL;
        return ORM_ERROR_MEMORY;
    }
    
    return ORM_SUCCESS;
}

void ORM_Shutdown(void) {
    if (g_orm_registry == NULL) {
        return;
    }
    
    // Free the models array (but not the metadata, as they're static)
    if (g_orm_registry->models != NULL) {
        free(g_orm_registry->models);
    }
    
    free(g_orm_registry);
    g_orm_registry = NULL;
}

// ============================================================================
// Model Registry Functions
// ============================================================================

static int registry_grow(void) {
    int new_capacity = g_orm_registry->capacity * REGISTRY_GROWTH_FACTOR;
    ORM_ModelMeta** new_models = (ORM_ModelMeta**)realloc(
        g_orm_registry->models,
        new_capacity * sizeof(ORM_ModelMeta*)
    );
    
    if (new_models == NULL) {
        return ORM_ERROR_MEMORY;
    }
    
    // Zero out new memory
    for (int i = g_orm_registry->capacity; i < new_capacity; i++) {
        new_models[i] = NULL;
    }
    
    g_orm_registry->models = new_models;
    g_orm_registry->capacity = new_capacity;
    
    return ORM_SUCCESS;
}

int ORM_RegisterModel(ORM_ModelMeta* meta) {
    if (meta == NULL) {
        return ORM_ERROR_INVALID_PARAM;
    }
    
    // Initialize registry if not already done
    if (g_orm_registry == NULL) {
        int result = ORM_Init();
        if (result != ORM_SUCCESS) {
            return result;
        }
    }
    
    // Check for duplicate
    if (ORM_GetModel(meta->model_name) != NULL) {
        return ORM_ERROR_DUPLICATE;
    }
    
    // Grow registry if needed
    if (g_orm_registry->count >= g_orm_registry->capacity) {
        int result = registry_grow();
        if (result != ORM_SUCCESS) {
            return result;
        }
    }
    
    // Add model to registry
    g_orm_registry->models[g_orm_registry->count] = meta;
    g_orm_registry->count++;
    
    return ORM_SUCCESS;
}

ORM_ModelMeta* ORM_GetModel(const char* name) {
    if (g_orm_registry == NULL || name == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < g_orm_registry->count; i++) {
        if (strcmp(g_orm_registry->models[i]->model_name, name) == 0) {
            return g_orm_registry->models[i];
        }
    }
    
    return NULL;
}

int ORM_UnregisterModel(const char* name) {
    if (g_orm_registry == NULL || name == NULL) {
        return ORM_ERROR_INVALID_PARAM;
    }
    
    for (int i = 0; i < g_orm_registry->count; i++) {
        if (strcmp(g_orm_registry->models[i]->model_name, name) == 0) {
            // Shift remaining models down
            for (int j = i; j < g_orm_registry->count - 1; j++) {
                g_orm_registry->models[j] = g_orm_registry->models[j + 1];
            }
            
            g_orm_registry->models[g_orm_registry->count - 1] = NULL;
            g_orm_registry->count--;
            
            return ORM_SUCCESS;
        }
    }
    
    return ORM_ERROR_NOT_FOUND;
}

int ORM_GetModelCount(void) {
    if (g_orm_registry == NULL) {
        return 0;
    }
    
    return g_orm_registry->count;
}

// ============================================================================
// Model Utility Functions
// ============================================================================

ORM_FieldMeta* ORM_GetField(ORM_ModelMeta* meta, const char* field_name) {
    if (meta == NULL || field_name == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < meta->field_count; i++) {
        if (strcmp(meta->fields[i].field_name, field_name) == 0) {
            return &meta->fields[i];
        }
    }
    
    return NULL;
}

ORM_FieldMeta* ORM_GetPrimaryKey(ORM_ModelMeta* meta) {
    if (meta == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < meta->field_count; i++) {
        if (meta->fields[i].flags & ORM_PRIMARY_KEY) {
            return &meta->fields[i];
        }
    }
    
    return NULL;
}

int ORM_FieldHasFlag(ORM_FieldMeta* field, int flag) {
    if (field == NULL) {
        return 0;
    }
    
    return (field->flags & flag) != 0;
}

void* ORM_GetFieldValue(void* obj, ORM_FieldMeta* field) {
    if (obj == NULL || field == NULL) {
        return NULL;
    }
    
    // Calculate the address of the field in the object
    char* obj_ptr = (char*)obj;
    return (void*)(obj_ptr + field->offset);
}

// ============================================================================
// Debug/Utility Functions
// ============================================================================

void ORM_PrintModelInfo(ORM_ModelMeta* meta) {
    if (meta == NULL) {
        printf("NULL model metadata\n");
        return;
    }
    
    printf("Model: %s\n", meta->model_name);
    printf("  Table: %s\n", meta->table_name);
    printf("  Size: %zu bytes\n", meta->struct_size);
    printf("  Fields: %d\n", meta->field_count);
    
    for (int i = 0; i < meta->field_count; i++) {
        ORM_FieldMeta* field = &meta->fields[i];
        printf("    [%d] %s (%s) - offset: %zu, size: %zu, flags: 0x%x\n",
               i,
               field->field_name,
               field->column_name,
               field->offset,
               field->size,
               field->flags);
    }
}

void ORM_PrintRegistry(void) {
    if (g_orm_registry == NULL) {
        printf("Registry not initialized\n");
        return;
    }
    
    printf("ORM Registry:\n");
    printf("  Count: %d\n", g_orm_registry->count);
    printf("  Capacity: %d\n", g_orm_registry->capacity);
    printf("  Models:\n");
    
    for (int i = 0; i < g_orm_registry->count; i++) {
        printf("    [%d] %s -> %s\n",
               i,
               g_orm_registry->models[i]->model_name,
               g_orm_registry->models[i]->table_name);
    }
}

const char* ORM_FieldTypeToString(ORM_FieldType type) {
    switch (type) {
        case ORM_TYPE_INT: return "INT";
        case ORM_TYPE_BIGINT: return "BIGINT";
        case ORM_TYPE_FLOAT: return "FLOAT";
        case ORM_TYPE_DOUBLE: return "DOUBLE";
        case ORM_TYPE_STRING: return "STRING";
        case ORM_TYPE_TEXT: return "TEXT";
        case ORM_TYPE_BOOLEAN: return "BOOLEAN";
        case ORM_TYPE_DATETIME: return "DATETIME";
        case ORM_TYPE_DATE: return "DATE";
        case ORM_TYPE_TIME: return "TIME";
        case ORM_TYPE_BLOB: return "BLOB";
        case ORM_TYPE_FOREIGN_KEY: return "FOREIGN_KEY";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// Default Value Functions
// ============================================================================

int ORM_SetDefaultValue(ORM_FieldMeta* field, void* value, size_t value_size) {
    if (field == NULL || value == NULL) {
        return ORM_ERROR_INVALID_PARAM;
    }
    
    // Allocate memory for the default value
    if (field->default_value != NULL) {
        free(field->default_value);
    }
    
    field->default_value = malloc(value_size);
    if (field->default_value == NULL) {
        return ORM_ERROR_MEMORY;
    }
    
    memcpy(field->default_value, value, value_size);
    field->flags |= ORM_HAS_DEFAULT;
    
    return ORM_SUCCESS;
}

int ORM_ApplyDefaults(ORM_ModelMeta* meta, void* obj) {
    if (meta == NULL || obj == NULL) {
        return ORM_ERROR_INVALID_PARAM;
    }
    
    for (int i = 0; i < meta->field_count; i++) {
        ORM_FieldMeta* field = &meta->fields[i];
        
        // Skip if no default value is set
        if (!(field->flags & ORM_HAS_DEFAULT) || field->default_value == NULL) {
            continue;
        }
        
        // Get pointer to the field in the object
        void* field_ptr = ORM_GetFieldValue(obj, field);
        if (field_ptr == NULL) {
            continue;
        }
        
        // Copy the default value to the field
        switch (field->type) {
            case ORM_TYPE_INT:
            case ORM_TYPE_BOOLEAN:
            case ORM_TYPE_FOREIGN_KEY:
                *(int*)field_ptr = *(int*)field->default_value;
                break;
                
            case ORM_TYPE_BIGINT:
            case ORM_TYPE_DATE:
            case ORM_TYPE_TIME:
            case ORM_TYPE_DATETIME:
                *(int64_t*)field_ptr = *(int64_t*)field->default_value;
                break;
                
            case ORM_TYPE_FLOAT:
                *(float*)field_ptr = *(float*)field->default_value;
                break;
                
            case ORM_TYPE_DOUBLE:
                *(double*)field_ptr = *(double*)field->default_value;
                break;
                
            case ORM_TYPE_STRING:
            case ORM_TYPE_TEXT:
                strncpy((char*)field_ptr, (char*)field->default_value, field->size - 1);
                ((char*)field_ptr)[field->size - 1] = '\0'; // Ensure null termination
                break;
                
            case ORM_TYPE_BLOB:
                memcpy(field_ptr, field->default_value, field->size);
                break;
                
            default:
                // Unknown type, skip
                break;
        }
    }
    
    return ORM_SUCCESS;
}
