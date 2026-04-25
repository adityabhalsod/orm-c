# Task 15: Performance Optimization and Caching

## Objective
Implement comprehensive performance optimizations including query caching, result caching, query optimization, indexing hints, and performance monitoring to achieve high throughput and low latency.

## Deliverables
1. Multi-level caching system
2. Query optimization engine
3. Index usage optimization
4. Performance monitoring
5. Batch operation optimizations

## Input Requirements
- Query patterns and frequency
- Cache size constraints
- Performance requirements
- Index definitions
- Monitoring requirements

## Output Specifications
- Cache hit/miss statistics
- Query execution plans
- Performance metrics
- Optimization recommendations
- Bottleneck identification

## Implementation Guide

### 1. Caching Architecture
```c
// Multi-level cache system
typedef struct {
    ORM_QueryCache* query_cache;
    ORM_ResultCache* result_cache;
    ORM_PreparedStmtCache* stmt_cache;
    ORM_MetadataCache* metadata_cache;
    ORM_ConnectionCache* conn_cache;
} ORM_CacheSystem;

// Query cache
typedef struct {
    ORM_HashMap* cache;
    size_t max_size;
    size_t current_size;
    ORM_CacheEviction eviction_policy;
    uint64_t hits;
    uint64_t misses;
    pthread_rwlock_t lock;
} ORM_QueryCache;

// Cache entry
typedef struct {
    char* query_hash;
    ORM_ResultSet* result;
    time_t created_at;
    time_t last_accessed;
    int access_count;
    size_t size;
    bool is_dirty;
} ORM_CacheEntry;

// Cache operations
ORM_CacheSystem* ORM_CreateCacheSystem(ORM_CacheConfig* config);
void ORM_CachePut(ORM_QueryCache* cache, const char* key, void* value);
void* ORM_CacheGet(ORM_QueryCache* cache, const char* key);
void ORM_CacheInvalidate(ORM_QueryCache* cache, const char* pattern);
void ORM_CacheClear(ORM_QueryCache* cache);

// Eviction policies
typedef enum {
    EVICT_LRU,    // Least Recently Used
    EVICT_LFU,    // Least Frequently Used
    EVICT_FIFO,   // First In First Out
    EVICT_TTL     // Time To Live
} ORM_CacheEviction;
```

### 2. Query Optimization
```c
// Query optimizer
typedef struct {
    ORM_QueryPlan* (*optimize)(struct ORM_QueryOptimizer*, QuerySet* qs);
    ORM_IndexHint* (*suggest_indexes)(struct ORM_QueryOptimizer*, QuerySet* qs);
    bool (*should_use_index)(struct ORM_QueryOptimizer*, const char* field);
} ORM_QueryOptimizer;

// Query execution plan
typedef struct {
    char* operation;
    double estimated_cost;
    int estimated_rows;
    char* access_type;
    char* possible_indexes;
    char* used_index;
    char* extra;
    struct ORM_QueryPlan** children;
    int child_count;
} ORM_QueryPlan;

// Optimization strategies
ORM_QueryPlan* ORM_AnalyzeQuery(QuerySet* qs);
void ORM_OptimizeQuery(QuerySet* qs);
char* ORM_ExplainQuery(QuerySet* qs);

// Cost-based optimization
double ORM_EstimateQueryCost(QuerySet* qs);
int ORM_EstimateRowCount(QuerySet* qs);
```

### 3. Index Management
```c
// Index hints
typedef struct {
    char* index_name;
    enum { USE_INDEX, FORCE_INDEX, IGNORE_INDEX } hint_type;
} ORM_IndexHint;

// Index statistics
typedef struct {
    char* index_name;
    char** columns;
    int column_count;
    bool is_unique;
    uint64_t cardinality;
    double selectivity;
    uint64_t usage_count;
} ORM_IndexStats;

// Index operations
void ORM_UseIndex(QuerySet* qs, const char* index_name);
void ORM_ForceIndex(QuerySet* qs, const char* index_name);
void ORM_IgnoreIndex(QuerySet* qs, const char* index_name);
ORM_IndexStats* ORM_GetIndexStats(const char* table);
void ORM_UpdateIndexStats(const char* table);

// Automatic index creation
void ORM_SuggestIndexes(ORM_ModelMeta* model);
void ORM_CreateMissingIndexes(ORM_ModelMeta* model);
```

