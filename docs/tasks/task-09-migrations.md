# Task 09: Migration System

## Objective
Implement a database migration system that tracks schema changes, generates migration files, and provides forward and rollback capabilities similar to Django migrations.

## Deliverables
1. Migration file generation
2. Schema change detection
3. Migration runner
4. Rollback support
5. Migration history tracking

## Input Requirements
- Model definitions
- Current database schema
- Migration history
- Schema changes
- Custom migration operations

## Output Specifications
- Migration files
- SQL DDL statements
- Migration history table
- Schema diff reports
- Rollback scripts

## Implementation Guide

### 1. Migration Structure
```c
typedef struct {
    char* name;
    char* timestamp;
    void (*forward)(ORM_MigrationContext* ctx);
    void (*backward)(ORM_MigrationContext* ctx);
    char** dependencies;
    int dependency_count;
} ORM_Migration;

typedef struct {
    ORM_Connection* conn;
    ORM_StringBuffer* sql_buffer;
    bool dry_run;
    void (*log)(const char* message);
} ORM_MigrationContext;

// Migration operations
typedef struct {
    enum {
        OP_CREATE_TABLE,
        OP_DROP_TABLE,
        OP_ADD_COLUMN,
        OP_DROP_COLUMN,
        OP_ALTER_COLUMN,
        OP_CREATE_INDEX,
        OP_DROP_INDEX,
        OP_RAW_SQL
    } type;
    void* params;
} ORM_MigrationOp;
```

### 2. Migration Generation
```c
// Auto-detect changes
typedef struct {
    ORM_ModelMeta** old_models;
    ORM_ModelMeta** new_models;
    int old_count;
    int new_count;
} ORM_SchemaState;

ORM_Migration* ORM_GenerateMigration(
    const char* name,
    ORM_SchemaState* old_state,
    ORM_SchemaState* new_state
);

// Migration file writer
int ORM_WriteMigrationFile(
    ORM_Migration* migration,
    const char* directory
);

// Migration template
#define ORM_MIGRATION(name, timestamp) \
    static void name##_forward(ORM_MigrationContext* ctx); \
    static void name##_backward(ORM_MigrationContext* ctx); \
    ORM_Migration name = { \
        .name = #name, \
        .timestamp = timestamp, \
        .forward = name##_forward, \
        .backward = name##_backward \
    }
```

### 3. Schema Operations
```c
// Table operations
void ORM_CreateTable(ORM_MigrationContext* ctx, ORM_ModelMeta* model);
void ORM_DropTable(ORM_MigrationContext* ctx, const char* table_name);
void ORM_RenameTable(ORM_MigrationContext* ctx, const char* old, const char* new);

// Column operations
void ORM_AddColumn(ORM_MigrationContext* ctx, 
                   const char* table,
                   ORM_FieldMeta* field);
void ORM_DropColumn(ORM_MigrationContext* ctx,
                    const char* table,
                    const char* column);
void ORM_AlterColumn(ORM_MigrationContext* ctx,
                     const char* table,
                     const char* column,
                     ORM_FieldMeta* new_field);

// Index operations
void ORM_CreateIndex(ORM_MigrationContext* ctx,
                     const char* table,
                     const char* columns[],
                     int column_count,
                     bool unique);
void ORM_DropIndex(ORM_MigrationContext* ctx,
                   const char* index_name);

// Raw SQL
void ORM_RunSQL(ORM_MigrationContext* ctx, const char* sql);
```

### 4. Migration History
```c
// History table structure
typedef struct {
    int id;
    char name[255];
    char timestamp[32];
    char applied_at[32];
} ORM_MigrationHistory;

// History operations
int ORM_CreateHistoryTable(ORM_Connection* conn);
bool ORM_IsMigrationApplied(ORM_Connection* conn, const char* name);
int ORM_RecordMigration(ORM_Connection* conn, const char* name);
int ORM_RemoveMigration(ORM_Connection* conn, const char* name);
ORM_MigrationHistory* ORM_GetAppliedMigrations(ORM_Connection* conn, int* count);
```

### 5. Migration Runner
```c
typedef struct {
    ORM_Migration** migrations;
    int migration_count;
    ORM_Connection* connection;
    bool fake;  // Record without running
    bool verbose;
} ORM_MigrationRunner;

// Runner operations
ORM_MigrationRunner* ORM_CreateRunner(ORM_Connection* conn);
int ORM_RunMigrations(ORM_MigrationRunner* runner, const char* target);
int ORM_RollbackMigrations(ORM_MigrationRunner* runner, int steps);
int ORM_ShowMigrations(ORM_MigrationRunner* runner);
void ORM_FreeRunner(ORM_MigrationRunner* runner);

// Migration discovery
ORM_Migration** ORM_DiscoverMigrations(const char* directory, int* count);
```

