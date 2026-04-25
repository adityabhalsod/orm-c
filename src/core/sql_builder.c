/* SQL Builder Implementation
 * 
 * Core SQL generation logic.
 */

#define _GNU_SOURCE
#include "sql_builder.h"
#include "model.h"
#include "queryset.h"
#include "../utils/memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Dialect Names
// ============================================================================

const char* ORM_DialectName(ORM_Dialect dialect) {
    switch (dialect) {
        case ORM_DIALECT_SQLITE: return "SQLite";
        case ORM_DIALECT_POSTGRESQL: return "PostgreSQL";
        case ORM_DIALECT_MYSQL: return "MySQL";
        case ORM_DIALECT_MSSQL: return "MS SQL Server";
        default: return "Unknown";
    }
}

// ============================================================================
// Builder Lifecycle
// ============================================================================

ORM_SQLBuilder* ORM_CreateSQLBuilder(ORM_Dialect dialect) {
    ORM_SQLBuilder* builder = (ORM_SQLBuilder*)malloc(sizeof(ORM_SQLBuilder));
    if (!builder) {
        return NULL;
    }
    
    builder->dialect = dialect;
    builder->sql = ORM_StringBufferNew(512);
    builder->select_clause = ORM_StringBufferNew(256);
    builder->from_clause = ORM_StringBufferNew(128);
    builder->where_clause = ORM_StringBufferNew(256);
    builder->join_clause = ORM_StringBufferNew(256);
    builder->order_clause = ORM_StringBufferNew(128);
    builder->group_clause = ORM_StringBufferNew(128);
    
    builder->param_capacity = 16;
    builder->parameters = (ORM_Parameter*)malloc(sizeof(ORM_Parameter) * builder->param_capacity);
    builder->param_count = 0;
    
    return builder;
}

void ORM_FreeSQLBuilder(ORM_SQLBuilder* builder) {
    if (!builder) {
        return;
    }
    
    if (builder->sql) ORM_StringBufferFree(builder->sql);
    if (builder->select_clause) ORM_StringBufferFree(builder->select_clause);
    if (builder->from_clause) ORM_StringBufferFree(builder->from_clause);
    if (builder->where_clause) ORM_StringBufferFree(builder->where_clause);
    if (builder->join_clause) ORM_StringBufferFree(builder->join_clause);
    if (builder->order_clause) ORM_StringBufferFree(builder->order_clause);
    if (builder->group_clause) ORM_StringBufferFree(builder->group_clause);
    
    if (builder->parameters) {
        free(builder->parameters);
    }
    
    free(builder);
}

void ORM_ResetSQLBuilder(ORM_SQLBuilder* builder) {
    if (!builder) {
        return;
    }
    
    ORM_StringBufferClear(builder->sql);
    ORM_StringBufferClear(builder->select_clause);
    ORM_StringBufferClear(builder->from_clause);
    ORM_StringBufferClear(builder->where_clause);
    ORM_StringBufferClear(builder->join_clause);
    ORM_StringBufferClear(builder->order_clause);
    ORM_StringBufferClear(builder->group_clause);
    
    builder->param_count = 0;
}

// ============================================================================
// Parameter Management
// ============================================================================

int ORM_AddParameter(ORM_SQLBuilder* builder, void* value, ORM_FieldType type, size_t size) {
    if (!builder) {
        return -1;
    }
    
    // Grow array if needed
    if (builder->param_count >= builder->param_capacity) {
        builder->param_capacity *= 2;
        builder->parameters = (ORM_Parameter*)realloc(builder->parameters, 
                                                       sizeof(ORM_Parameter) * builder->param_capacity);
    }
    
    builder->parameters[builder->param_count].value = value;
    builder->parameters[builder->param_count].type = type;
    builder->parameters[builder->param_count].size = size;
    
    return builder->param_count++;
}

const char* ORM_GetPlaceholder(ORM_SQLBuilder* builder, int param_index) {
    static char placeholder[32];
    
    switch (builder->dialect) {
        case ORM_DIALECT_POSTGRESQL:
            snprintf(placeholder, sizeof(placeholder), "$%d", param_index + 1);
            break;
        case ORM_DIALECT_MSSQL:
            snprintf(placeholder, sizeof(placeholder), "@p%d", param_index + 1);
            break;
        case ORM_DIALECT_SQLITE:
        case ORM_DIALECT_MYSQL:
        default:
            strcpy(placeholder, "?");
            break;
    }
    
    return placeholder;
}

ORM_Parameter* ORM_GetParameters(ORM_SQLBuilder* builder, int* count) {
    if (!builder || !count) {
        return NULL;
    }
    
    *count = builder->param_count;
    return builder->parameters;
}

// ============================================================================
// SELECT Query Builder
// ============================================================================

