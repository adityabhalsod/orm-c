# Task 13: PostgreSQL Backend Implementation

## Objective
Implement a native PostgreSQL backend without external dependencies, supporting the PostgreSQL wire protocol, prepared statements, and PostgreSQL-specific features.

## Deliverables
1. PostgreSQL wire protocol implementation
2. Connection management
3. Query execution with parameters
4. Data type mapping
5. PostgreSQL-specific features

## Input Requirements
- Connection parameters (host, port, database, credentials)
- SQL queries and parameters
- PostgreSQL data types
- SSL/TLS requirements
- Connection options

## Output Specifications
- Database connections
- Query results with proper types
- Error messages and codes
- Transaction support
- COPY protocol support

## Implementation Guide

### 1. Wire Protocol Implementation
```c
// PostgreSQL protocol messages
typedef enum {
    PG_MSG_STARTUP = 0,
    PG_MSG_AUTHENTICATION = 'R',
    PG_MSG_QUERY = 'Q',
    PG_MSG_PARSE = 'P',
    PG_MSG_BIND = 'B',
    PG_MSG_EXECUTE = 'E',
    PG_MSG_DESCRIBE = 'D',
    PG_MSG_SYNC = 'S',
    PG_MSG_CLOSE = 'C',
    PG_MSG_TERMINATE = 'X',
    PG_MSG_ROW_DATA = 'D',
    PG_MSG_COMMAND_COMPLETE = 'C',
    PG_MSG_ERROR = 'E',
    PG_MSG_READY = 'Z'
} PG_MessageType;

// Protocol packet structure
typedef struct {
    char type;
    int32_t length;
    char* data;
} PG_Message;

// Protocol handler
typedef struct {
    int socket;
    ORM_StringBuffer* read_buffer;
    ORM_StringBuffer* write_buffer;
    int protocol_version;
    char* server_version;
    int server_encoding;
    bool is_ssl;
} PG_Protocol;

// Protocol operations
int PG_SendMessage(PG_Protocol* proto, PG_Message* msg);
PG_Message* PG_ReceiveMessage(PG_Protocol* proto);
int PG_SendStartup(PG_Protocol* proto, const char* database, const char* user);
int PG_Authenticate(PG_Protocol* proto, const char* password);
```

### 2. Connection Management
```c
typedef struct {
    PG_Protocol* protocol;
    char* host;
    int port;
    char* database;
    char* user;
    bool connected;
    int transaction_status;
    ORM_HashMap* prepared_statements;
    int backend_pid;
    int secret_key;
} ORM_PGConnection;

// Connection operations
ORM_PGConnection* ORM_PGConnect(
    const char* host,
    int port,
    const char* database,
    const char* user,
    const char* password
);
void ORM_PGDisconnect(ORM_PGConnection* conn);
bool ORM_PGPing(ORM_PGConnection* conn);
int ORM_PGReconnect(ORM_PGConnection* conn);

// Connection string parser
typedef struct {
    char* host;
    int port;
    char* dbname;
    char* user;
    char* password;
    char* sslmode;
    int connect_timeout;
} PG_ConnInfo;

PG_ConnInfo* ORM_PGParseConnString(const char* connstr);
```

### 3. Query Execution
```c
// Simple query protocol
ORM_ResultSet* ORM_PGExecQuery(
    ORM_PGConnection* conn,
    const char* query
);

// Extended query protocol
typedef struct {
    char* name;
    char* query;
    int param_count;
    Oid* param_types;
} PG_PreparedStatement;

PG_PreparedStatement* ORM_PGPrepare(
    ORM_PGConnection* conn,
    const char* name,
    const char* query,
    int param_count
);

ORM_ResultSet* ORM_PGExecutePrepared(
    ORM_PGConnection* conn,
    const char* stmt_name,
    void** params,
    int* param_lengths,
    int* param_formats,
    int param_count
);

// Async query support
int ORM_PGSendQuery(ORM_PGConnection* conn, const char* query);
ORM_ResultSet* ORM_PGGetResult(ORM_PGConnection* conn);
bool ORM_PGIsBusy(ORM_PGConnection* conn);
```

