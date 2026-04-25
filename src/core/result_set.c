/* Result Set Management Implementation
 * 
 * Core result set operations for efficient query result handling.
 */

#define _GNU_SOURCE
#include "result_set.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Result Set Creation and Cleanup
// ============================================================================

ORM_ResultSet* ORM_CreateResultSet(int row_count, int column_count) {
    ORM_ResultSet* rs = (ORM_ResultSet*)calloc(1, sizeof(ORM_ResultSet));
    if (!rs) {
        return NULL;
    }
    
    rs->row_count = row_count;
    rs->column_count = column_count;
    rs->current_row = -1;
    rs->owns_data = true;
    rs->model = NULL;
    
    // Allocate column metadata
    if (column_count > 0) {
        rs->columns = (ORM_ColumnMeta*)calloc(column_count, sizeof(ORM_ColumnMeta));
        if (!rs->columns) {
            free(rs);
            return NULL;
        }
    }
    
    // Allocate rows
    if (row_count > 0 && column_count > 0) {
        rs->rows = (void***)calloc(row_count, sizeof(void**));
        if (!rs->rows) {
            free(rs->columns);
            free(rs);
            return NULL;
        }
        
        for (int i = 0; i < row_count; i++) {
            rs->rows[i] = (void**)calloc(column_count, sizeof(void*));
            if (!rs->rows[i]) {
                // Cleanup on failure
                for (int j = 0; j < i; j++) {
                    free(rs->rows[j]);
                }
                free(rs->rows);
                free(rs->columns);
                free(rs);
                return NULL;
            }
        }
    }
    
    return rs;
}

ORM_ResultSet* ORM_CreateResultSetFromModel(ORM_ModelMeta* model, int row_count) {
    if (!model) {
        return NULL;
    }
    
    ORM_ResultSet* rs = ORM_CreateResultSet(row_count, model->field_count);
    if (!rs) {
        return NULL;
    }
    
    rs->model = model;
    
    // Initialize column metadata from model fields
    for (int i = 0; i < model->field_count; i++) {
        ORM_FieldMeta* field = &model->fields[i];
        rs->columns[i].name = strdup(field->column_name);
        rs->columns[i].type = field->type;
        rs->columns[i].index = i;
        rs->columns[i].size = field->size;
    }
    
    return rs;
}

void ORM_FreeResultSet(ORM_ResultSet* rs) {
    if (!rs) {
        return;
    }
    
    // Free column metadata
    if (rs->columns) {
        for (int i = 0; i < rs->column_count; i++) {
            if (rs->columns[i].name) {
                free(rs->columns[i].name);
            }
        }
        free(rs->columns);
    }
    
    // Free row data if we own it
    if (rs->owns_data && rs->rows) {
        for (int i = 0; i < rs->row_count; i++) {
            if (rs->rows[i]) {
                // Free individual cell data (strings, etc.)
                for (int j = 0; j < rs->column_count; j++) {
                    if (rs->rows[i][j]) {
                        // Free based on column type
                        if (rs->columns && (rs->columns[j].type == ORM_TYPE_STRING || 
                            rs->columns[j].type == ORM_TYPE_TEXT)) {
                            free(rs->rows[i][j]);
                        } else {
                            free(rs->rows[i][j]);
                        }
                    }
                }
                free(rs->rows[i]);
            }
        }
        free(rs->rows);
    }
    
    free(rs);
}

void ORM_SetColumnMeta(ORM_ResultSet* rs, int col_index, const char* name, 
                       ORM_FieldType type, size_t size) {
    if (!rs || col_index < 0 || col_index >= rs->column_count) {
        return;
    }
    
    // Free existing name if any
    if (rs->columns[col_index].name) {
        free(rs->columns[col_index].name);
    }
    
    rs->columns[col_index].name = name ? strdup(name) : NULL;
    rs->columns[col_index].type = type;
    rs->columns[col_index].index = col_index;
    rs->columns[col_index].size = size;
}

// ============================================================================
// Iteration
// ============================================================================

bool ORM_ResultNext(ORM_ResultSet* rs) {
    if (!rs || rs->current_row >= rs->row_count - 1) {
        return false;
    }
    
    rs->current_row++;
    return true;
}

void* ORM_ResultCurrent(ORM_ResultSet* rs, int column) {
    if (!rs || rs->current_row < 0 || rs->current_row >= rs->row_count) {
        return NULL;
    }
    
    if (column < 0 || column >= rs->column_count) {
        return NULL;
    }
    
    return rs->rows[rs->current_row][column];
}

void ORM_ResultReset(ORM_ResultSet* rs) {
    if (rs) {
        rs->current_row = -1;
    }
}

bool ORM_ResultHasNext(ORM_ResultSet* rs) {
    return rs && (rs->current_row < rs->row_count - 1);
}