### 4. Batch Operations
```c
// Batch insert optimization
typedef struct {
    void** objects;
    int object_count;
    int batch_size;
    bool use_transaction;
    bool ignore_conflicts;
} ORM_BulkInsertOptions;

int ORM_BulkInsert(
    ORM_ModelMeta* model,
    void** objects,
    int count,
    ORM_BulkInsertOptions* options
);

// Batch update
int ORM_BulkUpdate(
    ORM_ModelMeta* model,
    void** objects,
    int count,
    const char** fields,
    int field_count
);

// Batch delete
int ORM_BulkDelete(
    ORM_ModelMeta* model,
    QuerySet* conditions
);

// Chunk processing for large datasets
typedef void (*ORM_ChunkProcessor)(void** chunk, int size, void* context);

void ORM_ProcessInChunks(
    QuerySet* qs,
    int chunk_size,
    ORM_ChunkProcessor processor,
    void* context
);
```

### 5. Performance Monitoring
```c
// Performance metrics
typedef struct {
    uint64_t query_count;
    double avg_query_time;
    double max_query_time;
    double min_query_time;
    uint64_t cache_hits;
    uint64_t cache_misses;
    double cache_hit_ratio;
    uint64_t slow_queries;
    size_t memory_usage;
    int active_connections;
} ORM_PerfMetrics;

// Query profiler
typedef struct {
    bool enabled;
    double slow_query_threshold;
    ORM_CircularBuffer* query_log;
    void (*on_slow_query)(const char* query, double duration);
} ORM_QueryProfiler;

// Monitoring operations
void ORM_EnableProfiling(void);
void ORM_DisableProfiling(void);
ORM_PerfMetrics* ORM_GetMetrics(void);
void ORM_ResetMetrics(void);
char* ORM_GetSlowQueryLog(int limit);

// Performance hooks
void ORM_SetQueryStartHook(void (*hook)(const char* query));
void ORM_SetQueryEndHook(void (*hook)(const char* query, double duration));
```

### 6. Memory Optimization
```c
// Memory-efficient result streaming
typedef struct {
    ORM_ResultSet* (*next_batch)(struct ORM_StreamingCursor*);
    void (*close)(struct ORM_StreamingCursor*);
    int batch_size;
    bool has_more;
} ORM_StreamingCursor;

ORM_StreamingCursor* ORM_CreateStreamingCursor(
    QuerySet* qs,
    int batch_size
);

// Object pooling for model instances
typedef struct {
    ORM_Stack* free_objects;
    ORM_ModelMeta* model;
    int pool_size;
    int allocated;
} ORM_ObjectPool;

void* ORM_ObjectPoolGet(ORM_ObjectPool* pool);
void ORM_ObjectPoolReturn(ORM_ObjectPool* pool, void* object);

// String interning
typedef struct {
    ORM_HashMap* strings;
    size_t total_size;
    uint64_t lookups;
    uint64_t hits;
} ORM_StringIntern;

const char* ORM_InternString(const char* str);
void ORM_ClearInternedStrings(void);
```

## Testing Requirements

### Performance Tests
```c
void test_cache_performance() {
    ORM_CacheConfig config = {
        .max_size = 1024 * 1024 * 10,  // 10MB
        .eviction = EVICT_LRU
    };
    ORM_QueryCache* cache = ORM_CreateQueryCache(&config);
    
    // Benchmark cache operations
    clock_t start = clock();
    for (int i = 0; i < 10000; i++) {
        char key[32];
        sprintf(key, "query_%d", i % 100);
        
        void* result = ORM_CacheGet(cache, key);
        if (!result) {
            result = execute_query(key);
            ORM_CachePut(cache, key, result);
        }
    }
    clock_t end = clock();
    
    double hit_ratio = (double)cache->hits / (cache->hits + cache->misses);
    assert(hit_ratio > 0.9);  // 90% hit rate
    
    double time_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000;
    assert(time_ms < 100);  // Less than 100ms for 10k operations
}

void test_bulk_operations() {
    // Create test data
    User* users[10000];
    for (int i = 0; i < 10000; i++) {
        users[i] = create_test_user(i);
    }
    
    // Benchmark bulk insert
    clock_t start = clock();
    ORM_BulkInsert(&User_meta, (void**)users, 10000,
        &(ORM_BulkInsertOptions){
            .batch_size = 1000,
            .use_transaction = true
        }
    );
    clock_t end = clock();
    
    double time_s = (double)(end - start) / CLOCKS_PER_SEC;
    assert(time_s < 1.0);  // Less than 1 second for 10k inserts
}

void test_query_optimization() {
    // Complex query
    QuerySet* qs = User_objects()
        ->filter("age", ">", &(int){18})
        ->filter("country", "=", "USA")
        ->order_by("-created_at")
        ->limit(100);
    
    // Analyze query
    ORM_QueryPlan* plan = ORM_AnalyzeQuery(qs);
    assert(plan != NULL);
    
    // Should suggest index on age and country
    ORM_IndexHint* hints = ORM_SuggestIndexes(qs);
    assert(hints != NULL);
    
    // Execute with optimization
    ORM_OptimizeQuery(qs);
    clock_t start = clock();
    ORM_ResultSet* results = qs->execute(qs);
    clock_t end = clock();
    
    double query_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000;
    assert(query_ms < 10);  // Less than 10ms with optimization
}

void test_memory_efficiency() {
    // Large dataset streaming
    QuerySet* qs = Post_objects()->all();  // Millions of records
    
    size_t initial_memory = get_memory_usage();
    
    ORM_StreamingCursor* cursor = ORM_CreateStreamingCursor(qs, 100);
    
    int total_processed = 0;
    while (cursor->has_more) {
        ORM_ResultSet* batch = cursor->next_batch(cursor);
        total_processed += batch->row_count;
        ORM_FreeResultSet(batch);
        
        // Memory should not grow significantly
        size_t current_memory = get_memory_usage();
        assert(current_memory - initial_memory < 1024 * 1024);  // < 1MB growth
    }
    
    cursor->close(cursor);
}
```

