/* SQLite Query Execution
 * 
 * Handles query execution, prepared statements, and parameter binding.
 */

#define _GNU_SOURCE
#include "sqlite_backend.h"
#include "../../utils/memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Parameter Binding
// ============================================================================

static int bind_parameter(sqlite3_stmt* stmt, int index, ORM_Parameter* param) {
    int rc;
    
    switch (param->type) {
        case ORM_TYPE_INT:
            rc = sqlite3_bind_int(stmt, index, *(int*)param->value);
            break;
            
        case ORM_TYPE_BIGINT:
            rc = sqlite3_bind_int64(stmt, index, *(int64_t*)param->value);
            break;
            
        case ORM_TYPE_FLOAT:
            rc = sqlite3_bind_double(stmt, index, (double)(*(float*)param->value));
            break;
            
        case ORM_TYPE_DOUBLE:
            rc = sqlite3_bind_double(stmt, index, *(double*)param->value);
            break;
            
        case ORM_TYPE_STRING:
        case ORM_TYPE_TEXT:
            if (param->value) {
                rc = sqlite3_bind_text(stmt, index, (const char*)param->value, 
                                      param->size > 0 ? param->size : -1, SQLITE_TRANSIENT);
            } else {
                rc = sqlite3_bind_null(stmt, index);
            }
            break;
            
        case ORM_TYPE_BOOLEAN:
            rc = sqlite3_bind_int(stmt, index, *(bool*)param->value ? 1 : 0);
            break;
            
        case ORM_TYPE_BLOB:
            if (param->value && param->size > 0) {
                rc = sqlite3_bind_blob(stmt, index, param->value, param->size, SQLITE_TRANSIENT);
            } else {
                rc = sqlite3_bind_null(stmt, index);
            }
            break;
            
        default:
            rc = sqlite3_bind_null(stmt, index);
            break;
    }
    
    return rc;
}

int ORM_SQLiteBindParameters(ORM_PreparedStatement* prep_stmt, ORM_Parameter* params, int param_count) {
    if (!prep_stmt || !prep_stmt->stmt) {
        return -1;
    }
    
    for (int i = 0; i < param_count; i++) {
        int rc = bind_parameter(prep_stmt->stmt, i + 1, &params[i]);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to bind parameter %d\n", i);
            return -1;
        }
    }
    
    return 0;
}

// ============================================================================
// Result Set Creation
// ============================================================================

static ORM_FieldType sqlite_type_to_orm_type(int sqlite_type) {
    switch (sqlite_type) {
        case SQLITE_INTEGER:
            return ORM_TYPE_INT;
        case SQLITE_FLOAT:
            return ORM_TYPE_DOUBLE;
        case SQLITE_TEXT:
            return ORM_TYPE_STRING;
        case SQLITE_BLOB:
            return ORM_TYPE_BLOB;
        case SQLITE_NULL:
        default:
            return ORM_TYPE_STRING;
    }
}

static ORM_SQLiteResultSet* create_result_set(sqlite3_stmt* stmt) {
    ORM_SQLiteResultSet* rs = (ORM_SQLiteResultSet*)calloc(1, sizeof(ORM_SQLiteResultSet));
    if (!rs) {
        return NULL;
    }
    
    // Get column count
    rs->column_count = sqlite3_column_count(stmt);
    
    // For non-SELECT statements (INSERT/UPDATE/DELETE), execute and return empty result
    if (rs->column_count == 0) {
        sqlite3_step(stmt);  // Execute the statement
        return rs;
    }
    
    // Allocate column names and types
    rs->column_names = (char**)calloc(rs->column_count, sizeof(char*));
    rs->column_types = (ORM_FieldType*)calloc(rs->column_count, sizeof(ORM_FieldType));
    
    // Get column names
    for (int i = 0; i < rs->column_count; i++) {
        const char* name = sqlite3_column_name(stmt, i);
        rs->column_names[i] = strdup(name ? name : "");
    }
    
    // Count rows first
    int row_capacity = 10;
    rs->rows = (void***)calloc(row_capacity, sizeof(void**));
    rs->row_count = 0;
    
    // Fetch all rows
    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        // Resize if needed
        if (rs->row_count >= row_capacity) {
            row_capacity *= 2;
            rs->rows = (void***)realloc(rs->rows, row_capacity * sizeof(void**));
        }
        
        // Allocate row
        rs->rows[rs->row_count] = (void**)calloc(rs->column_count, sizeof(void*));
        
        // Get column values
        for (int i = 0; i < rs->column_count; i++) {
            int col_type = sqlite3_column_type(stmt, i);
            
            // Set type on first row
            if (rs->row_count == 0) {
                rs->column_types[i] = sqlite_type_to_orm_type(col_type);
            }
            
            switch (col_type) {
                case SQLITE_INTEGER: {
                    int64_t* val = (int64_t*)malloc(sizeof(int64_t));
                    *val = sqlite3_column_int64(stmt, i);
                    rs->rows[rs->row_count][i] = val;
                    break;
                }
                
                case SQLITE_FLOAT: {
                    double* val = (double*)malloc(sizeof(double));
                    *val = sqlite3_column_double(stmt, i);
                    rs->rows[rs->row_count][i] = val;
                    break;
                }
                
                case SQLITE_TEXT: {
                    const unsigned char* text = sqlite3_column_text(stmt, i);
                    rs->rows[rs->row_count][i] = strdup((const char*)text);
                    break;
                }
                
                case SQLITE_BLOB: {
                    int blob_size = sqlite3_column_bytes(stmt, i);
                    const void* blob_data = sqlite3_column_blob(stmt, i);
                    void* blob_copy = malloc(blob_size);
                    memcpy(blob_copy, blob_data, blob_size);
                    rs->rows[rs->row_count][i] = blob_copy;
                    break;
                }
                
                case SQLITE_NULL:
                default:
                    rs->rows[rs->row_count][i] = NULL;
                    break;
            }
        }
        
        rs->row_count++;
    }
    
    rs->current_row = 0;
    return rs;
}

