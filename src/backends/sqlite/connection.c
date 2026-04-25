/* SQLite Connection Management
 * 
 * Handles database connections, connection pooling, and resource management.
 */

#define _GNU_SOURCE
#include "sqlite_backend.h"
#include "../../utils/memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Connection Management
// ============================================================================

ORM_SQLiteConnection* ORM_SQLiteConnect(const char* database) {
    if (!database) {
        return NULL;
    }
    
    ORM_SQLiteConnection* conn = (ORM_SQLiteConnection*)calloc(1, sizeof(ORM_SQLiteConnection));
    if (!conn) {
        return NULL;
    }
    
    // Open SQLite database
    int rc = sqlite3_open(database, &conn->db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to open database: %s\n", sqlite3_errmsg(conn->db));
        sqlite3_close(conn->db);
        free(conn);
        return NULL;
    }
    
    conn->database_path = strdup(database);
    conn->is_open = true;
    conn->in_transaction = false;
    conn->transaction_level = 0;
    
    // Initialize mutex
    pthread_mutex_init(&conn->lock, NULL);
    
    // Enable foreign keys by default
    sqlite3_exec(conn->db, "PRAGMA foreign_keys = ON", NULL, NULL, NULL);
    
    return conn;
}

void ORM_SQLiteDisconnect(ORM_SQLiteConnection* conn) {
    if (!conn) {
        return;
    }
    
    pthread_mutex_lock(&conn->lock);
    
    if (conn->is_open && conn->db) {
        sqlite3_close(conn->db);
        conn->is_open = false;
    }
    
    if (conn->database_path) {
        free(conn->database_path);
    }
    
    pthread_mutex_unlock(&conn->lock);
    pthread_mutex_destroy(&conn->lock);
    
    free(conn);
}

bool ORM_SQLiteIsConnected(ORM_SQLiteConnection* conn) {
    if (!conn) {
        return false;
    }
    return conn->is_open && conn->db != NULL;
}

const char* ORM_SQLiteGetError(ORM_SQLiteConnection* conn) {
    if (!conn || !conn->db) {
        return "No connection";
    }
    return sqlite3_errmsg(conn->db);
}

// ============================================================================
// Query Execution Helpers
// ============================================================================

int64_t ORM_SQLiteLastInsertId(ORM_SQLiteConnection* conn) {
    if (!conn || !conn->db) {
        return -1;
    }
    return sqlite3_last_insert_rowid(conn->db);
}

int ORM_SQLiteAffectedRows(ORM_SQLiteConnection* conn) {
    if (!conn || !conn->db) {
        return -1;
    }
    return sqlite3_changes(conn->db);
}

// ============================================================================
// Result Set Management
// ============================================================================

void ORM_FreeSQLiteResultSet(ORM_SQLiteResultSet* rs) {
    if (!rs) {
        return;
    }
    
    // Free column names
    if (rs->column_names) {
        for (int i = 0; i < rs->column_count; i++) {
            free(rs->column_names[i]);
        }
        free(rs->column_names);
    }
    
    // Free column types
    if (rs->column_types) {
        free(rs->column_types);
    }
    
    // Free rows
    if (rs->rows) {
        for (int i = 0; i < rs->row_count; i++) {
            if (rs->rows[i]) {
                for (int j = 0; j < rs->column_count; j++) {
                    if (rs->rows[i][j]) {
                        // Free string values
                        if (rs->column_types[j] == ORM_TYPE_STRING ||
                            rs->column_types[j] == ORM_TYPE_TEXT) {
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

// ============================================================================
// Utility Functions
// ============================================================================

int ORM_SQLiteVacuum(ORM_SQLiteConnection* conn) {
    if (!conn || !conn->db) {
        return -1;
    }
    
    char* errmsg = NULL;
    int rc = sqlite3_exec(conn->db, "VACUUM", NULL, NULL, &errmsg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "VACUUM failed: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    
    return 0;
}

int ORM_SQLiteAnalyze(ORM_SQLiteConnection* conn) {
    if (!conn || !conn->db) {
        return -1;
    }
    
    char* errmsg = NULL;
    int rc = sqlite3_exec(conn->db, "ANALYZE", NULL, NULL, &errmsg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "ANALYZE failed: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    
    return 0;
}

int64_t ORM_SQLiteGetDatabaseSize(ORM_SQLiteConnection* conn) {
    if (!conn || !conn->db) {
        return -1;
    }
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(conn->db, "PRAGMA page_count", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }
    
    int64_t page_count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        page_count = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    rc = sqlite3_prepare_v2(conn->db, "PRAGMA page_size", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }
    
    int64_t page_size = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        page_size = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    return page_count * page_size;
}

int ORM_SQLiteSetForeignKeys(ORM_SQLiteConnection* conn, bool enabled) {
    if (!conn || !conn->db) {
        return -1;
    }
    
    const char* sql = enabled ? "PRAGMA foreign_keys = ON" : "PRAGMA foreign_keys = OFF";
    char* errmsg = NULL;
    int rc = sqlite3_exec(conn->db, sql, NULL, NULL, &errmsg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Foreign keys pragma failed: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    
    return 0;
}
