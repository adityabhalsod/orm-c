/* SQL Dialect-Specific Code
 * 
 * Type mappings and dialect-specific SQL generation.
 */

#define _GNU_SOURCE
#include "sql_builder.h"
#include "model.h"
#include "field_types.h"
#include "../utils/memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Type Name Mapping
// ============================================================================

const char* ORM_DialectTypeName(ORM_FieldType type, ORM_Dialect dialect) {
    switch (type) {
        case ORM_TYPE_INT:
            switch (dialect) {
                case ORM_DIALECT_POSTGRESQL:
                    return "INTEGER";
                case ORM_DIALECT_MYSQL:
                    return "INT";
                case ORM_DIALECT_SQLITE:
                    return "INTEGER";
                case ORM_DIALECT_MSSQL:
                    return "INT";
                default:
                    return "INTEGER";
            }
            
        case ORM_TYPE_BIGINT:
            switch (dialect) {
                case ORM_DIALECT_POSTGRESQL:
                case ORM_DIALECT_MYSQL:
                case ORM_DIALECT_MSSQL:
                    return "BIGINT";
                case ORM_DIALECT_SQLITE:
                    return "INTEGER";
                default:
                    return "BIGINT";
            }
            
        case ORM_TYPE_FLOAT:
            switch (dialect) {
                case ORM_DIALECT_POSTGRESQL:
                    return "REAL";
                case ORM_DIALECT_MYSQL:
                case ORM_DIALECT_MSSQL:
                    return "FLOAT";
                case ORM_DIALECT_SQLITE:
                    return "REAL";
                default:
                    return "REAL";
            }
            
        case ORM_TYPE_DOUBLE:
            switch (dialect) {
                case ORM_DIALECT_POSTGRESQL:
                    return "DOUBLE PRECISION";
                case ORM_DIALECT_MYSQL:
                case ORM_DIALECT_MSSQL:
                    return "DOUBLE";
                case ORM_DIALECT_SQLITE:
                    return "REAL";
                default:
                    return "DOUBLE PRECISION";
            }
            
        case ORM_TYPE_BOOLEAN:
            switch (dialect) {
                case ORM_DIALECT_POSTGRESQL:
                    return "BOOLEAN";
                case ORM_DIALECT_MYSQL:
                    return "TINYINT(1)";
                case ORM_DIALECT_SQLITE:
                    return "INTEGER";
                case ORM_DIALECT_MSSQL:
                    return "BIT";
                default:
                    return "BOOLEAN";
            }
            
        case ORM_TYPE_STRING:
            return "VARCHAR";
            
        case ORM_TYPE_TEXT:
            switch (dialect) {
                case ORM_DIALECT_POSTGRESQL:
                case ORM_DIALECT_MYSQL:
                case ORM_DIALECT_SQLITE:
                    return "TEXT";
                case ORM_DIALECT_MSSQL:
                    return "NVARCHAR(MAX)";
                default:
                    return "TEXT";
            }
            
        case ORM_TYPE_DATETIME:
            switch (dialect) {
                case ORM_DIALECT_POSTGRESQL:
                    return "TIMESTAMP";
                case ORM_DIALECT_MYSQL:
                    return "DATETIME";
                case ORM_DIALECT_SQLITE:
                    return "TEXT";
                case ORM_DIALECT_MSSQL:
                    return "DATETIME2";
                default:
                    return "TIMESTAMP";
            }
            
        case ORM_TYPE_DATE:
            switch (dialect) {
                case ORM_DIALECT_SQLITE:
                    return "TEXT";
                default:
                    return "DATE";
            }
            
        case ORM_TYPE_TIME:
            switch (dialect) {
                case ORM_DIALECT_SQLITE:
                    return "TEXT";
                default:
                    return "TIME";
            }
            
        case ORM_TYPE_BLOB:
            switch (dialect) {
                case ORM_DIALECT_POSTGRESQL:
                    return "BYTEA";
                case ORM_DIALECT_MYSQL:
                    return "BLOB";
                case ORM_DIALECT_SQLITE:
                    return "BLOB";
                case ORM_DIALECT_MSSQL:
                    return "VARBINARY(MAX)";
                default:
                    return "BLOB";
            }
            
        default:
            return "TEXT";
    }
}

// ============================================================================
// Column Definition Builder
// ============================================================================