### 4. Data Type Mapping
```c
// PostgreSQL OIDs for common types
enum {
    PG_OID_BOOL = 16,
    PG_OID_INT2 = 21,
    PG_OID_INT4 = 23,
    PG_OID_INT8 = 20,
    PG_OID_FLOAT4 = 700,
    PG_OID_FLOAT8 = 701,
    PG_OID_VARCHAR = 1043,
    PG_OID_TEXT = 25,
    PG_OID_DATE = 1082,
    PG_OID_TIMESTAMP = 1114,
    PG_OID_UUID = 2950,
    PG_OID_JSON = 114,
    PG_OID_JSONB = 3802
};

// Type conversion
void* PG_ConvertFromBinary(Oid type, const char* data, int length);
char* PG_ConvertToBinary(Oid type, void* value, int* length);
void* PG_ConvertFromText(Oid type, const char* text);
char* PG_ConvertToText(Oid type, void* value);

// Array support
typedef struct {
    Oid element_type;
    int dimensions;
    int* dim_sizes;
    void** elements;
} PG_Array;

PG_Array* PG_ParseArray(const char* data, int length);
char* PG_FormatArray(PG_Array* array, int* length);
```

### 5. PostgreSQL-Specific Features
```c
// COPY protocol
typedef struct {
    ORM_PGConnection* conn;
    bool is_binary;
    bool is_from;
    char delimiter;
    char* null_string;
} PG_CopyContext;

PG_CopyContext* ORM_PGCopyFrom(
    ORM_PGConnection* conn,
    const char* table,
    const char** columns,
    int column_count
);

int ORM_PGCopyData(PG_CopyContext* ctx, const char* data, int length);
int ORM_PGCopyEnd(PG_CopyContext* ctx);

// LISTEN/NOTIFY
int ORM_PGListen(ORM_PGConnection* conn, const char* channel);
int ORM_PGUnlisten(ORM_PGConnection* conn, const char* channel);
int ORM_PGNotify(ORM_PGConnection* conn, const char* channel, const char* payload);

typedef struct {
    char* channel;
    char* payload;
    int backend_pid;
} PG_Notification;

PG_Notification* ORM_PGGetNotification(ORM_PGConnection* conn);

// Large objects
Oid ORM_PGCreateLO(ORM_PGConnection* conn);
int ORM_PGWriteLO(ORM_PGConnection* conn, Oid oid, const char* data, int length);
int ORM_PGReadLO(ORM_PGConnection* conn, Oid oid, char* buffer, int length);
int ORM_PGDeleteLO(ORM_PGConnection* conn, Oid oid);
```

### 6. Error Handling
```c
typedef struct {
    char severity[10];
    char sqlstate[6];
    char* primary_message;
    char* detail;
    char* hint;
    int position;
    char* internal_query;
    char* context;
    char* source_file;
    int source_line;
} PG_ErrorInfo;

PG_ErrorInfo* ORM_PGGetLastError(ORM_PGConnection* conn);
void ORM_PGClearError(ORM_PGConnection* conn);
bool ORM_PGIsConnectionError(const char* sqlstate);
```

## Testing Requirements

