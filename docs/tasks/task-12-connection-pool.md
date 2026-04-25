# Task 12: Connection Pooling

## Objective
Implement an efficient connection pooling system that manages database connections, reduces overhead, handles connection lifecycle, and provides thread-safe access.

## Deliverables
1. Connection pool manager
2. Connection lifecycle management
3. Thread-safe connection checkout
4. Connection health checking
5. Pool statistics and monitoring

## Input Requirements
- Database connection parameters
- Pool size configuration
- Timeout settings
- Health check intervals
- Thread concurrency levels

## Output Specifications
- Available connections
- Connection lease tracking
- Pool statistics
- Health status reports
- Performance metrics

## Implementation Guide

### 1. Connection Pool Structure
```c
typedef struct {
    ORM_Connection* connection;
    bool in_use;
    time_t last_used;
    time_t created_at;
    int use_count;
    pthread_t owner_thread;
    bool is_healthy;
} ORM_PooledConnection;

typedef struct {
    ORM_PooledConnection** connections;
    int pool_size;
    int min_size;
    int max_size;
    int active_count;
    int idle_count;
    
    // Configuration
    int max_idle_time;
    int max_lifetime;
    int connection_timeout;
    int checkout_timeout;
    
    // Thread safety
    pthread_mutex_t pool_lock;
    pthread_cond_t available_cond;
    
    // Statistics
    ORM_PoolStats stats;
} ORM_ConnectionPool;

typedef struct {
    uint64_t total_connections_created;
    uint64_t total_checkouts;
    uint64_t total_returns;
    uint64_t failed_checkouts;
    uint64_t connection_errors;
    double avg_wait_time;
    double avg_use_time;
} ORM_PoolStats;
```

### 2. Pool Management
```c
// Pool lifecycle
ORM_ConnectionPool* ORM_CreatePool(
    const char* connection_string,
    int min_size,
    int max_size
);
void ORM_DestroyPool(ORM_ConnectionPool* pool);

// Configuration
void ORM_ConfigurePool(ORM_ConnectionPool* pool, ORM_PoolConfig* config);

typedef struct {
    int min_connections;
    int max_connections;
    int max_idle_time_seconds;
    int max_lifetime_seconds;
    int connection_timeout_ms;
    int checkout_timeout_ms;
    bool test_on_checkout;
    bool test_on_return;
    int health_check_interval;
} ORM_PoolConfig;

// Pool operations
int ORM_InitializePool(ORM_ConnectionPool* pool);
int ORM_ResizePool(ORM_ConnectionPool* pool, int new_size);
void ORM_DrainPool(ORM_ConnectionPool* pool);
```

### 3. Connection Checkout/Return
```c
// Get connection from pool
ORM_Connection* ORM_CheckoutConnection(
    ORM_ConnectionPool* pool,
    int timeout_ms
);

// Return connection to pool
void ORM_ReturnConnection(
    ORM_ConnectionPool* pool,
    ORM_Connection* conn
);

// Automatic return with RAII
typedef struct {
    ORM_Connection* conn;
    ORM_ConnectionPool* pool;
} ORM_PooledConnectionHandle;

ORM_PooledConnectionHandle* ORM_GetPooledConnection(ORM_ConnectionPool* pool);
void ORM_ReleasePooledConnection(ORM_PooledConnectionHandle* handle);

// Macro for automatic cleanup
#define ORM_WITH_CONNECTION(pool, conn_var, block) \
    do { \
        ORM_Connection* conn_var = ORM_CheckoutConnection(pool, -1); \
        if (conn_var) { \
            block; \
            ORM_ReturnConnection(pool, conn_var); \
        } \
    } while(0)
```

### 4. Connection Health
```c
// Health checking
typedef bool (*ORM_HealthCheck)(ORM_Connection* conn);

void ORM_SetHealthCheck(
    ORM_ConnectionPool* pool,
    ORM_HealthCheck check_func
);

bool ORM_DefaultHealthCheck(ORM_Connection* conn);
bool ORM_IsConnectionHealthy(ORM_PooledConnection* conn);
void ORM_ValidateConnection(ORM_PooledConnection* conn);

// Connection refresh
int ORM_RefreshConnection(ORM_PooledConnection* conn);
void ORM_EvictStaleConnections(ORM_ConnectionPool* pool);

// Background health monitoring
void ORM_StartHealthMonitor(ORM_ConnectionPool* pool);
void ORM_StopHealthMonitor(ORM_ConnectionPool* pool);
```

### 5. Pool Statistics
```c
// Statistics gathering
ORM_PoolStats* ORM_GetPoolStats(ORM_ConnectionPool* pool);
void ORM_ResetPoolStats(ORM_ConnectionPool* pool);

// Monitoring
int ORM_GetActiveConnections(ORM_ConnectionPool* pool);
int ORM_GetIdleConnections(ORM_ConnectionPool* pool);
int ORM_GetWaitingThreads(ORM_ConnectionPool* pool);
double ORM_GetPoolUtilization(ORM_ConnectionPool* pool);

// Logging
void ORM_LogPoolStatus(ORM_ConnectionPool* pool);
char* ORM_FormatPoolStats(ORM_PoolStats* stats);

// Alerts
typedef void (*ORM_PoolAlert)(ORM_ConnectionPool* pool, const char* message);
void ORM_SetPoolAlertHandler(ORM_ConnectionPool* pool, ORM_PoolAlert handler);
```

