/* Query Builder System
 * 
 * Provides a fluent API for building database queries with method chaining,
 * lazy evaluation, and complex filter expressions.
 */

#ifndef ORM_QUERYSET_H
#define ORM_QUERYSET_H

#include "model.h"
#include "field_types.h"
#include "../utils/memory.h"
#include <stdbool.h>

// Forward declarations
typedef struct ORM_QuerySet ORM_QuerySet;
typedef struct ORM_QueryNode ORM_QueryNode;
typedef struct ORM_ResultSet ORM_ResultSet;

// ============================================================================
// Filter Operators
// ============================================================================

typedef enum {
    OP_EQ,          // = (equals)
    OP_NE,          // != (not equals)
    OP_LT,          // < (less than)
    OP_LE,          // <= (less than or equal)
    OP_GT,          // > (greater than)
    OP_GE,          // >= (greater than or equal)
    OP_IN,          // IN (value in list)
    OP_NOT_IN,      // NOT IN (value not in list)
    OP_LIKE,        // LIKE (pattern match)
    OP_ILIKE,       // ILIKE (case-insensitive LIKE)
    OP_CONTAINS,    // LIKE %value%
    OP_STARTSWITH,  // LIKE value%
    OP_ENDSWITH,    // LIKE %value
    OP_REGEX,       // REGEXP
    OP_IS_NULL,     // IS NULL
    OP_IS_NOT_NULL  // IS NOT NULL
} ORM_Operator;

/* Parse operator from string (e.g., ">=", "contains", "in") */
ORM_Operator ORM_ParseOperator(const char* op_str);

/* Convert operator to SQL string */
const char* ORM_OperatorToSQL(ORM_Operator op);

// ============================================================================
// Expression Tree
// ============================================================================

typedef enum {
    EXPR_AND,   // Logical AND
    EXPR_OR,    // Logical OR
    EXPR_NOT,   // Logical NOT
    EXPR_LEAF   // Leaf condition
} ORM_ExprType;

struct ORM_QueryNode {
    ORM_ExprType type;
    struct ORM_QueryNode* left;
    struct ORM_QueryNode* right;
    
    // Leaf node data
    struct {
        char* field;
        ORM_Operator op;
        ORM_FieldValue value;
        ORM_FieldType value_type;
    } condition;
};

/* Create expression nodes */
ORM_QueryNode* ORM_Leaf(const char* field, const char* op, void* value, ORM_FieldType type);
ORM_QueryNode* ORM_And(ORM_QueryNode* left, ORM_QueryNode* right);
ORM_QueryNode* ORM_Or(ORM_QueryNode* left, ORM_QueryNode* right);
ORM_QueryNode* ORM_Not(ORM_QueryNode* expr);

/* Free expression tree */
void ORM_FreeExpression(ORM_QueryNode* node);

// ============================================================================
// QuerySet Structure
// ============================================================================

struct ORM_QuerySet {
    ORM_ModelMeta* model;
    ORM_QueryNode* filter_tree;
    char** order_by_fields;
    int order_by_count;
    int limit_value;
    int offset_value;
    bool distinct;
    
    // Lazy evaluation
    bool evaluated;
    ORM_ResultSet* cached_result;
    char* cached_sql;
    
    // Memory management
    ORM_Arena* arena;  // Temporary allocations
};

/* Create new QuerySet for model */
ORM_QuerySet* ORM_CreateQuerySet(ORM_ModelMeta* model);
ORM_QuerySet* ORM_QuerySetNew(ORM_ModelMeta* model);

/* Clone QuerySet (for chaining) */
ORM_QuerySet* ORM_QuerySetClone(ORM_QuerySet* qs);

/* Free QuerySet */
void ORM_FreeQuerySet(ORM_QuerySet* qs);
void ORM_QuerySetFree(ORM_QuerySet* qs);

/* Build SQL from QuerySet */
char* ORM_BuildSQL(ORM_QuerySet* qs);

// ============================================================================
// Filter Operations
// ============================================================================

/* Add filter condition (chainable) */
ORM_QuerySet* ORM_Filter(ORM_QuerySet* qs, const char* field, const char* op, void* value);

/* Add exclude condition (chainable) */
ORM_QuerySet* ORM_Exclude(ORM_QuerySet* qs, const char* field, const char* op, void* value);

/* Add complex query expression */
ORM_QuerySet* ORM_Q(ORM_QuerySet* qs, ORM_QueryNode* expr);

/* Get all objects (no filter) */
ORM_QuerySet* ORM_All(ORM_QuerySet* qs);

// ============================================================================
// Ordering & Limiting
// ============================================================================

/* Add ordering (prefix with '-' for descending) */
ORM_QuerySet* ORM_OrderBy(ORM_QuerySet* qs, const char* field);

/* Set limit */
ORM_QuerySet* ORM_Limit(ORM_QuerySet* qs, int limit);

/* Set offset */
ORM_QuerySet* ORM_Offset(ORM_QuerySet* qs, int offset);

/* Enable distinct results */
ORM_QuerySet* ORM_Distinct(ORM_QuerySet* qs);

// ============================================================================
// Aggregation
// ============================================================================

typedef enum {
    AGG_COUNT,
    AGG_SUM,
    AGG_AVG,
    AGG_MIN,
    AGG_MAX
} ORM_AggregateType;

/* Generic aggregate */
ORM_FieldValue ORM_Aggregate(ORM_QuerySet* qs, const char* field, const char* func);

/* Count records */
size_t ORM_Count(ORM_QuerySet* qs);

/* Sum numeric field */
ORM_FieldValue ORM_Sum(ORM_QuerySet* qs, const char* field);

/* Average numeric field */
ORM_FieldValue ORM_Avg(ORM_QuerySet* qs, const char* field);

/* Minimum value */
ORM_FieldValue ORM_Min(ORM_QuerySet* qs, const char* field);

/* Maximum value */
ORM_FieldValue ORM_Max(ORM_QuerySet* qs, const char* field);

/* Check if any records exist */
int ORM_Exists(ORM_QuerySet* qs);

// ============================================================================
// Query Execution
// ============================================================================

/* Result set structure */
struct ORM_ResultSet {
    void** records;      // Array of model instances
    int count;          // Number of records
    int capacity;       // Allocated capacity
    int current_index;  // Iterator position
    ORM_ModelMeta* model;
};

/* Execute query and return results */
ORM_ResultSet* ORM_Execute(ORM_QuerySet* qs);

/* Get first record */
void* ORM_First(ORM_QuerySet* qs);

/* Get last record */
void* ORM_Last(ORM_QuerySet* qs);

/* Get single record (error if multiple) */
void* ORM_Get(ORM_QuerySet* qs);

/* Result set operations */
void* ORM_ResultNext(ORM_ResultSet* rs);
void ORM_ResultReset(ORM_ResultSet* rs);
void ORM_FreeResultSet(ORM_ResultSet* rs);

// ============================================================================
// SQL Generation
// ============================================================================

/* Generate SQL from QuerySet */
char* ORM_QueryToSQL(ORM_QuerySet* qs);

/* Generate WHERE clause from expression tree */
char* ORM_ExpressionToSQL(ORM_QueryNode* node);

// ============================================================================
// Utility Functions
// ============================================================================

/* Check if QuerySet has been evaluated */
bool ORM_IsEvaluated(ORM_QuerySet* qs);

/* Invalidate cache (force re-evaluation) */
void ORM_InvalidateCache(ORM_QuerySet* qs);

/* Get QuerySet description (for debugging) */
char* ORM_QuerySetRepr(ORM_QuerySet* qs);

#endif /* ORM_QUERYSET_H */