// ============================================================================
// Query Execution
// ============================================================================

// ============================================================================
// Query Execution
// ============================================================================

ORM_SQLiteResultSet* ORM_SQLiteExecute(
    ORM_SQLiteConnection* conn,
    const char* sql,
    ORM_Parameter* params,
    int param_count
) {
    if (!conn || !conn->db || !sql) {
        return NULL;
    }
    
    pthread_mutex_lock(&conn->lock);
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(conn->db, sql, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&conn->lock);
        return NULL;
    }
    
    // Bind parameters
    if (param_count > 0) {
        for (int i = 0; i < param_count; i++) {
            if (bind_parameter(stmt, i + 1, &params[i]) != 0) {
                sqlite3_finalize(stmt);
                pthread_mutex_unlock(&conn->lock);
                return NULL;
            }
        }
    }
    
    // Create result set
    ORM_SQLiteResultSet* result = create_result_set(stmt);
    
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&conn->lock);
    
    return result;
}

int ORM_SQLiteExecuteNonQuery(
    ORM_SQLiteConnection* conn,
    const char* sql,
    ORM_Parameter* params,
    int param_count
) {
    if (!conn || !conn->db || !sql) {
        return -1;
    }
    
    pthread_mutex_lock(&conn->lock);
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(conn->db, sql, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(conn->db));
        pthread_mutex_unlock(&conn->lock);
        return -1;
    }
    
    // Bind parameters
    if (params && param_count > 0) {
        for (int i = 0; i < param_count; i++) {
            rc = bind_parameter(stmt, i + 1, &params[i]);
            if (rc != SQLITE_OK) {
                fprintf(stderr, "Failed to bind parameter %d: %s\n", i, sqlite3_errmsg(conn->db));
                sqlite3_finalize(stmt);
                pthread_mutex_unlock(&conn->lock);
                return -1;
            }
        }
    }
    
    // Execute
    rc = sqlite3_step(stmt);
    
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&conn->lock);
    
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(conn->db));
        return -1;
    }
    
    return 0;
}

// ============================================================================
// Prepared Statements
// ============================================================================

ORM_PreparedStatement* ORM_SQLitePrepare(ORM_SQLiteConnection* conn, const char* sql) {
    if (!conn || !conn->db || !sql) {
        return NULL;
    }
    
    ORM_PreparedStatement* prep = (ORM_PreparedStatement*)calloc(1, sizeof(ORM_PreparedStatement));
    if (!prep) {
        return NULL;
    }
    
    int rc = sqlite3_prepare_v2(conn->db, sql, -1, &prep->stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(conn->db));
        free(prep);
        return NULL;
    }
    
    prep->sql = strdup(sql);
    prep->use_count = 0;
    prep->last_used = time(NULL);
    
    return prep;
}

ORM_SQLiteResultSet* ORM_SQLiteExecutePrepared(ORM_PreparedStatement* stmt) {
    if (!stmt || !stmt->stmt) {
        return NULL;
    }
    
    ORM_SQLiteResultSet* result = create_result_set(stmt->stmt);
    stmt->use_count++;
    stmt->last_used = time(NULL);
    
    return result;
}

void ORM_SQLiteResetStatement(ORM_PreparedStatement* stmt) {
    if (stmt && stmt->stmt) {
        sqlite3_reset(stmt->stmt);
        sqlite3_clear_bindings(stmt->stmt);
    }
}

void ORM_SQLiteFreePrepared(ORM_PreparedStatement* stmt) {
    if (!stmt) {
        return;
    }
    
    if (stmt->stmt) {
        sqlite3_finalize(stmt->stmt);
    }
    
    if (stmt->sql) {
        free(stmt->sql);
    }
    
    free(stmt);
}
