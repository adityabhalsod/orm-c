/* SQLite Backend for ORM-C
 * 
 * Provides SQLite database backend with connection pooling,
 * prepared statements, transactions, and schema operations.
 */

#ifndef ORM_SQLITE_BACKEND_H
#define ORM_SQLITE_BACKEND_H

#include "../../core/model.h"
#include "../../core/sql_builder.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <pthread.h>

// Forward declarations
typedef struct ORM_SQLiteConnection ORM_SQLiteConnection;
typedef struct ORM_PreparedStatement ORM_PreparedStatement;
typedef struct ORM_ResultSet ORM_ResultSet;
typedef struct ORM_SQLitePool ORM_SQLitePool;

// ============================================================================
// Connection Management
// ============================================================================

struct ORM_SQLiteConnection {
    sqlite3* db;
    char* database_path;
    bool is_open;
    bool in_transaction;
    int transaction_level;
    pthread_mutex_t lock;
    void* prepared_cache;  // HashMap of prepared statements
};

/* Connect to SQLite database (use ":memory:" for in-memory) */
ORM_SQLiteConnection* ORM_SQLiteConnect(const char* database);

/* Disconnect from database */
void ORM_SQLiteDisconnect(ORM_SQLiteConnection* conn);

/* Check if connection is open */
bool ORM_SQLiteIsConnected(ORM_SQLiteConnection* conn);

/* Get last error message */
const char* ORM_SQLiteGetError(ORM_SQLiteConnection* conn);

// ============================================================================
// Query Execution
// ============================================================================

/* Raw result set structure for database rows */
typedef struct {
    char** column_names;
    int column_count;
    void*** rows;  // Array of row arrays
    int row_count;
    int current_row;
    ORM_FieldType* column_types;
} ORM_SQLiteResultSet;

/* Execute SQL query with parameters */
ORM_SQLiteResultSet* ORM_SQLiteExecute(
    ORM_SQLiteConnection* conn,
    const char* sql,
    ORM_Parameter* params,
    int param_count
);

/* Execute SQL without results (INSERT/UPDATE/DELETE) */
int ORM_SQLiteExecuteNonQuery(
    ORM_SQLiteConnection* conn,
    const char* sql,
    ORM_Parameter* params,
    int param_count
);

/* Get last inserted row ID */
int64_t ORM_SQLiteLastInsertId(ORM_SQLiteConnection* conn);

/* Get number of affected rows */
int ORM_SQLiteAffectedRows(ORM_SQLiteConnection* conn);

/* Free result set */
void ORM_FreeSQLiteResultSet(ORM_SQLiteResultSet* rs);

// ============================================================================
// Prepared Statements
// ============================================================================

struct ORM_PreparedStatement {
    sqlite3_stmt* stmt;
    char* sql;
    int use_count;
    time_t last_used;
};

/* Prepare statement (with caching) */
ORM_PreparedStatement* ORM_SQLitePrepare(
    ORM_SQLiteConnection* conn,
    const char* sql
);

/* Bind parameters to prepared statement */
int ORM_SQLiteBindParameters(
    ORM_PreparedStatement* stmt,
    ORM_Parameter* params,
    int param_count
);

/* Execute prepared statement */
ORM_SQLiteResultSet* ORM_SQLiteExecutePrepared(ORM_PreparedStatement* stmt);

/* Reset prepared statement for reuse */
void ORM_SQLiteResetStatement(ORM_PreparedStatement* stmt);

/* Free prepared statement */
void ORM_SQLiteFreePrepared(ORM_PreparedStatement* stmt);

// ============================================================================
// Transaction Management
// ============================================================================

/* Begin transaction */
int ORM_SQLiteBeginTransaction(ORM_SQLiteConnection* conn);

/* Commit transaction */
int ORM_SQLiteCommit(ORM_SQLiteConnection* conn);

/* Rollback transaction */
int ORM_SQLiteRollback(ORM_SQLiteConnection* conn);

/* Create savepoint */
int ORM_SQLiteSavepoint(ORM_SQLiteConnection* conn, const char* name);

/* Release savepoint */
int ORM_SQLiteReleaseSavepoint(ORM_SQLiteConnection* conn, const char* name);

/* Rollback to savepoint */
int ORM_SQLiteRollbackToSavepoint(ORM_SQLiteConnection* conn, const char* name);

/* Check if in transaction */
bool ORM_SQLiteInTransaction(ORM_SQLiteConnection* conn);

// ============================================================================
// Schema Operations
// ============================================================================

/* Column information structure */
typedef struct {
    char* name;
    char* type;
    bool not_null;
    char* default_value;
    bool primary_key;
} ORM_ColumnInfo;

/* Create table from model metadata */
int ORM_SQLiteCreateTable(ORM_SQLiteConnection* conn, ORM_ModelMeta* model);

/* Drop table */
int ORM_SQLiteDropTable(ORM_SQLiteConnection* conn, const char* table_name);

/* Check if table exists */
bool ORM_SQLiteTableExists(ORM_SQLiteConnection* conn, const char* table_name);

/* Get table columns */
ORM_ColumnInfo* ORM_SQLiteGetColumns(
    ORM_SQLiteConnection* conn,
    const char* table_name,
    int* count
);

/* Free column info array */
void ORM_FreeColumnInfo(ORM_ColumnInfo* columns, int count);

/* Create index */
int ORM_SQLiteCreateIndex(
    ORM_SQLiteConnection* conn,
    const char* table_name,
    const char* column_name,
    bool unique
);

/* Drop index */
int ORM_SQLiteDropIndex(ORM_SQLiteConnection* conn, const char* index_name);

// ============================================================================
// Utility Functions
// ============================================================================

/* VACUUM database */
int ORM_SQLiteVacuum(ORM_SQLiteConnection* conn);

/* ANALYZE database */
int ORM_SQLiteAnalyze(ORM_SQLiteConnection* conn);

/* Get database size in bytes */
int64_t ORM_SQLiteGetDatabaseSize(ORM_SQLiteConnection* conn);

/* Enable/disable foreign keys */
int ORM_SQLiteSetForeignKeys(ORM_SQLiteConnection* conn, bool enabled);

#endif /* ORM_SQLITE_BACKEND_H */