### 6. Thread Safety
```c
// Thread-local storage for connection affinity
__thread ORM_Connection* thread_local_connection;

// Thread-safe operations
ORM_Connection* ORM_GetThreadConnection(ORM_ConnectionPool* pool);
void ORM_ReleaseThreadConnection(ORM_ConnectionPool* pool);

// Connection reservation
typedef struct {
    ORM_Connection* connection;
    time_t reserved_until;
    char* reservation_token;
} ORM_ReservedConnection;

ORM_ReservedConnection* ORM_ReserveConnection(
    ORM_ConnectionPool* pool,
    int duration_seconds
);
void ORM_ReleaseReservation(
    ORM_ConnectionPool* pool,
    ORM_ReservedConnection* reservation
);
```

## Testing Requirements

### Unit Tests
```c
void test_pool_creation() {
    ORM_ConnectionPool* pool = ORM_CreatePool(":memory:", 5, 10);
    assert(pool != NULL);
    
    int result = ORM_InitializePool(pool);
    assert(result == 0);
    assert(ORM_GetIdleConnections(pool) == 5);
    
    ORM_DestroyPool(pool);
}

void test_connection_checkout() {
    ORM_ConnectionPool* pool = ORM_CreatePool(":memory:", 2, 5);
    ORM_InitializePool(pool);
    
    ORM_Connection* conn1 = ORM_CheckoutConnection(pool, 1000);
    assert(conn1 != NULL);
    assert(ORM_GetActiveConnections(pool) == 1);
    
    ORM_Connection* conn2 = ORM_CheckoutConnection(pool, 1000);
    assert(conn2 != NULL);
    assert(conn1 != conn2);
    
    ORM_ReturnConnection(pool, conn1);
    assert(ORM_GetIdleConnections(pool) == 1);
    
    ORM_ReturnConnection(pool, conn2);
    ORM_DestroyPool(pool);
}

void test_pool_exhaustion() {
    ORM_ConnectionPool* pool = ORM_CreatePool(":memory:", 1, 1);
    ORM_InitializePool(pool);
    
    ORM_Connection* conn1 = ORM_CheckoutConnection(pool, 100);
    assert(conn1 != NULL);
    
    // Pool exhausted, should timeout
    ORM_Connection* conn2 = ORM_CheckoutConnection(pool, 100);
    assert(conn2 == NULL);
    
    ORM_ReturnConnection(pool, conn1);
    
    // Now should succeed
    conn2 = ORM_CheckoutConnection(pool, 100);
    assert(conn2 != NULL);
    
    ORM_ReturnConnection(pool, conn2);
    ORM_DestroyPool(pool);
}

void test_concurrent_access() {
    ORM_ConnectionPool* pool = ORM_CreatePool(":memory:", 5, 10);
    ORM_InitializePool(pool);
    
    #define NUM_THREADS 20
    pthread_t threads[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker_thread, pool);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Check pool integrity
    assert(ORM_GetActiveConnections(pool) == 0);
    assert(pool->stats.connection_errors == 0);
    
    ORM_DestroyPool(pool);
}

void test_health_check() {
    ORM_ConnectionPool* pool = ORM_CreatePool("test.db", 2, 5);
    ORM_InitializePool(pool);
    
    // Set custom health check
    ORM_SetHealthCheck(pool, custom_health_check);
    
    // Simulate unhealthy connection
    ORM_Connection* conn = ORM_CheckoutConnection(pool, 1000);
    corrupt_connection(conn);
    ORM_ReturnConnection(pool, conn);
    
    // Pool should detect and refresh
    ORM_EvictStaleConnections(pool);
    
    conn = ORM_CheckoutConnection(pool, 1000);
    assert(ORM_IsConnectionHealthy(conn));
    
    ORM_ReturnConnection(pool, conn);
    ORM_DestroyPool(pool);
}
```

## Integration Points
- **Database Backends**: Creates backend-specific connections
- **Transaction Management**: Ensures connection consistency
- **Query Execution**: Provides connections for queries
- **Thread Management**: Handles multi-threaded access
- **Monitoring**: Exports metrics

## Success Criteria
- [ ] Thread-safe connection management
- [ ] No connection leaks
- [ ] Automatic reconnection on failure
- [ ] Configurable pool sizing
- [ ] Health checks prevent bad connections
- [ ] < 1ms checkout time for available connections
- [ ] Handles 100+ concurrent threads

## Example Usage
```c
// Create and configure pool
ORM_ConnectionPool* pool = ORM_CreatePool("postgresql://localhost/mydb", 10, 50);

ORM_PoolConfig config = {
    .min_connections = 10,
    .max_connections = 50,
    .max_idle_time_seconds = 300,
    .connection_timeout_ms = 5000,
    .test_on_checkout = true
};
ORM_ConfigurePool(pool, &config);
ORM_InitializePool(pool);

// Use connection from pool
ORM_WITH_CONNECTION(pool, conn, {
    QuerySet* users = User_objects_with_conn(conn)->all();
    // Connection automatically returned
});

// Manual checkout/return
ORM_Connection* conn = ORM_CheckoutConnection(pool, 1000);
if (conn) {
    // Use connection
    perform_database_operations(conn);
    ORM_ReturnConnection(pool, conn);
}

// Monitor pool
ORM_PoolStats* stats = ORM_GetPoolStats(pool);
printf("Pool utilization: %.2f%%\n", ORM_GetPoolUtilization(pool) * 100);
printf("Active connections: %d\n", ORM_GetActiveConnections(pool));

// Cleanup
ORM_DrainPool(pool);
ORM_DestroyPool(pool);
```

## Files to Create
- `src/pool/connection_pool.h` - Pool interface
- `src/pool/connection_pool.c` - Core implementation
- `src/pool/health_check.c` - Health monitoring
- `src/pool/statistics.c` - Stats collection
- `src/pool/thread_safety.c` - Thread management
- `tests/test_pool.c` - Connection pool tests