### 6. Schema Introspection
```c
// Get current database schema
typedef struct {
    char* table_name;
    ORM_ColumnInfo* columns;
    int column_count;
    ORM_IndexInfo* indexes;
    int index_count;
} ORM_TableSchema;

ORM_TableSchema* ORM_IntrospectTable(ORM_Connection* conn, const char* table);
ORM_TableSchema** ORM_IntrospectDatabase(ORM_Connection* conn, int* count);

// Compare schemas
typedef struct {
    enum { DIFF_ADD, DIFF_REMOVE, DIFF_ALTER } type;
    char* table;
    char* column;
    void* old_value;
    void* new_value;
} ORM_SchemaDiff;

ORM_SchemaDiff* ORM_CompareSchemas(
    ORM_TableSchema* old,
    ORM_TableSchema* new,
    int* diff_count
);
```

## Testing Requirements

### Unit Tests
```c
void test_migration_generation() {
    // Create old and new model states
    ORM_SchemaState old = { .models = old_models, .count = 1 };
    ORM_SchemaState new = { .models = new_models, .count = 2 };
    
    // Generate migration
    ORM_Migration* mig = ORM_GenerateMigration("add_table", &old, &new);
    
    assert(mig != NULL);
    assert(mig->forward != NULL);
    assert(mig->backward != NULL);
}

void test_migration_execution() {
    ORM_Connection* conn = create_test_connection();
    ORM_MigrationRunner* runner = ORM_CreateRunner(conn);
    
    // Create test migration
    ORM_MIGRATION(test_001, "2024-01-01");
    
    // Run forward
    int result = ORM_RunMigrations(runner, "test_001");
    assert(result == 0);
    
    // Check if applied
    assert(ORM_IsMigrationApplied(conn, "test_001"));
    
    // Rollback
    result = ORM_RollbackMigrations(runner, 1);
    assert(result == 0);
    
    // Check if removed
    assert(!ORM_IsMigrationApplied(conn, "test_001"));
    
    ORM_FreeRunner(runner);
}

void test_schema_introspection() {
    ORM_Connection* conn = create_test_connection();
    
    // Create test table
    ORM_ExecuteSQL(conn, 
        "CREATE TABLE test (id INT, name VARCHAR(50))");
    
    // Introspect
    ORM_TableSchema* schema = ORM_IntrospectTable(conn, "test");
    
    assert(schema != NULL);
    assert(schema->column_count == 2);
    assert(strcmp(schema->columns[0].name, "id") == 0);
    
    ORM_FreeTableSchema(schema);
}
```

## Integration Points
- **Model System**: Reads model definitions
- **SQL Generation**: Creates DDL statements
- **Database Backends**: Executes migrations
- **Schema Operations**: Modifies database schema
- **Version Control**: Migration files for git

## Success Criteria
- [ ] Auto-detects schema changes
- [ ] Generates valid migration files
- [ ] Forward migrations work
- [ ] Rollback works correctly
- [ ] Migration history tracked
- [ ] Dependency resolution works
- [ ] Dry-run mode available

## Example Usage
```c
// Define migration
ORM_MIGRATION(create_users_table, "2024-01-15-1200");

static void create_users_table_forward(ORM_MigrationContext* ctx) {
    ORM_CreateTable(ctx, &User_meta);
    ORM_CreateIndex(ctx, "users", 
                    (const char*[]){"email"}, 1, true);
}

static void create_users_table_backward(ORM_MigrationContext* ctx) {
    ORM_DropTable(ctx, "users");
}

// Run migrations
int main() {
    ORM_Init("app.db");
    
    ORM_MigrationRunner* runner = ORM_CreateRunner(orm_connection);
    
    // Show pending migrations
    ORM_ShowMigrations(runner);
    
    // Run all migrations
    int result = ORM_RunMigrations(runner, NULL);
    
    if (result != 0) {
        // Rollback on error
        ORM_RollbackMigrations(runner, 1);
    }
    
    ORM_FreeRunner(runner);
    ORM_Shutdown();
}

// CLI usage
// ./orm migrate           # Run all pending
// ./orm migrate --fake    # Mark as run without executing
// ./orm rollback 2        # Rollback 2 migrations
// ./orm showmigrations    # List all migrations
```

## Files to Create
- `src/migrations/migration.h` - Migration system interface
- `src/migrations/migration.c` - Core implementation
- `src/migrations/generator.c` - Auto-generation
- `src/migrations/runner.c` - Migration execution
- `src/migrations/history.c` - History tracking
- `src/migrations/introspect.c` - Schema introspection
- `tests/test_migrations.c` - Migration tests
