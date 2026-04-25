/* SQLite Transaction Support
 * 
 * Handles transactions, savepoints, and nested transactions.
 */

#define _GNU_SOURCE
#include "sqlite_backend.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Transaction Management
// ============================================================================

int ORM_SQLiteBeginTransaction(ORM_SQLiteConnection* conn) {
    if (!conn || !conn->db) {
        return -1;
    }
    
    pthread_mutex_lock(&conn->lock);
    
    char* errmsg = NULL;
    int rc;
    
    if (conn->transaction_level == 0) {
        // Start new transaction
        rc = sqlite3_exec(conn->db, "BEGIN TRANSACTION", NULL, NULL, &errmsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "BEGIN TRANSACTION failed: %s\n", errmsg);
            sqlite3_free(errmsg);
            pthread_mutex_unlock(&conn->lock);
            return -1;
        }
        conn->in_transaction = true;
    } else {
        // Use savepoint for nested transaction
        char savepoint[64];
        snprintf(savepoint, sizeof(savepoint), "SAVEPOINT sp_%d", conn->transaction_level);
        rc = sqlite3_exec(conn->db, savepoint, NULL, NULL, &errmsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SAVEPOINT failed: %s\n", errmsg);
            sqlite3_free(errmsg);
            pthread_mutex_unlock(&conn->lock);
            return -1;
        }
    }
    
    conn->transaction_level++;
    pthread_mutex_unlock(&conn->lock);
    
    return 0;
}

int ORM_SQLiteCommit(ORM_SQLiteConnection* conn) {
    if (!conn || !conn->db) {
        return -1;
    }
    
    pthread_mutex_lock(&conn->lock);
    
    if (conn->transaction_level == 0) {
        fprintf(stderr, "No transaction to commit\n");
        pthread_mutex_unlock(&conn->lock);
        return -1;
    }
    
    char* errmsg = NULL;
    int rc;
    
    conn->transaction_level--;
    
    if (conn->transaction_level == 0) {
        // Commit main transaction
        rc = sqlite3_exec(conn->db, "COMMIT", NULL, NULL, &errmsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "COMMIT failed: %s\n", errmsg);
            sqlite3_free(errmsg);
            conn->transaction_level++;  // Restore level on error
            pthread_mutex_unlock(&conn->lock);
            return -1;
        }
        conn->in_transaction = false;
    } else {
        // Release savepoint
        char savepoint[64];
        snprintf(savepoint, sizeof(savepoint), "RELEASE SAVEPOINT sp_%d", conn->transaction_level);
        rc = sqlite3_exec(conn->db, savepoint, NULL, NULL, &errmsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "RELEASE SAVEPOINT failed: %s\n", errmsg);
            sqlite3_free(errmsg);
            conn->transaction_level++;  // Restore level on error
            pthread_mutex_unlock(&conn->lock);
            return -1;
        }
    }
    
    pthread_mutex_unlock(&conn->lock);
    return 0;
}

int ORM_SQLiteRollback(ORM_SQLiteConnection* conn) {
    if (!conn || !conn->db) {
        return -1;
    }
    
    pthread_mutex_lock(&conn->lock);
    
    if (conn->transaction_level == 0) {
        fprintf(stderr, "No transaction to rollback\n");
        pthread_mutex_unlock(&conn->lock);
        return -1;
    }
    
    char* errmsg = NULL;
    int rc;
    
    conn->transaction_level--;
    
    if (conn->transaction_level == 0) {
        // Rollback main transaction
        rc = sqlite3_exec(conn->db, "ROLLBACK", NULL, NULL, &errmsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "ROLLBACK failed: %s\n", errmsg);
            sqlite3_free(errmsg);
            pthread_mutex_unlock(&conn->lock);
            return -1;
        }
        conn->in_transaction = false;
    } else {
        // Rollback to savepoint
        char savepoint[64];
        snprintf(savepoint, sizeof(savepoint), "ROLLBACK TO SAVEPOINT sp_%d", conn->transaction_level);
        rc = sqlite3_exec(conn->db, savepoint, NULL, NULL, &errmsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "ROLLBACK TO SAVEPOINT failed: %s\n", errmsg);
            sqlite3_free(errmsg);
            pthread_mutex_unlock(&conn->lock);
            return -1;
        }
    }
    
    pthread_mutex_unlock(&conn->lock);
    return 0;
}

// ============================================================================
// Savepoint Operations
// ============================================================================

int ORM_SQLiteSavepoint(ORM_SQLiteConnection* conn, const char* name) {
    if (!conn || !conn->db || !name) {
        return -1;
    }
    
    char sql[256];
    snprintf(sql, sizeof(sql), "SAVEPOINT %s", name);
    
    char* errmsg = NULL;
    int rc = sqlite3_exec(conn->db, sql, NULL, NULL, &errmsg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SAVEPOINT %s failed: %s\n", name, errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    
    return 0;
}

int ORM_SQLiteReleaseSavepoint(ORM_SQLiteConnection* conn, const char* name) {
    if (!conn || !conn->db || !name) {
        return -1;
    }
    
    char sql[256];
    snprintf(sql, sizeof(sql), "RELEASE SAVEPOINT %s", name);
    
    char* errmsg = NULL;
    int rc = sqlite3_exec(conn->db, sql, NULL, NULL, &errmsg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "RELEASE SAVEPOINT %s failed: %s\n", name, errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    
    return 0;
}

int ORM_SQLiteRollbackToSavepoint(ORM_SQLiteConnection* conn, const char* name) {
    if (!conn || !conn->db || !name) {
        return -1;
    }
    
    char sql[256];
    snprintf(sql, sizeof(sql), "ROLLBACK TO SAVEPOINT %s", name);
    
    char* errmsg = NULL;
    int rc = sqlite3_exec(conn->db, sql, NULL, NULL, &errmsg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "ROLLBACK TO SAVEPOINT %s failed: %s\n", name, errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    
    return 0;
}

bool ORM_SQLiteInTransaction(ORM_SQLiteConnection* conn) {
    if (!conn) {
        return false;
    }
    return conn->in_transaction;
}
