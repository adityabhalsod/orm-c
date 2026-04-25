/* Filter Operators Implementation
 * 
 * Implements operator parsing and SQL conversion.
 */

#include "queryset.h"
#include <string.h>
#include <strings.h>

ORM_Operator ORM_ParseOperator(const char* op_str) {
    if (!op_str) {
        return OP_EQ;  // Default
    }
    
    // Exact matches
    if (strcmp(op_str, "=") == 0 || strcmp(op_str, "eq") == 0) return OP_EQ;
    if (strcmp(op_str, "!=") == 0 || strcmp(op_str, "ne") == 0) return OP_NE;
    if (strcmp(op_str, "<") == 0 || strcmp(op_str, "lt") == 0) return OP_LT;
    if (strcmp(op_str, "<=") == 0 || strcmp(op_str, "le") == 0 || strcmp(op_str, "lte") == 0) return OP_LE;
    if (strcmp(op_str, ">") == 0 || strcmp(op_str, "gt") == 0) return OP_GT;
    if (strcmp(op_str, ">=") == 0 || strcmp(op_str, "ge") == 0 || strcmp(op_str, "gte") == 0) return OP_GE;
    
    // Case-insensitive matches
    if (strcasecmp(op_str, "in") == 0) return OP_IN;
    if (strcasecmp(op_str, "not_in") == 0) return OP_NOT_IN;
    if (strcasecmp(op_str, "like") == 0) return OP_LIKE;
    if (strcasecmp(op_str, "ilike") == 0) return OP_ILIKE;
    if (strcasecmp(op_str, "contains") == 0) return OP_CONTAINS;
    if (strcasecmp(op_str, "startswith") == 0) return OP_STARTSWITH;
    if (strcasecmp(op_str, "endswith") == 0) return OP_ENDSWITH;
    if (strcasecmp(op_str, "regex") == 0 || strcasecmp(op_str, "regexp") == 0) return OP_REGEX;
    if (strcasecmp(op_str, "isnull") == 0 || strcasecmp(op_str, "is_null") == 0) return OP_IS_NULL;
    if (strcasecmp(op_str, "isnotnull") == 0 || strcasecmp(op_str, "is_not_null") == 0) return OP_IS_NOT_NULL;
    
    // Default to equals
    return OP_EQ;
}

const char* ORM_OperatorToSQL(ORM_Operator op) {
    switch (op) {
        case OP_EQ:          return "=";
        case OP_NE:          return "!=";
        case OP_LT:          return "<";
        case OP_LE:          return "<=";
        case OP_GT:          return ">";
        case OP_GE:          return ">=";
        case OP_IN:          return "IN";
        case OP_NOT_IN:      return "NOT IN";
        case OP_LIKE:        return "LIKE";
        case OP_ILIKE:       return "LIKE";  // SQLite doesn't have ILIKE, use LIKE
        case OP_CONTAINS:    return "LIKE";
        case OP_STARTSWITH:  return "LIKE";
        case OP_ENDSWITH:    return "LIKE";
        case OP_REGEX:       return "REGEXP";
        case OP_IS_NULL:     return "IS NULL";
        case OP_IS_NOT_NULL: return "IS NOT NULL";
        default:             return "=";
    }
}