char* ORM_BuildSelectSQL(ORM_SQLBuilder* builder, ORM_ModelMeta* model, ORM_QuerySet* qs) {
    if (!builder || !model) {
        return NULL;
    }
    
    ORM_ResetSQLBuilder(builder);
    
    // SELECT clause
    ORM_StringBufferAppend(builder->sql, "SELECT ");
    
    if (qs && qs->distinct) {
        ORM_StringBufferAppend(builder->sql, "DISTINCT ");
    }
    
    // Field list
    for (int i = 0; i < model->field_count; i++) {
        if (i > 0) {
            ORM_StringBufferAppend(builder->sql, ", ");
        }
        
        char* quoted = ORM_QuoteIdentifier(model->fields[i].column_name, builder->dialect);
        ORM_StringBufferAppend(builder->sql, quoted);
        free(quoted);
    }
    
    // FROM clause
    ORM_StringBufferAppend(builder->sql, " FROM ");
    char* table = ORM_QuoteIdentifier(model->table_name, builder->dialect);
    ORM_StringBufferAppend(builder->sql, table);
    free(table);
    
    // WHERE clause
    if (qs && qs->filter_tree) {
        ORM_BuildWhereClause(builder, qs->filter_tree);
        if (builder->where_clause->length > 0) {
            ORM_StringBufferAppend(builder->sql, " WHERE ");
            ORM_StringBufferAppend(builder->sql, ORM_StringBufferGet(builder->where_clause));
        }
    }
    
    // ORDER BY clause
    if (qs && qs->order_by_count > 0) {
        ORM_BuildOrderClause(builder, qs->order_by_fields, qs->order_by_count);
        if (builder->order_clause->length > 0) {
            ORM_StringBufferAppend(builder->sql, " ");
            ORM_StringBufferAppend(builder->sql, ORM_StringBufferGet(builder->order_clause));
        }
    }
    
    // LIMIT/OFFSET clause
    if (qs && (qs->limit_value > 0 || qs->offset_value > 0)) {
        ORM_BuildLimitClause(builder, qs->limit_value, qs->offset_value);
    }
    
    return strdup(ORM_StringBufferGet(builder->sql));
}

// ============================================================================
// INSERT Query Builder
// ============================================================================

char* ORM_BuildInsertSQL(ORM_SQLBuilder* builder, ORM_ModelMeta* model, void* instance) {
    if (!builder || !model || !instance) {
        return NULL;
    }
    
    ORM_ResetSQLBuilder(builder);
    
    // INSERT INTO table_name
    ORM_StringBufferAppend(builder->sql, "INSERT INTO ");
    char* table = ORM_QuoteIdentifier(model->table_name, builder->dialect);
    ORM_StringBufferAppend(builder->sql, table);
    free(table);
    
    // (column1, column2, ...)
    ORM_StringBufferAppend(builder->sql, " (");
    int field_count = 0;
    
    for (int i = 0; i < model->field_count; i++) {
        // Skip auto-increment fields
        if (model->fields[i].flags & ORM_AUTO_INCREMENT) {
            continue;
        }
        
        if (field_count > 0) {
            ORM_StringBufferAppend(builder->sql, ", ");
        }
        
        char* quoted = ORM_QuoteIdentifier(model->fields[i].column_name, builder->dialect);
        ORM_StringBufferAppend(builder->sql, quoted);
        free(quoted);
        
        field_count++;
    }
    
    // VALUES (?, ?, ...)
    ORM_StringBufferAppend(builder->sql, ") VALUES (");
    
    field_count = 0;
    for (int i = 0; i < model->field_count; i++) {
        if (model->fields[i].flags & ORM_AUTO_INCREMENT) {
            continue;
        }
        
        if (field_count > 0) {
            ORM_StringBufferAppend(builder->sql, ", ");
        }
        
        // Add parameter
        void* field_ptr = (char*)instance + model->fields[i].offset;
        int param_idx = ORM_AddParameter(builder, field_ptr, model->fields[i].type, model->fields[i].size);
        ORM_StringBufferAppend(builder->sql, ORM_GetPlaceholder(builder, param_idx));
        
        field_count++;
    }
    
    ORM_StringBufferAppend(builder->sql, ")");
    
    return strdup(ORM_StringBufferGet(builder->sql));
}

// ============================================================================
// UPDATE Query Builder
// ============================================================================