char* ORM_BuildColumnDefinition(ORM_FieldMeta* field, ORM_Dialect dialect) {
    if (!field) {
        return NULL;
    }
    
    ORM_StringBuffer* def = ORM_StringBufferNew(128);
    if (!def) {
        return NULL;
    }
    
    // Column name
    char* quoted = ORM_QuoteIdentifier(field->column_name, dialect);
    ORM_StringBufferAppend(def, quoted);
    free(quoted);
    
    ORM_StringBufferAppend(def, " ");
    
    // Type
    const char* type_name = ORM_DialectTypeName(field->type, dialect);
    ORM_StringBufferAppend(def, type_name);
    
    // VARCHAR max_length - Note: ORM_FieldMeta doesn't store max_length separately
    // It's embedded in the type or we need to extract it differently
    
    // PRIMARY KEY
    if (field->flags & ORM_PRIMARY_KEY) {
        ORM_StringBufferAppend(def, " PRIMARY KEY");
        
        // AUTO INCREMENT
        if (field->flags & ORM_AUTO_INCREMENT) {
            switch (dialect) {
                case ORM_DIALECT_SQLITE:
                    ORM_StringBufferAppend(def, " AUTOINCREMENT");
                    break;
                case ORM_DIALECT_MYSQL:
                    ORM_StringBufferAppend(def, " AUTO_INCREMENT");
                    break;
                case ORM_DIALECT_POSTGRESQL:
                    // PostgreSQL uses SERIAL type instead
                    break;
                case ORM_DIALECT_MSSQL:
                    ORM_StringBufferAppend(def, " IDENTITY(1,1)");
                    break;
            }
        }
    }
    
    // UNIQUE
    if (field->flags & ORM_UNIQUE && !(field->flags & ORM_PRIMARY_KEY)) {
        ORM_StringBufferAppend(def, " UNIQUE");
    }
    
    // NULL / NOT NULL
    if (field->flags & ORM_NOT_NULL && !(field->flags & ORM_PRIMARY_KEY)) {
        ORM_StringBufferAppend(def, " NOT NULL");
    }
    
    // DEFAULT
    if (field->flags & ORM_HAS_DEFAULT && !(field->flags & ORM_AUTO_INCREMENT)) {
        ORM_StringBufferAppend(def, " DEFAULT ");
        
        if (field->default_value) {
            switch (field->type) {
                case ORM_TYPE_INT:
                case ORM_TYPE_BIGINT: {
                    int64_t val = *(int64_t*)field->default_value;
                    ORM_StringBufferAppendF(def, "%lld", val);
                    break;
                }
                
                case ORM_TYPE_FLOAT:
                case ORM_TYPE_DOUBLE: {
                    double val = *(double*)field->default_value;
                    ORM_StringBufferAppendF(def, "%f", val);
                    break;
                }
                
                case ORM_TYPE_BOOLEAN: {
                    bool val = *(bool*)field->default_value;
                    if (dialect == ORM_DIALECT_POSTGRESQL) {
                        ORM_StringBufferAppend(def, val ? "TRUE" : "FALSE");
                    } else {
                        ORM_StringBufferAppendF(def, "%d", val ? 1 : 0);
                    }
                    break;
                }
                
                case ORM_TYPE_STRING: {
                    char* escaped = ORM_EscapeSQLString((const char*)field->default_value, dialect);
                    ORM_StringBufferAppendF(def, "'%s'", escaped);
                    free(escaped);
                    break;
                }
                    
                default:
                    ORM_StringBufferAppend(def, "NULL");
                    break;
            }
        } else {
            ORM_StringBufferAppend(def, "NULL");
        }
    }
    
    char* result = strdup(def->data);
    ORM_StringBufferFree(def);
    return result;
}

// ============================================================================
// Dialect Feature Detection
// ============================================================================

bool ORM_SupportsFeature(ORM_Dialect dialect, const char* feature) {
    if (!feature) {
        return false;
    }
    
    if (strcmp(feature, "RETURNING") == 0) {
        return dialect == ORM_DIALECT_POSTGRESQL;
    }
    
    if (strcmp(feature, "ON CONFLICT") == 0) {
        return dialect == ORM_DIALECT_POSTGRESQL || dialect == ORM_DIALECT_SQLITE;
    }
    
    if (strcmp(feature, "ILIKE") == 0) {
        return dialect == ORM_DIALECT_POSTGRESQL;
    }
    
    if (strcmp(feature, "LIMIT") == 0) {
        return true; // All support LIMIT
    }
    
    if (strcmp(feature, "OFFSET") == 0) {
        return true; // All support OFFSET
    }
    
    if (strcmp(feature, "AUTOINCREMENT") == 0) {
        return true; // All support some form
    }
    
    return false;
}