void** ORM_GetRow(ORM_ResultSet* rs, int row_index) {
    if (!rs || row_index < 0 || row_index >= rs->row_count) {
        return NULL;
    }
    
    return rs->rows[row_index];
}

// ============================================================================
// Column Information
// ============================================================================

int ORM_GetColumnIndex(ORM_ResultSet* rs, const char* name) {
    if (!rs || !name || !rs->columns) {
        return -1;
    }
    
    for (int i = 0; i < rs->column_count; i++) {
        if (rs->columns[i].name && strcmp(rs->columns[i].name, name) == 0) {
            return i;
        }
    }
    
    return -1;
}

const char* ORM_GetColumnName(ORM_ResultSet* rs, int index) {
    if (!rs || !rs->columns || index < 0 || index >= rs->column_count) {
        return NULL;
    }
    
    return rs->columns[index].name;
}

ORM_FieldType ORM_GetColumnType(ORM_ResultSet* rs, int index) {
    if (!rs || !rs->columns || index < 0 || index >= rs->column_count) {
        return ORM_TYPE_INT; // Default fallback
    }
    
    return rs->columns[index].type;
}

int ORM_GetColumnCount(ORM_ResultSet* rs) {
    return rs ? rs->column_count : 0;
}

int ORM_GetRowCount(ORM_ResultSet* rs) {
    return rs ? rs->row_count : 0;
}

// ============================================================================
// Type-Safe Data Access
// ============================================================================

void* ORM_GetValue(ORM_ResultSet* rs, int row, const char* column) {
    int col_index = ORM_GetColumnIndex(rs, column);
    if (col_index < 0) {
        return NULL;
    }
    
    return ORM_GetValueByIndex(rs, row, col_index);
}

void* ORM_GetValueByIndex(ORM_ResultSet* rs, int row, int column) {
    if (!rs || row < 0 || row >= rs->row_count || 
        column < 0 || column >= rs->column_count) {
        return NULL;
    }
    
    return rs->rows[row][column];
}

int ORM_GetInt(ORM_ResultSet* rs, int row, const char* column) {
    void* value = ORM_GetValue(rs, row, column);
    return value ? *(int*)value : 0;
}

int64_t ORM_GetBigInt(ORM_ResultSet* rs, int row, const char* column) {
    void* value = ORM_GetValue(rs, row, column);
    return value ? *(int64_t*)value : 0;
}

float ORM_GetFloat(ORM_ResultSet* rs, int row, const char* column) {
    void* value = ORM_GetValue(rs, row, column);
    return value ? *(float*)value : 0.0f;
}

double ORM_GetDouble(ORM_ResultSet* rs, int row, const char* column) {
    void* value = ORM_GetValue(rs, row, column);
    return value ? *(double*)value : 0.0;
}

const char* ORM_GetString(ORM_ResultSet* rs, int row, const char* column) {
    void* value = ORM_GetValue(rs, row, column);
    return value ? (const char*)value : NULL;
}

bool ORM_GetBool(ORM_ResultSet* rs, int row, const char* column) {
    void* value = ORM_GetValue(rs, row, column);
    return value ? *(bool*)value : false;
}

time_t ORM_GetDateTime(ORM_ResultSet* rs, int row, const char* column) {
    void* value = ORM_GetValue(rs, row, column);
    return value ? *(time_t*)value : 0;
}

int ORM_GetIntByIndex(ORM_ResultSet* rs, int row, int column) {
    void* value = ORM_GetValueByIndex(rs, row, column);
    return value ? *(int*)value : 0;
}

int64_t ORM_GetBigIntByIndex(ORM_ResultSet* rs, int row, int column) {
    void* value = ORM_GetValueByIndex(rs, row, column);
    return value ? *(int64_t*)value : 0;
}

double ORM_GetDoubleByIndex(ORM_ResultSet* rs, int row, int column) {
    void* value = ORM_GetValueByIndex(rs, row, column);
    return value ? *(double*)value : 0.0;
}

const char* ORM_GetStringByIndex(ORM_ResultSet* rs, int row, int column) {
    void* value = ORM_GetValueByIndex(rs, row, column);
    return value ? (const char*)value : NULL;
}

bool ORM_IsNull(ORM_ResultSet* rs, int row, const char* column) {
    return ORM_GetValue(rs, row, column) == NULL;
}

bool ORM_IsNullByIndex(ORM_ResultSet* rs, int row, int column) {
    return ORM_GetValueByIndex(rs, row, column) == NULL;
}

// ============================================================================
// Iterator Pattern
// ============================================================================

ORM_ResultIterator* ORM_CreateIterator(ORM_ResultSet* rs) {
    if (!rs) {
        return NULL;
    }
    
    ORM_ResultIterator* iter = (ORM_ResultIterator*)calloc(1, sizeof(ORM_ResultIterator));
    if (!iter) {
        return NULL;
    }
    
    iter->result_set = rs;
    iter->current_index = -1;
    iter->current_row = NULL;
    
    return iter;
}

