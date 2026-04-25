/* SQL Escaping and Safety
 * 
 * String escaping and identifier quoting to prevent SQL injection.
 */

#define _GNU_SOURCE
#include "sql_builder.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// ============================================================================
// String Escaping
// ============================================================================

char* ORM_EscapeSQLString(const char* str, ORM_Dialect dialect) {
    if (!str) {
        return NULL;
    }
    
    size_t len = strlen(str);
    // Worst case: every character needs escaping
    char* escaped = (char*)malloc(len * 2 + 1);
    if (!escaped) {
        return NULL;
    }
    
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        
        switch (dialect) {
            case ORM_DIALECT_MYSQL:
                // MySQL escape sequences
                switch (c) {
                    case '\0': escaped[j++] = '\\'; escaped[j++] = '0'; break;
                    case '\n': escaped[j++] = '\\'; escaped[j++] = 'n'; break;
                    case '\r': escaped[j++] = '\\'; escaped[j++] = 'r'; break;
                    case '\\': escaped[j++] = '\\'; escaped[j++] = '\\'; break;
                    case '\'': escaped[j++] = '\\'; escaped[j++] = '\''; break;
                    case '"':  escaped[j++] = '\\'; escaped[j++] = '"'; break;
                    case '\032': escaped[j++] = '\\'; escaped[j++] = 'Z'; break;
                    default: escaped[j++] = c; break;
                }
                break;
                
            case ORM_DIALECT_POSTGRESQL:
            case ORM_DIALECT_SQLITE:
            case ORM_DIALECT_MSSQL:
            default:
                // Standard SQL: escape single quotes by doubling
                if (c == '\'') {
                    escaped[j++] = '\'';
                    escaped[j++] = '\'';
                } else {
                    escaped[j++] = c;
                }
                break;
        }
    }
    
    escaped[j] = '\0';
    return escaped;
}

char* ORM_UnescapeSQLString(const char* str, ORM_Dialect dialect) {
    if (!str) {
        return NULL;
    }
    
    size_t len = strlen(str);
    char* unescaped = (char*)malloc(len + 1);
    if (!unescaped) {
        return NULL;
    }
    
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\'' && i + 1 < len && str[i+1] == '\'') {
            // Doubled single quote -> single quote
            unescaped[j++] = '\'';
            i++; // Skip next quote
        } else if (dialect == ORM_DIALECT_MYSQL && str[i] == '\\' && i + 1 < len) {
            // MySQL escape sequences
            i++;
            switch (str[i]) {
                case '0': unescaped[j++] = '\0'; break;
                case 'n': unescaped[j++] = '\n'; break;
                case 'r': unescaped[j++] = '\r'; break;
                case '\\': unescaped[j++] = '\\'; break;
                case '\'': unescaped[j++] = '\''; break;
                case '"': unescaped[j++] = '"'; break;
                case 'Z': unescaped[j++] = '\032'; break;
                default: unescaped[j++] = str[i]; break;
            }
        } else {
            unescaped[j++] = str[i];
        }
    }
    
    unescaped[j] = '\0';
    return unescaped;
}

// ============================================================================
// Identifier Quoting
// ============================================================================

char* ORM_QuoteIdentifier(const char* identifier, ORM_Dialect dialect) {
    if (!identifier) {
        return NULL;
    }
    
    size_t len = strlen(identifier);
    char open_quote, close_quote;
    
    switch (dialect) {
        case ORM_DIALECT_MYSQL:
            open_quote = '`';
            close_quote = '`';
            break;
        case ORM_DIALECT_MSSQL:
            open_quote = '[';
            close_quote = ']';
            break;
        case ORM_DIALECT_POSTGRESQL:
        case ORM_DIALECT_SQLITE:
        default:
            open_quote = '"';
            close_quote = '"';
            break;
    }
    
    // Allocate: open + identifier + close + null
    char* quoted = (char*)malloc(len + 3);
    if (!quoted) {
        return NULL;
    }
    
    quoted[0] = open_quote;
    memcpy(quoted + 1, identifier, len);
    quoted[len + 1] = close_quote;
    quoted[len + 2] = '\0';
    
    return quoted;
}

// ============================================================================
// Identifier Validation
// ============================================================================

bool ORM_ValidateIdentifier(const char* identifier) {
    if (!identifier || !*identifier) {
        return false;
    }
    
    // First character must be letter or underscore
    if (!isalpha(identifier[0]) && identifier[0] != '_') {
        return false;
    }
    
    // Rest must be alphanumeric or underscore
    for (size_t i = 1; identifier[i]; i++) {
        if (!isalnum(identifier[i]) && identifier[i] != '_') {
            return false;
        }
    }
    
    // Check length (reasonable limit)
    if (strlen(identifier) > 64) {
        return false;
    }
    
    return true;
}