### Benchmark Suite
```c
void benchmark_orm_operations() {
    ORM_EnableProfiling();
    
    // Benchmark different operations
    benchmark_single_insert();
    benchmark_bulk_insert();
    benchmark_simple_query();
    benchmark_complex_query();
    benchmark_joins();
    benchmark_aggregations();
    benchmark_transactions();
    
    ORM_PerfMetrics* metrics = ORM_GetMetrics();
    
    printf("Performance Report:\n");
    printf("Total Queries: %lu\n", metrics->query_count);
    printf("Avg Query Time: %.2f ms\n", metrics->avg_query_time);
    printf("Cache Hit Ratio: %.2f%%\n", metrics->cache_hit_ratio * 100);
    printf("Slow Queries: %lu\n", metrics->slow_queries);
    printf("Memory Usage: %.2f MB\n", metrics->memory_usage / 1024.0 / 1024.0);
    
    ORM_DisableProfiling();
}
```

## Integration Points
- **All Components**: Performance affects entire system
- **Query Builder**: Optimizes query construction
- **Database Backends**: Leverages backend-specific optimizations
- **Memory Management**: Coordinates with memory pools
- **Result Sets**: Optimizes result handling

## Success Criteria
- [ ] Query cache hit ratio > 80% for repeated queries
- [ ] Bulk operations 10x faster than individual operations
- [ ] Query optimization reduces execution time by > 50%
- [ ] Memory usage remains constant with streaming
- [ ] No performance degradation under load
- [ ] Profiling overhead < 5%
- [ ] Support for 10,000+ concurrent operations

## Example Usage
```c
// Enable caching
ORM_CacheConfig cache_config = {
    .max_size = 50 * 1024 * 1024,  // 50MB
    .eviction = EVICT_LRU,
    .ttl = 300  // 5 minutes
};
ORM_EnableCache(&cache_config);

// Query with caching
QuerySet* popular_posts = Post_objects()
    ->filter("views", ">", &(int){1000})
    ->order_by("-views")
    ->limit(10)
    ->cache(300);  // Cache for 5 minutes

// Bulk operations
User* users[1000];
// ... populate users ...
ORM_BulkInsert(&User_meta, (void**)users, 1000,
    &(ORM_BulkInsertOptions){
        .batch_size = 100,
        .use_transaction = true,
        .ignore_conflicts = true
    }
);

// Query optimization hints
QuerySet* optimized = User_objects()
    ->use_index("idx_age_country")
    ->filter("age", "BETWEEN", &(int[]){25, 35})
    ->filter("country", "IN", &(char*[]){"USA", "UK", "CA"})
    ->prefetch_related("posts");

// Streaming large results
ORM_StreamingCursor* cursor = ORM_CreateStreamingCursor(
    Post_objects()->all(), 
    100  // Process 100 at a time
);

while (cursor->has_more) {
    ORM_ResultSet* batch = cursor->next_batch(cursor);
    process_batch(batch);
    ORM_FreeResultSet(batch);
}

// Monitor performance
ORM_EnableProfiling();
ORM_SetSlowQueryThreshold(100.0);  // 100ms

// ... run operations ...

ORM_PerfMetrics* metrics = ORM_GetMetrics();
printf("Cache hit ratio: %.2f%%\n", metrics->cache_hit_ratio * 100);
printf("Average query time: %.2f ms\n", metrics->avg_query_time);

// Get slow queries
char* slow_log = ORM_GetSlowQueryLog(10);
printf("Slow queries:\n%s\n", slow_log);
free(slow_log);
```

## Files to Create
- `src/performance/cache.h` - Caching system interface
- `src/performance/cache.c` - Cache implementation
- `src/performance/optimizer.c` - Query optimization
- `src/performance/bulk_ops.c` - Bulk operations
- `src/performance/profiler.c` - Performance profiling
- `src/performance/monitor.c` - Performance monitoring
- `tests/test_performance.c` - Performance tests
- `benchmarks/benchmark.c` - Benchmark suite
