/* SQLite Schema Operations
 * 
 * Handles table creation, modifications, and introspection.
 */

#define _GNU_SOURCE
#include "sqlite_backend.h"
#include "../../core/sql_builder.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Table Operations
// ============================================================================

int ORM_SQLiteCreateTable(ORM_SQLiteConnection* conn, ORM_ModelMeta* model) {
    if (!conn || !conn->db || !model) {
        return -1;
    }
    
    // Use SQL builder to generate CREATE TABLE statement
    ORM_SQLBuilder* builder = ORM_CreateSQLBuilder(ORM_DIALECT_SQLITE);
    if (!builder) {
        return -1;
    }
    
    const char* sql = ORM_BuildCreateTableSQL(builder, model);
    if (!sql) {
        ORM_FreeSQLBuilder(builder);
        return -1;
    }
    
    // Execute CREATE TABLE
    char* errmsg = NULL;
    int rc = sqlite3_exec(conn->db, sql, NULL, NULL, &errmsg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "CREATE TABLE failed: %s\n", errmsg);
        sqlite3_free(errmsg);
        ORM_FreeSQLBuilder(builder);
        return -1;
    }
    
    ORM_FreeSQLBuilder(builder);
    return 0;
}

int ORM_SQLiteDropTable(ORM_SQLiteConnection* conn, const char* table_name) {
    if (!conn || !conn->db || !table_name) {
        return -1;
    }
    
    char sql[256];
    snprintf(sql, sizeof(sql), "DROP TABLE IF EXISTS \"%s\"", table_name);
    
    char* errmsg = NULL;
    int rc = sqlite3_exec(conn->db, sql, NULL, NULL, &errmsg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "DROP TABLE failed: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    
    return 0;
}

bool ORM_SQLiteTableExists(ORM_SQLiteConnection* conn, const char* table_name) {
    if (!conn || !conn->db || !table_name) {
        return false;
    }
    
    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(conn->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, table_name, -1, SQLITE_STATIC);
    
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    
    sqlite3_finalize(stmt);
    return exists;
}

// ============================================================================
// Column Introspection
// ============================================================================

ORM_ColumnInfo* ORM_SQLiteGetColumns(
    ORM_SQLiteConnection* conn,
    const char* table_name,
    int* count
) {
    if (!conn || !conn->db || !table_name || !count) {
        return NULL;
    }
    
    *count = 0;
    
    // Query table_info pragma
    char sql[256];
    snprintf(sql, sizeof(sql), "PRAGMA table_info(\"%s\")", table_name);
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(conn->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to get table info: %s\n", sqlite3_errmsg(conn->db));
        return NULL;
    }
    
    // Count columns first
    int col_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        col_count++;
    }
    
    if (col_count == 0) {
        sqlite3_finalize(stmt);
        return NULL;
    }
    
    // Allocate column info array
    ORM_ColumnInfo* columns = (ORM_ColumnInfo*)calloc(col_count, sizeof(ORM_ColumnInfo));
    if (!columns) {
        sqlite3_finalize(stmt);
        return NULL;
    }
    
    // Reset and fetch column info
    sqlite3_reset(stmt);
    
    int idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < col_count) {
        // cid, name, type, notnull, dflt_value, pk
        const unsigned char* name = sqlite3_column_text(stmt, 1);
        const unsigned char* type = sqlite3_column_text(stmt, 2);
        int notnull = sqlite3_column_int(stmt, 3);
        const unsigned char* dflt_value = sqlite3_column_text(stmt, 4);
        int pk = sqlite3_column_int(stmt, 5);
        
        columns[idx].name = strdup((const char*)name);
        columns[idx].type = strdup((const char*)type);
        columns[idx].not_null = (notnull != 0);
        columns[idx].default_value = dflt_value ? strdup((const char*)dflt_value) : NULL;
        columns[idx].primary_key = (pk != 0);
        
        idx++;
    }
    
    sqlite3_finalize(stmt);
    
    *count = col_count;
    return columns;
}

void ORM_FreeColumnInfo(ORM_ColumnInfo* columns, int count) {
    if (!columns) {
        return;
    }
    
    for (int i = 0; i < count; i++) {
        if (columns[i].name) {
            free(columns[i].name);
        }
        if (columns[i].type) {
            free(columns[i].type);
        }
        if (columns[i].default_value) {
            free(columns[i].default_value);
        }
    }
    
    free(columns);
}

// ============================================================================
// Index Operations
// ============================================================================

int ORM_SQLiteCreateIndex(
    ORM_SQLiteConnection* conn,
    const char* table_name,
    const char* column_name,
    bool unique
) {
    if (!conn || !conn->db || !table_name || !column_name) {
        return -1;
    }
    
    char sql[512];
    char index_name[256];
    
    // Generate index name
    snprintf(index_name, sizeof(index_name), "idx_%s_%s", table_name, column_name);
    
    // Generate CREATE INDEX statement
    if (unique) {
        snprintf(sql, sizeof(sql), "CREATE UNIQUE INDEX \"%s\" ON \"%s\" (\"%s\")",
                index_name, table_name, column_name);
    } else {
        snprintf(sql, sizeof(sql), "CREATE INDEX \"%s\" ON \"%s\" (\"%s\")",
                index_name, table_name, column_name);
    }
    
    char* errmsg = NULL;
    int rc = sqlite3_exec(conn->db, sql, NULL, NULL, &errmsg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "CREATE INDEX failed: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    
    return 0;
}

int ORM_SQLiteDropIndex(ORM_SQLiteConnection* conn, const char* index_name) {
    if (!conn || !conn->db || !index_name) {
        return -1;
    }
    
    char sql[256];
    snprintf(sql, sizeof(sql), "DROP INDEX IF EXISTS \"%s\"", index_name);
    
    char* errmsg = NULL;
    int rc = sqlite3_exec(conn->db, sql, NULL, NULL, &errmsg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "DROP INDEX failed: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    
    return 0;
}