// ============================================================================
// WHERE Clause Builder
// ============================================================================

static void BuildExpressionSQL(ORM_SQLBuilder* builder, ORM_QueryNode* node) {
    if (!node) {
        return;
    }
    
    switch (node->type) {
        case EXPR_AND:
            ORM_StringBufferAppend(builder->where_clause, "(");
            BuildExpressionSQL(builder, node->left);
            ORM_StringBufferAppend(builder->where_clause, " AND ");
            BuildExpressionSQL(builder, node->right);
            ORM_StringBufferAppend(builder->where_clause, ")");
            break;
            
        case EXPR_OR:
            ORM_StringBufferAppend(builder->where_clause, "(");
            BuildExpressionSQL(builder, node->left);
            ORM_StringBufferAppend(builder->where_clause, " OR ");
            BuildExpressionSQL(builder, node->right);
            ORM_StringBufferAppend(builder->where_clause, ")");
            break;
            
        case EXPR_NOT:
            ORM_StringBufferAppend(builder->where_clause, "NOT (");
            BuildExpressionSQL(builder, node->left);
            ORM_StringBufferAppend(builder->where_clause, ")");
            break;
            
        case EXPR_LEAF: {
            // Field name
            char* quoted = ORM_QuoteIdentifier(node->condition.field, builder->dialect);
            ORM_StringBufferAppend(builder->where_clause, quoted);
            free(quoted);
            
            ORM_StringBufferAppend(builder->where_clause, " ");
            
            // Operator
            const char* op_sql = ORM_OperatorToSQL(node->condition.op);
            ORM_StringBufferAppend(builder->where_clause, op_sql);
            
            // Value (if needed)
            if (node->condition.op != OP_IS_NULL && node->condition.op != OP_IS_NOT_NULL) {
                ORM_StringBufferAppend(builder->where_clause, " ");
                
                // Add as parameter
                int param_idx = ORM_AddParameter(builder, &node->condition.value, 
                                                 node->condition.value_type, 0);
                ORM_StringBufferAppend(builder->where_clause, ORM_GetPlaceholder(builder, param_idx));
            }
            break;
        }
    }
}

void ORM_BuildWhereClause(ORM_SQLBuilder* builder, ORM_QueryNode* filter_tree) {
    if (!builder || !filter_tree) {
        return;
    }
    
    ORM_StringBufferClear(builder->where_clause);
    BuildExpressionSQL(builder, filter_tree);
}

// ============================================================================
// ORDER BY Clause Builder
// ============================================================================

void ORM_BuildOrderClause(ORM_SQLBuilder* builder, char** fields, int count) {
    if (!builder || !fields || count <= 0) {
        return;
    }
    
    ORM_StringBufferClear(builder->order_clause);
    ORM_StringBufferAppend(builder->order_clause, "ORDER BY ");
    
    for (int i = 0; i < count; i++) {
        if (i > 0) {
            ORM_StringBufferAppend(builder->order_clause, ", ");
        }
        
        // Check for descending order (field starts with '-')
        const char* field = fields[i];
        bool descending = (field[0] == '-');
        if (descending) {
            field++; // Skip '-'
        }
        
        char* quoted = ORM_QuoteIdentifier(field, builder->dialect);
        ORM_StringBufferAppend(builder->order_clause, quoted);
        free(quoted);
        
        if (descending) {
            ORM_StringBufferAppend(builder->order_clause, " DESC");
        }
    }
}

// ============================================================================
// LIMIT/OFFSET Clause Builder
// ============================================================================

void ORM_BuildLimitClause(ORM_SQLBuilder* builder, int limit, int offset) {
    if (!builder) {
        return;
    }
    
    if (builder->dialect == ORM_DIALECT_MYSQL && offset > 0 && limit > 0) {
        // MySQL: LIMIT offset, limit
        ORM_StringBufferAppendF(builder->sql, " LIMIT %d, %d", offset, limit);
    } else {
        // Standard SQL: LIMIT x OFFSET y
        if (limit > 0) {
            ORM_StringBufferAppendF(builder->sql, " LIMIT %d", limit);
        }
        if (offset > 0) {
            ORM_StringBufferAppendF(builder->sql, " OFFSET %d", offset);
        }
    }
}
