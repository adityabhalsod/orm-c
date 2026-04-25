# Task 11: Transaction Management

## Objective
Implement robust transaction management with support for nested transactions, savepoints, isolation levels, and automatic rollback on errors.

## Deliverables
1. Transaction context management
2. Nested transaction support
3. Savepoint implementation
4. Isolation level configuration
5. Automatic rollback mechanisms

## Input Requirements
- Transaction boundaries
- Isolation level settings
- Savepoint names
- Error conditions
- Connection state

## Output Specifications
- Transaction handles
- Commit/rollback status
- Transaction state tracking
- Error recovery
- Performance metrics

## Implementation Guide

### 1. Transaction Structure
```c
typedef struct {
    ORM_Connection* connection;
    char* transaction_id;
    int nesting_level;
    bool is_active;
    bool auto_commit;
    ORM_IsolationLevel isolation_level;
    struct ORM_Transaction* parent;
    ORM_StringList* savepoints;
    time_t start_time;
} ORM_Transaction;

typedef enum {
    ISOLATION_READ_UNCOMMITTED,
    ISOLATION_READ_COMMITTED,
    ISOLATION_REPEATABLE_READ,
    ISOLATION_SERIALIZABLE
} ORM_IsolationLevel;

// Transaction manager
typedef struct {
    ORM_Transaction* current;
    ORM_Stack* transaction_stack;
    bool auto_rollback;
    void (*on_commit)(ORM_Transaction*);
    void (*on_rollback)(ORM_Transaction*);
} ORM_TransactionManager;
```

### 2. Transaction Operations
```c
// Basic transaction control
ORM_Transaction* ORM_BeginTransaction(ORM_Connection* conn);
int ORM_Commit(ORM_Transaction* txn);
int ORM_Rollback(ORM_Transaction* txn);

// Transaction configuration
void ORM_SetIsolationLevel(ORM_Transaction* txn, ORM_IsolationLevel level);
void ORM_SetAutoCommit(ORM_Connection* conn, bool auto_commit);

// Transaction state
bool ORM_InTransaction(ORM_Connection* conn);
ORM_Transaction* ORM_CurrentTransaction(ORM_Connection* conn);

// Transaction context manager
#define ORM_WITH_TRANSACTION(conn, block) \
    do { \
        ORM_Transaction* __txn = ORM_BeginTransaction(conn); \
        bool __success = true; \
        do block while(0); \
        if (__success) ORM_Commit(__txn); \
        else ORM_Rollback(__txn); \
    } while(0)
```

### 3. Nested Transactions
```c
// Nested transaction support using savepoints
ORM_Transaction* ORM_BeginNestedTransaction(ORM_Transaction* parent);
int ORM_CommitNested(ORM_Transaction* nested);
int ORM_RollbackNested(ORM_Transaction* nested);

// Savepoint operations
int ORM_CreateSavepoint(ORM_Transaction* txn, const char* name);
int ORM_ReleaseSavepoint(ORM_Transaction* txn, const char* name);
int ORM_RollbackToSavepoint(ORM_Transaction* txn, const char* name);

// Automatic savepoint naming
char* ORM_GenerateSavepointName(ORM_Transaction* txn);
```

### 4. Error Handling
```c
// Transaction error handling
typedef struct {
    int error_code;
    char* error_message;
    bool can_retry;
    bool should_rollback;
} ORM_TransactionError;

// Error handlers
void ORM_SetTransactionErrorHandler(
    ORM_TransactionManager* mgr,
    void (*handler)(ORM_TransactionError*)
);

// Automatic rollback on error
#define ORM_TRANSACTION_SAFE(conn, block) \
    do { \
        ORM_Transaction* __txn = ORM_BeginTransaction(conn); \
        jmp_buf __jmp; \
        if (setjmp(__jmp) == 0) { \
            block; \
            ORM_Commit(__txn); \
        } else { \
            ORM_Rollback(__txn); \
        } \
    } while(0)

// Retry logic
int ORM_RetryTransaction(
    ORM_Connection* conn,
    int (*operation)(ORM_Transaction*),
    int max_retries
);
```

### 5. Transaction Hooks
```c
// Transaction lifecycle hooks
typedef struct {
    void (*before_begin)(ORM_Connection* conn);
    void (*after_begin)(ORM_Transaction* txn);
    void (*before_commit)(ORM_Transaction* txn);
    void (*after_commit)(ORM_Transaction* txn);
    void (*before_rollback)(ORM_Transaction* txn);
    void (*after_rollback)(ORM_Transaction* txn);
} ORM_TransactionHooks;

void ORM_RegisterTransactionHooks(
    ORM_Connection* conn,
    ORM_TransactionHooks* hooks
);

// Middleware pattern
typedef bool (*ORM_TransactionMiddleware)(
    ORM_Transaction* txn,
    void* context
);

void ORM_AddTransactionMiddleware(
    ORM_TransactionManager* mgr,
    ORM_TransactionMiddleware middleware,
    void* context
);
```

### 6. Distributed Transactions
```c
// Two-phase commit support
typedef struct {
    char* transaction_id;
    ORM_Connection** participants;
    int participant_count;
    bool is_coordinator;
} ORM_DistributedTransaction;

// 2PC operations
ORM_DistributedTransaction* ORM_Begin2PC(
    ORM_Connection** connections,
    int count
);
int ORM_Prepare2PC(ORM_DistributedTransaction* dtxn);
int ORM_Commit2PC(ORM_DistributedTransaction* dtxn);
int ORM_Rollback2PC(ORM_DistributedTransaction* dtxn);
```

