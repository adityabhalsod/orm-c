/* Result Set Transformations
 * 
 * Map, filter, and export operations for result sets.
 */

#define _GNU_SOURCE
#include "result_set.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Map and Filter
// ============================================================================

ORM_ResultSet* ORM_MapResultSet(ORM_ResultSet* rs, ORM_MapFunction fn, void* context) {
    if (!rs || !fn) {
        return NULL;
    }
    
    ORM_ResultSet* mapped = ORM_CreateResultSet(rs->row_count, rs->column_count);
    if (!mapped) {
        return NULL;
    }
    
    mapped->model = rs->model;
    
    // Copy column metadata
    for (int i = 0; i < rs->column_count; i++) {
        mapped->columns[i].name = rs->columns[i].name ? strdup(rs->columns[i].name) : NULL;
        mapped->columns[i].type = rs->columns[i].type;
        mapped->columns[i].index = rs->columns[i].index;
        mapped->columns[i].size = rs->columns[i].size;
    }
    
    // Apply map function to each row
    for (int i = 0; i < rs->row_count; i++) {
        void* new_row = fn(rs->rows[i], rs->column_count, context);
        if (new_row) {
            // Assuming map function returns void** with same column count
            mapped->rows[i] = (void**)new_row;
        }
    }
    
    return mapped;
}

ORM_ResultSet* ORM_FilterResultSet(ORM_ResultSet* rs, ORM_FilterFunction fn, void* context) {
    if (!rs || !fn) {
        return NULL;
    }
    
    // First pass: count matching rows
    int match_count = 0;
    for (int i = 0; i < rs->row_count; i++) {
        if (fn(rs->rows[i], rs->column_count, context)) {
            match_count++;
        }
    }
    
    // Create filtered result set
    ORM_ResultSet* filtered = ORM_CreateResultSet(match_count, rs->column_count);
    if (!filtered) {
        return NULL;
    }
    
    filtered->model = rs->model;
    
    // Copy column metadata
    for (int i = 0; i < rs->column_count; i++) {
        filtered->columns[i].name = rs->columns[i].name ? strdup(rs->columns[i].name) : NULL;
        filtered->columns[i].type = rs->columns[i].type;
        filtered->columns[i].index = rs->columns[i].index;
        filtered->columns[i].size = rs->columns[i].size;
    }
    
    // Second pass: copy matching rows
    int dest_idx = 0;
    for (int i = 0; i < rs->row_count; i++) {
        if (fn(rs->rows[i], rs->column_count, context)) {
            for (int j = 0; j < rs->column_count; j++) {
                if (rs->rows[i][j]) {
                    size_t size = rs->columns[j].size;
                    filtered->rows[dest_idx][j] = malloc(size);
                    memcpy(filtered->rows[dest_idx][j], rs->rows[i][j], size);
                }
            }
            dest_idx++;
        }
    }
    
    return filtered;
}

// ============================================================================
// Export Functions
// ============================================================================

