/* Expression Tree Implementation
 * 
 * Builds and manipulates expression trees for complex queries.
 */

#define _GNU_SOURCE
#include "queryset.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

ORM_QueryNode* ORM_Leaf(const char* field, const char* op, void* value, ORM_FieldType type) {
    if (!field) {
        return NULL;
    }
    
    ORM_QueryNode* node = (ORM_QueryNode*)malloc(sizeof(ORM_QueryNode));
    if (!node) {
        return NULL;
    }
    
    node->type = EXPR_LEAF;
    node->left = NULL;
    node->right = NULL;
    
    // Copy field name
    node->condition.field = strdup(field);
    node->condition.op = ORM_ParseOperator(op);
    node->condition.value_type = type;
    
    // Copy value based on type
    if (value) {
        switch (type) {
            case ORM_TYPE_INT:
                node->condition.value.int_val = *(int*)value;
                break;
            case ORM_TYPE_BIGINT:
                node->condition.value.bigint_val = *(int64_t*)value;
                break;
            case ORM_TYPE_FLOAT:
                node->condition.value.float_val = *(float*)value;
                break;
            case ORM_TYPE_DOUBLE:
                node->condition.value.double_val = *(double*)value;
                break;
            case ORM_TYPE_BOOLEAN:
                node->condition.value.bool_val = *(int*)value;
                break;
            case ORM_TYPE_STRING:
            case ORM_TYPE_TEXT:
                node->condition.value.string_val = strdup((char*)value);
                break;
            case ORM_TYPE_DATETIME:
            case ORM_TYPE_DATE:
            case ORM_TYPE_TIME:
                node->condition.value.datetime_val = *(int64_t*)value;
                break;
            default:
                node->condition.value.int_val = 0;
                break;
        }
    }
    
    return node;
}

ORM_QueryNode* ORM_And(ORM_QueryNode* left, ORM_QueryNode* right) {
    if (!left || !right) {
        return left ? left : right;
    }
    
    ORM_QueryNode* node = (ORM_QueryNode*)malloc(sizeof(ORM_QueryNode));
    if (!node) {
        return NULL;
    }
    
    node->type = EXPR_AND;
    node->left = left;
    node->right = right;
    memset(&node->condition, 0, sizeof(node->condition));
    
    return node;
}

ORM_QueryNode* ORM_Or(ORM_QueryNode* left, ORM_QueryNode* right) {
    if (!left || !right) {
        return left ? left : right;
    }
    
    ORM_QueryNode* node = (ORM_QueryNode*)malloc(sizeof(ORM_QueryNode));
    if (!node) {
        return NULL;
    }
    
    node->type = EXPR_OR;
    node->left = left;
    node->right = right;
    memset(&node->condition, 0, sizeof(node->condition));
    
    return node;
}

ORM_QueryNode* ORM_Not(ORM_QueryNode* expr) {
    if (!expr) {
        return NULL;
    }
    
    ORM_QueryNode* node = (ORM_QueryNode*)malloc(sizeof(ORM_QueryNode));
    if (!node) {
        return NULL;
    }
    
    node->type = EXPR_NOT;
    node->left = expr;
    node->right = NULL;
    memset(&node->condition, 0, sizeof(node->condition));
    
    return node;
}

void ORM_FreeExpression(ORM_QueryNode* node) {
    if (!node) {
        return;
    }
    
    // Free children first
    if (node->left) {
        ORM_FreeExpression(node->left);
    }
    if (node->right) {
        ORM_FreeExpression(node->right);
    }
    
    // Free leaf data
    if (node->type == EXPR_LEAF) {
        if (node->condition.field) {
            free(node->condition.field);
        }
        if (node->condition.value_type == ORM_TYPE_STRING || 
            node->condition.value_type == ORM_TYPE_TEXT) {
            if (node->condition.value.string_val) {
                free(node->condition.value.string_val);
            }
        }
    }
    
    free(node);
}