## Testing Requirements

### Unit Tests
```c
void test_basic_transaction() {
    ORM_Connection* conn = create_test_connection();
    
    ORM_Transaction* txn = ORM_BeginTransaction(conn);
    assert(txn != NULL);
    assert(ORM_InTransaction(conn));
    
    // Perform operations
    User* user = User_create(.name = "Test");
    
    // Commit
    assert(ORM_Commit(txn) == 0);
    assert(!ORM_InTransaction(conn));
    
    // Verify data persisted
    User* loaded = User_get(user->id);
    assert(loaded != NULL);
}

void test_rollback() {
    ORM_Connection* conn = create_test_connection();
    
    ORM_Transaction* txn = ORM_BeginTransaction(conn);
    
    // Create user in transaction
    User* user = User_create(.name = "WillRollback");
    int user_id = user->id;
    
    // Rollback
    ORM_Rollback(txn);
    
    // User should not exist
    User* loaded = User_get(user_id);
    assert(loaded == NULL);
}

void test_nested_transactions() {
    ORM_Connection* conn = create_test_connection();
    
    ORM_Transaction* outer = ORM_BeginTransaction(conn);
    User* user1 = User_create(.name = "Outer");
    
    ORM_Transaction* inner = ORM_BeginNestedTransaction(outer);
    User* user2 = User_create(.name = "Inner");
    
    // Rollback inner
    ORM_RollbackNested(inner);
    
    // Commit outer
    ORM_Commit(outer);
    
    // user1 should exist, user2 should not
    assert(User_get(user1->id) != NULL);
    assert(User_get(user2->id) == NULL);
}

void test_savepoints() {
    ORM_Connection* conn = create_test_connection();
    ORM_Transaction* txn = ORM_BeginTransaction(conn);
    
    User* user1 = User_create(.name = "Before");
    ORM_CreateSavepoint(txn, "sp1");
    
    User* user2 = User_create(.name = "After");
    
    // Rollback to savepoint
    ORM_RollbackToSavepoint(txn, "sp1");
    ORM_Commit(txn);
    
    // user1 should exist, user2 should not
    assert(User_get(user1->id) != NULL);
    assert(User_get(user2->id) == NULL);
}

void test_isolation_levels() {
    ORM_Connection* conn1 = create_connection();
    ORM_Connection* conn2 = create_connection();
    
    // Test READ_COMMITTED
    ORM_Transaction* txn1 = ORM_BeginTransaction(conn1);
    ORM_SetIsolationLevel(txn1, ISOLATION_READ_COMMITTED);
    
    ORM_Transaction* txn2 = ORM_BeginTransaction(conn2);
    
    // txn1 creates user
    User* user = User_create_in_txn(txn1, .name = "IsolationTest");
    
    // txn2 shouldn't see it yet
    User* loaded = User_get_in_txn(txn2, user->id);
    assert(loaded == NULL);
    
    ORM_Commit(txn1);
    
    // Now txn2 should see it
    loaded = User_get_in_txn(txn2, user->id);
    assert(loaded != NULL);
    
    ORM_Commit(txn2);
}
```

## Integration Points
- **Connection Management**: Uses database connections
- **Query Execution**: Wraps queries in transactions
- **Error Handling**: Triggers rollback on errors
- **Logging**: Records transaction events
- **Performance**: Monitors transaction duration

## Success Criteria
- [ ] ACID properties maintained
- [ ] Nested transactions work via savepoints
- [ ] Isolation levels properly enforced
- [ ] Automatic rollback on errors
- [ ] No transaction leaks
- [ ] Deadlock detection/prevention
- [ ] < 5ms transaction overhead

## Example Usage
```c
// Simple transaction
ORM_Transaction* txn = ORM_BeginTransaction(conn);
if (txn) {
    User* user = User_create(.name = "John");
    Post* post = Post_create(.author_id = user->id, .title = "Hello");
    
    if (user && post) {
        ORM_Commit(txn);
    } else {
        ORM_Rollback(txn);
    }
}

// Using transaction macro
ORM_WITH_TRANSACTION(conn, {
    User* user = User_create(.name = "Jane");
    if (!user) __success = false;
});

// Nested transactions
ORM_Transaction* outer = ORM_BeginTransaction(conn);
process_order(order);

ORM_Transaction* inner = ORM_BeginNestedTransaction(outer);
if (!charge_payment(payment)) {
    ORM_RollbackNested(inner);  // Only rollback payment
} else {
    ORM_CommitNested(inner);
}

ORM_Commit(outer);  // Commit the order

// Retry on deadlock
int result = ORM_RetryTransaction(conn, 
    transfer_funds_operation, 
    3  // max retries
);
```

## Files to Create
- `src/core/transaction.h` - Transaction interface
- `src/core/transaction.c` - Core implementation
- `src/core/savepoint.c` - Savepoint support
- `src/core/isolation.c` - Isolation levels
- `src/core/distributed.c` - 2PC support
- `tests/test_transaction.c` - Transaction tests