char* ORM_BuildUpdateSQL(ORM_SQLBuilder* builder, ORM_ModelMeta* model, void* instance,
                         const char** fields, int field_count) {
    if (!builder || !model || !instance) {
        return NULL;
    }
    
    ORM_ResetSQLBuilder(builder);
    
    // UPDATE table_name
    ORM_StringBufferAppend(builder->sql, "UPDATE ");
    char* table = ORM_QuoteIdentifier(model->table_name, builder->dialect);
    ORM_StringBufferAppend(builder->sql, table);
    free(table);
    
    // SET column1 = ?, column2 = ?, ...
    ORM_StringBufferAppend(builder->sql, " SET ");
    
    int set_count = 0;
    for (int i = 0; i < model->field_count; i++) {
        // Skip primary key
        if (model->fields[i].flags & ORM_PRIMARY_KEY) {
            continue;
        }
        
        // If specific fields provided, check if this field is in the list
        if (fields && field_count > 0) {
            bool found = false;
            for (int j = 0; j < field_count; j++) {
                if (strcmp(model->fields[i].column_name, fields[j]) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) continue;
        }
        
        if (set_count > 0) {
            ORM_StringBufferAppend(builder->sql, ", ");
        }
        
        char* quoted = ORM_QuoteIdentifier(model->fields[i].column_name, builder->dialect);
        ORM_StringBufferAppend(builder->sql, quoted);
        ORM_StringBufferAppend(builder->sql, " = ");
        free(quoted);
        
        void* field_ptr = (char*)instance + model->fields[i].offset;
        int param_idx = ORM_AddParameter(builder, field_ptr, model->fields[i].type, model->fields[i].size);
        ORM_StringBufferAppend(builder->sql, ORM_GetPlaceholder(builder, param_idx));
        
        set_count++;
    }
    
    // WHERE primary_key = ?
    ORM_FieldMeta* pk = ORM_GetPrimaryKey(model);
    if (pk) {
        ORM_StringBufferAppend(builder->sql, " WHERE ");
        char* quoted = ORM_QuoteIdentifier(pk->column_name, builder->dialect);
        ORM_StringBufferAppend(builder->sql, quoted);
        ORM_StringBufferAppend(builder->sql, " = ");
        free(quoted);
        
        void* pk_ptr = (char*)instance + pk->offset;
        int param_idx = ORM_AddParameter(builder, pk_ptr, pk->type, pk->size);
        ORM_StringBufferAppend(builder->sql, ORM_GetPlaceholder(builder, param_idx));
    }
    
    return strdup(ORM_StringBufferGet(builder->sql));
}

// ============================================================================
// DELETE Query Builder
// ============================================================================

char* ORM_BuildDeleteSQL(ORM_SQLBuilder* builder, ORM_ModelMeta* model, ORM_QuerySet* qs) {
    if (!builder || !model) {
        return NULL;
    }
    
    ORM_ResetSQLBuilder(builder);
    
    // DELETE FROM table_name
    ORM_StringBufferAppend(builder->sql, "DELETE FROM ");
    char* table = ORM_QuoteIdentifier(model->table_name, builder->dialect);
    ORM_StringBufferAppend(builder->sql, table);
    free(table);
    
    // WHERE clause
    if (qs && qs->filter_tree) {
        ORM_BuildWhereClause(builder, qs->filter_tree);
        if (builder->where_clause->length > 0) {
            ORM_StringBufferAppend(builder->sql, " WHERE ");
            ORM_StringBufferAppend(builder->sql, ORM_StringBufferGet(builder->where_clause));
        }
    }
    
    return strdup(ORM_StringBufferGet(builder->sql));
}

// ============================================================================
// CREATE TABLE Query Builder
// ============================================================================

char* ORM_BuildCreateTableSQL(ORM_SQLBuilder* builder, ORM_ModelMeta* model) {
    if (!builder || !model) {
        return NULL;
    }
    
    ORM_ResetSQLBuilder(builder);
    
    // CREATE TABLE table_name (
    ORM_StringBufferAppend(builder->sql, "CREATE TABLE ");
    char* table = ORM_QuoteIdentifier(model->table_name, builder->dialect);
    ORM_StringBufferAppend(builder->sql, table);
    ORM_StringBufferAppend(builder->sql, " (");
    free(table);
    
    // Column definitions
    for (int i = 0; i < model->field_count; i++) {
        if (i > 0) {
            ORM_StringBufferAppend(builder->sql, ", ");
        }
        
        char* col_def = ORM_BuildColumnDefinition(&model->fields[i], builder->dialect);
        ORM_StringBufferAppend(builder->sql, col_def);
        free(col_def);
    }
    
    ORM_StringBufferAppend(builder->sql, ")");
    
    return strdup(ORM_StringBufferGet(builder->sql));
}

char* ORM_BuildDropTableSQL(ORM_SQLBuilder* builder, ORM_ModelMeta* model) {
    if (!builder || !model) {
        return NULL;
    }
    
    ORM_ResetSQLBuilder(builder);
    
    ORM_StringBufferAppend(builder->sql, "DROP TABLE IF EXISTS ");
    char* table = ORM_QuoteIdentifier(model->table_name, builder->dialect);
    ORM_StringBufferAppend(builder->sql, table);
    free(table);
    
    return strdup(ORM_StringBufferGet(builder->sql));
}