char* ORM_ExpressionToSQL(ORM_QueryNode* node) {
    if (!node) {
        return strdup("1=1");
    }
    
    ORM_StringBuffer* sb = ORM_StringBufferNew(256);
    if (!sb) {
        return NULL;
    }
    
    switch (node->type) {
        case EXPR_AND: {
            ORM_StringBufferAppend(sb, "(");
            char* left_sql = ORM_ExpressionToSQL(node->left);
            ORM_StringBufferAppend(sb, left_sql);
            free(left_sql);
            ORM_StringBufferAppend(sb, " AND ");
            char* right_sql = ORM_ExpressionToSQL(node->right);
            ORM_StringBufferAppend(sb, right_sql);
            free(right_sql);
            ORM_StringBufferAppend(sb, ")");
            break;
        }
        
        case EXPR_OR: {
            ORM_StringBufferAppend(sb, "(");
            char* left_sql = ORM_ExpressionToSQL(node->left);
            ORM_StringBufferAppend(sb, left_sql);
            free(left_sql);
            ORM_StringBufferAppend(sb, " OR ");
            char* right_sql = ORM_ExpressionToSQL(node->right);
            ORM_StringBufferAppend(sb, right_sql);
            free(right_sql);
            ORM_StringBufferAppend(sb, ")");
            break;
        }
        
        case EXPR_NOT: {
            ORM_StringBufferAppend(sb, "NOT (");
            char* expr_sql = ORM_ExpressionToSQL(node->left);
            ORM_StringBufferAppend(sb, expr_sql);
            free(expr_sql);
            ORM_StringBufferAppend(sb, ")");
            break;
        }
        
        case EXPR_LEAF: {
            // Field name
            ORM_StringBufferAppend(sb, node->condition.field);
            ORM_StringBufferAppend(sb, " ");
            
            // Operator
            const char* op_sql = ORM_OperatorToSQL(node->condition.op);
            ORM_StringBufferAppend(sb, op_sql);
            
            // Value (if needed)
            if (node->condition.op != OP_IS_NULL && node->condition.op != OP_IS_NOT_NULL) {
                ORM_StringBufferAppend(sb, " ");
                
                // Convert value to SQL
                char value_buf[256];
                switch (node->condition.value_type) {
                    case ORM_TYPE_INT:
                        snprintf(value_buf, sizeof(value_buf), "%d", node->condition.value.int_val);
                        ORM_StringBufferAppend(sb, value_buf);
                        break;
                    case ORM_TYPE_BIGINT:
                        snprintf(value_buf, sizeof(value_buf), "%lld", (long long)node->condition.value.bigint_val);
                        ORM_StringBufferAppend(sb, value_buf);
                        break;
                    case ORM_TYPE_FLOAT:
                        snprintf(value_buf, sizeof(value_buf), "%f", node->condition.value.float_val);
                        ORM_StringBufferAppend(sb, value_buf);
                        break;
                    case ORM_TYPE_DOUBLE:
                        snprintf(value_buf, sizeof(value_buf), "%f", node->condition.value.double_val);
                        ORM_StringBufferAppend(sb, value_buf);
                        break;
                    case ORM_TYPE_BOOLEAN:
                        ORM_StringBufferAppend(sb, node->condition.value.bool_val ? "1" : "0");
                        break;
                    case ORM_TYPE_STRING:
                    case ORM_TYPE_TEXT:
                        if (node->condition.op == OP_CONTAINS) {
                            ORM_StringBufferAppendF(sb, "'%%%s%%'", node->condition.value.string_val);
                        } else if (node->condition.op == OP_STARTSWITH) {
                            ORM_StringBufferAppendF(sb, "'%s%%'", node->condition.value.string_val);
                        } else if (node->condition.op == OP_ENDSWITH) {
                            ORM_StringBufferAppendF(sb, "'%%%s'", node->condition.value.string_val);
                        } else {
                            ORM_StringBufferAppendF(sb, "'%s'", node->condition.value.string_val);
                        }
                        break;
                    default:
                        ORM_StringBufferAppend(sb, "NULL");
                        break;
                }
            }
            break;
        }
    }
    
    return ORM_StringBufferRelease(sb);
}