### Unit Tests
```c
void test_pg_connection() {
    ORM_PGConnection* conn = ORM_PGConnect(
        "localhost", 5432, "testdb", "user", "password"
    );
    assert(conn != NULL);
    assert(conn->connected);
    
    // Test ping
    assert(ORM_PGPing(conn));
    
    ORM_PGDisconnect(conn);
}

void test_pg_query() {
    ORM_PGConnection* conn = create_test_pg_connection();
    
    // Simple query
    ORM_ResultSet* rs = ORM_PGExecQuery(conn, 
        "SELECT 1 as num, 'test' as str");
    
    assert(rs != NULL);
    assert(rs->row_count == 1);
    assert(ORM_GetInt(rs, 0, "num") == 1);
    assert(strcmp(ORM_GetString(rs, 0, "str"), "test") == 0);
    
    ORM_FreeResultSet(rs);
    ORM_PGDisconnect(conn);
}

void test_prepared_statements() {
    ORM_PGConnection* conn = create_test_pg_connection();
    
    // Prepare statement
    PG_PreparedStatement* stmt = ORM_PGPrepare(conn,
        "get_user", "SELECT * FROM users WHERE id = $1", 1);
    
    // Execute with parameters
    int user_id = 123;
    void* params[] = {&user_id};
    int lengths[] = {sizeof(int)};
    int formats[] = {1}; // binary
    
    ORM_ResultSet* rs = ORM_PGExecutePrepared(conn,
        "get_user", params, lengths, formats, 1);
    
    assert(rs != NULL);
    ORM_FreeResultSet(rs);
    ORM_PGDisconnect(conn);
}

void test_copy_protocol() {
    ORM_PGConnection* conn = create_test_pg_connection();
    
    // Start COPY
    PG_CopyContext* copy = ORM_PGCopyFrom(conn,
        "users", (const char*[]){"id", "name"}, 2);
    
    // Send data
    ORM_PGCopyData(copy, "1\tJohn\n", 7);
    ORM_PGCopyData(copy, "2\tJane\n", 7);
    
    // End COPY
    int result = ORM_PGCopyEnd(copy);
    assert(result == 2); // 2 rows copied
    
    ORM_PGDisconnect(conn);
}
```

## Integration Points
- **Connection Pool**: Provides PostgreSQL connections
- **Query Builder**: Generates PostgreSQL-compatible SQL
- **Result Sets**: Handles PostgreSQL result formats
- **Transaction Management**: Uses PostgreSQL transactions
- **Migration System**: Executes PostgreSQL DDL

## Success Criteria
- [ ] Native protocol implementation works
- [ ] All PostgreSQL data types supported
- [ ] Prepared statements cached properly
- [ ] COPY protocol for bulk operations
- [ ] LISTEN/NOTIFY support
- [ ] SSL/TLS connection support
- [ ] < 10ms connection time
- [ ] Handle 1M+ rows efficiently

## Example Usage
```c
// Connect to PostgreSQL
ORM_PGConnection* conn = ORM_PGConnect(
    "localhost", 5432, "mydb", "user", "password"
);

// Execute query
ORM_ResultSet* users = ORM_PGExecQuery(conn,
    "SELECT * FROM users WHERE age > 18");

// Use prepared statements
PG_PreparedStatement* stmt = ORM_PGPrepare(conn,
    "insert_user", 
    "INSERT INTO users (name, email) VALUES ($1, $2)",
    2
);

char* name = "John";
char* email = "john@example.com";
void* params[] = {name, email};
ORM_PGExecutePrepared(conn, "insert_user", params, NULL, NULL, 2);

// Bulk copy
PG_CopyContext* copy = ORM_PGCopyFrom(conn, "users", NULL, 0);
for (int i = 0; i < 10000; i++) {
    char buffer[256];
    sprintf(buffer, "%d\tUser%d\tuser%d@example.com\n", i, i, i);
    ORM_PGCopyData(copy, buffer, strlen(buffer));
}
ORM_PGCopyEnd(copy);

// LISTEN/NOTIFY
ORM_PGListen(conn, "user_updates");
// ... in another thread/process
ORM_PGNotify(conn, "user_updates", "user:123:updated");

ORM_PGDisconnect(conn);
```

## Files to Create
- `src/backends/postgresql/pg_protocol.h` - Protocol definitions
- `src/backends/postgresql/pg_protocol.c` - Wire protocol
- `src/backends/postgresql/pg_connection.c` - Connection management
- `src/backends/postgresql/pg_query.c` - Query execution
- `src/backends/postgresql/pg_types.c` - Type conversion
- `src/backends/postgresql/pg_copy.c` - COPY protocol
- `tests/test_postgresql.c` - PostgreSQL tests