void ORM_FreeIterator(ORM_ResultIterator* iter) {
    free(iter);
}

bool ORM_IteratorHasNext(ORM_ResultIterator* iter) {
    if (!iter || !iter->result_set) {
        return false;
    }
    
    return iter->current_index < iter->result_set->row_count - 1;
}

void** ORM_IteratorNext(ORM_ResultIterator* iter) {
    if (!iter || !ORM_IteratorHasNext(iter)) {
        return NULL;
    }
    
    iter->current_index++;
    iter->current_row = iter->result_set->rows[iter->current_index];
    
    return iter->current_row;
}

void ORM_IteratorReset(ORM_ResultIterator* iter) {
    if (iter) {
        iter->current_index = -1;
        iter->current_row = NULL;
    }
}

// ============================================================================
// Utility Functions
// ============================================================================

void ORM_PrintResultSet(ORM_ResultSet* rs) {
    if (!rs) {
        printf("NULL result set\n");
        return;
    }
    
    printf("Result Set: %d rows x %d columns\n", rs->row_count, rs->column_count);
    
    // Print column headers
    for (int i = 0; i < rs->column_count; i++) {
        printf("%-15s ", rs->columns[i].name ? rs->columns[i].name : "");
    }
    printf("\n");
    
    // Print separator
    for (int i = 0; i < rs->column_count; i++) {
        printf("%-15s ", "---------------");
    }
    printf("\n");
    
    // Print rows
    for (int i = 0; i < rs->row_count; i++) {
        for (int j = 0; j < rs->column_count; j++) {
            void* value = rs->rows[i][j];
            if (!value) {
                printf("%-15s ", "NULL");
            } else {
                switch (rs->columns[j].type) {
                    case ORM_TYPE_INT:
                        printf("%-15d ", *(int*)value);
                        break;
                    case ORM_TYPE_BIGINT:
                        printf("%-15ld ", *(int64_t*)value);
                        break;
                    case ORM_TYPE_FLOAT:
                        printf("%-15.2f ", *(float*)value);
                        break;
                    case ORM_TYPE_DOUBLE:
                        printf("%-15.2f ", *(double*)value);
                        break;
                    case ORM_TYPE_STRING:
                    case ORM_TYPE_TEXT:
                        printf("%-15s ", (char*)value);
                        break;
                    case ORM_TYPE_BOOLEAN:
                        printf("%-15s ", *(bool*)value ? "true" : "false");
                        break;
                    default:
                        printf("%-15s ", "?");
                        break;
                }
            }
        }
        printf("\n");
    }
}

ORM_ResultSet* ORM_CloneResultSet(ORM_ResultSet* rs) {
    if (!rs) {
        return NULL;
    }
    
    ORM_ResultSet* clone = ORM_CreateResultSet(rs->row_count, rs->column_count);
    if (!clone) {
        return NULL;
    }
    
    clone->model = rs->model;
    
    // Clone column metadata
    for (int i = 0; i < rs->column_count; i++) {
        clone->columns[i].name = rs->columns[i].name ? strdup(rs->columns[i].name) : NULL;
        clone->columns[i].type = rs->columns[i].type;
        clone->columns[i].index = rs->columns[i].index;
        clone->columns[i].size = rs->columns[i].size;
    }
    
    // Clone data
    for (int i = 0; i < rs->row_count; i++) {
        for (int j = 0; j < rs->column_count; j++) {
            if (rs->rows[i][j]) {
                // Allocate and copy based on type
                size_t size = rs->columns[j].size;
                clone->rows[i][j] = malloc(size);
                memcpy(clone->rows[i][j], rs->rows[i][j], size);
            }
        }
    }
    
    return clone;
}

ORM_ResultSet* ORM_SliceResultSet(ORM_ResultSet* rs, int start, int end) {
    if (!rs || start < 0 || end > rs->row_count || start >= end) {
        return NULL;
    }
    
    int slice_count = end - start;
    ORM_ResultSet* slice = ORM_CreateResultSet(slice_count, rs->column_count);
    if (!slice) {
        return NULL;
    }
    
    slice->model = rs->model;
    
    // Copy column metadata
    for (int i = 0; i < rs->column_count; i++) {
        slice->columns[i].name = rs->columns[i].name ? strdup(rs->columns[i].name) : NULL;
        slice->columns[i].type = rs->columns[i].type;
        slice->columns[i].index = rs->columns[i].index;
        slice->columns[i].size = rs->columns[i].size;
    }
    
    // Copy sliced rows
    for (int i = 0; i < slice_count; i++) {
        int source_row = start + i;
        for (int j = 0; j < rs->column_count; j++) {
            if (rs->rows[source_row][j]) {
                size_t size = rs->columns[j].size;
                slice->rows[i][j] = malloc(size);
                memcpy(slice->rows[i][j], rs->rows[source_row][j], size);
            }
        }
    }
    
    return slice;
}