char* ORM_ResultSetToJSON(ORM_ResultSet* rs) {
    if (!rs) {
        return NULL;
    }
    
    // Estimate size (rough calculation)
    size_t buffer_size = 1024 + (rs->row_count * rs->column_count * 128);
    char* json = (char*)malloc(buffer_size);
    if (!json) {
        return NULL;
    }
    
    size_t offset = 0;
    
    // Start JSON array
    offset += snprintf(json + offset, buffer_size - offset, "[\n");
    
    // Iterate rows
    for (int i = 0; i < rs->row_count; i++) {
        offset += snprintf(json + offset, buffer_size - offset, "  {");
        
        // Iterate columns
        for (int j = 0; j < rs->column_count; j++) {
            const char* col_name = rs->columns[j].name ? rs->columns[j].name : "unknown";
            void* value = rs->rows[i][j];
            
            if (j > 0) {
                offset += snprintf(json + offset, buffer_size - offset, ", ");
            }
            
            offset += snprintf(json + offset, buffer_size - offset, "\"%s\": ", col_name);
            
            if (!value) {
                offset += snprintf(json + offset, buffer_size - offset, "null");
            } else {
                switch (rs->columns[j].type) {
                    case ORM_TYPE_INT:
                        offset += snprintf(json + offset, buffer_size - offset, 
                                         "%d", *(int*)value);
                        break;
                    case ORM_TYPE_BIGINT:
                        offset += snprintf(json + offset, buffer_size - offset, 
                                         "%ld", *(int64_t*)value);
                        break;
                    case ORM_TYPE_FLOAT:
                        offset += snprintf(json + offset, buffer_size - offset, 
                                         "%.2f", *(float*)value);
                        break;
                    case ORM_TYPE_DOUBLE:
                        offset += snprintf(json + offset, buffer_size - offset, 
                                         "%.2f", *(double*)value);
                        break;
                    case ORM_TYPE_STRING:
                    case ORM_TYPE_TEXT:
                        offset += snprintf(json + offset, buffer_size - offset, 
                                         "\"%s\"", (char*)value);
                        break;
                    case ORM_TYPE_BOOLEAN:
                        offset += snprintf(json + offset, buffer_size - offset, 
                                         "%s", *(bool*)value ? "true" : "false");
                        break;
                    default:
                        offset += snprintf(json + offset, buffer_size - offset, "null");
                        break;
                }
            }
        }
        
        offset += snprintf(json + offset, buffer_size - offset, "}");
        
        if (i < rs->row_count - 1) {
            offset += snprintf(json + offset, buffer_size - offset, ",\n");
        } else {
            offset += snprintf(json + offset, buffer_size - offset, "\n");
        }
    }
    
    offset += snprintf(json + offset, buffer_size - offset, "]\n");
    
    return json;
}

char* ORM_ResultSetToCSV(ORM_ResultSet* rs) {
    if (!rs) {
        return NULL;
    }
    
    // Estimate size
    size_t buffer_size = 1024 + (rs->row_count * rs->column_count * 64);
    char* csv = (char*)malloc(buffer_size);
    if (!csv) {
        return NULL;
    }
    
    size_t offset = 0;
    
    // Write header row
    for (int i = 0; i < rs->column_count; i++) {
        const char* name = rs->columns[i].name ? rs->columns[i].name : "";
        offset += snprintf(csv + offset, buffer_size - offset, "%s%s", 
                          name, i < rs->column_count - 1 ? "," : "\n");
    }
    
    // Write data rows
    for (int i = 0; i < rs->row_count; i++) {
        for (int j = 0; j < rs->column_count; j++) {
            void* value = rs->rows[i][j];
            
            if (!value) {
                offset += snprintf(csv + offset, buffer_size - offset, "");
            } else {
                switch (rs->columns[j].type) {
                    case ORM_TYPE_INT:
                        offset += snprintf(csv + offset, buffer_size - offset, 
                                         "%d", *(int*)value);
                        break;
                    case ORM_TYPE_BIGINT:
                        offset += snprintf(csv + offset, buffer_size - offset, 
                                         "%ld", *(int64_t*)value);
                        break;
                    case ORM_TYPE_FLOAT:
                        offset += snprintf(csv + offset, buffer_size - offset, 
                                         "%.2f", *(float*)value);
                        break;
                    case ORM_TYPE_DOUBLE:
                        offset += snprintf(csv + offset, buffer_size - offset, 
                                         "%.2f", *(double*)value);
                        break;
                    case ORM_TYPE_STRING:
                    case ORM_TYPE_TEXT:
                        // Quote strings that contain commas
                        if (strchr((char*)value, ',')) {
                            offset += snprintf(csv + offset, buffer_size - offset, 
                                             "\"%s\"", (char*)value);
                        } else {
                            offset += snprintf(csv + offset, buffer_size - offset, 
                                             "%s", (char*)value);
                        }
                        break;
                    case ORM_TYPE_BOOLEAN:
                        offset += snprintf(csv + offset, buffer_size - offset, 
                                         "%s", *(bool*)value ? "true" : "false");
                        break;
                    default:
                        break;
                }
            }
            
            if (j < rs->column_count - 1) {
                offset += snprintf(csv + offset, buffer_size - offset, ",");
            }
        }
        offset += snprintf(csv + offset, buffer_size - offset, "\n");
    }
    
    return csv;
}

void*** ORM_ResultSetToArray(ORM_ResultSet* rs) {
    if (!rs) {
        return NULL;
    }
    
    // Simply return the internal rows array (caller must not free it)
    return rs->rows;
}
