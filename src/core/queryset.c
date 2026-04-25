/* QuerySet Implementation
 * 
 * Core QuerySet operations for building and executing queries.
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "queryset.h"

ORM_QuerySet* ORM_QuerySetNew(ORM_ModelMeta* model) {
    if (!model) {
        return NULL;
    }
    
    ORM_QuerySet* qs = (ORM_QuerySet*)malloc(sizeof(ORM_QuerySet));
    if (!qs) {
        return NULL;
    }
    
    qs->model = model;
    qs->filter_tree = NULL;
    qs->order_by_fields = NULL;
    qs->order_by_count = 0;
    qs->limit_value = -1;
    qs->offset_value = 0;
    qs->distinct = false;
    qs->evaluated = false;
    qs->cached_result = NULL;
    qs->cached_sql = NULL;
    qs->arena = NULL;
    
    return qs;
}

void ORM_QuerySetFree(ORM_QuerySet* qs) {
    if (!qs) {
        return;
    }
    
    if (qs->filter_tree) {
        ORM_FreeExpression(qs->filter_tree);
    }
    
    if (qs->order_by_fields) {
        for (int i = 0; i < qs->order_by_count; i++) {
            free(qs->order_by_fields[i]);
        }
        free(qs->order_by_fields);
    }
    
    free(qs);
}

ORM_QuerySet* ORM_QuerySetClone(ORM_QuerySet* qs) {
    if (!qs) {
        return NULL;
    }
    
    ORM_QuerySet* clone = ORM_QuerySetNew(qs->model);
    if (!clone) {
        return NULL;
    }
    
    // Clone filter tree (would need a deep copy function)
    // For now, we'll share the filter tree pointer (not ideal in production)
    clone->filter_tree = qs->filter_tree;
    
    // Clone order by
    if (qs->order_by_count > 0) {
        clone->order_by_fields = (char**)malloc(sizeof(char*) * qs->order_by_count);
        clone->order_by_count = qs->order_by_count;
        for (int i = 0; i < qs->order_by_count; i++) {
            clone->order_by_fields[i] = strdup(qs->order_by_fields[i]);
        }
    }
    
    clone->limit_value = qs->limit_value;
    clone->offset_value = qs->offset_value;
    clone->distinct = qs->distinct;
    
    return clone;
}

ORM_QuerySet* ORM_Filter(ORM_QuerySet* qs, const char* field, const char* op, void* value) {
    if (!qs || !field) {
        return qs;
    }
    
    // We'll default to INT type - in real usage this would be inferred from model
    ORM_QueryNode* new_filter = ORM_Leaf(field, op, value, ORM_TYPE_INT);
    if (!new_filter) {
        return qs;
    }
    
    if (qs->filter_tree) {
        qs->filter_tree = ORM_And(qs->filter_tree, new_filter);
    } else {
        qs->filter_tree = new_filter;
    }
    
    return qs;
}

ORM_QuerySet* ORM_Exclude(ORM_QuerySet* qs, const char* field, const char* op, void* value) {
    if (!qs || !field) {
        return qs;
    }
    
    ORM_QueryNode* new_filter = ORM_Leaf(field, op, value, ORM_TYPE_INT);
    if (!new_filter) {
        return qs;
    }
    
    // Exclude means NOT the condition
    ORM_QueryNode* negated = ORM_Not(new_filter);
    
    if (qs->filter_tree) {
        qs->filter_tree = ORM_And(qs->filter_tree, negated);
    } else {
        qs->filter_tree = negated;
    }
    
    return qs;
}

ORM_QuerySet* ORM_OrderBy(ORM_QuerySet* qs, const char* field) {
    if (!qs || !field) {
        return qs;
    }
    
    // Add to order by list
    char** new_fields = (char**)realloc(qs->order_by_fields, 
                                         sizeof(char*) * (qs->order_by_count + 1));
    if (!new_fields) {
        return qs;
    }
    
    qs->order_by_fields = new_fields;
    qs->order_by_fields[qs->order_by_count] = strdup(field);
    qs->order_by_count++;
    
    return qs;
}

ORM_QuerySet* ORM_Limit(ORM_QuerySet* qs, int limit) {
    if (!qs) {
        return qs;
    }
    
    qs->limit_value = limit;
    return qs;
}

ORM_QuerySet* ORM_Offset(ORM_QuerySet* qs, int offset) {
    if (!qs) {
        return qs;
    }
    
    qs->offset_value = offset;
    return qs;
}

ORM_QuerySet* ORM_Distinct(ORM_QuerySet* qs) {
    if (!qs) {
        return qs;
    }
    
    qs->distinct = true;
    return qs;
}

char* ORM_BuildSQL(ORM_QuerySet* qs) {
    if (!qs || !qs->model) {
        return NULL;
    }
    
    ORM_StringBuffer* sb = ORM_StringBufferNew(512);
    if (!sb) {
        return NULL;
    }
    
    // SELECT clause
    ORM_StringBufferAppend(sb, "SELECT ");
    if (qs->distinct) {
        ORM_StringBufferAppend(sb, "DISTINCT ");
    }
    
    // Field list
    for (int i = 0; i < qs->model->field_count; i++) {
        if (i > 0) {
            ORM_StringBufferAppend(sb, ", ");
        }
        ORM_StringBufferAppend(sb, qs->model->fields[i].column_name);
    }
    
    // FROM clause
    ORM_StringBufferAppendF(sb, " FROM %s", qs->model->table_name);
    
    // WHERE clause
    if (qs->filter_tree) {
        ORM_StringBufferAppend(sb, " WHERE ");
        char* where_sql = ORM_ExpressionToSQL(qs->filter_tree);
        ORM_StringBufferAppend(sb, where_sql);
        free(where_sql);
    }
    
    // ORDER BY clause
    if (qs->order_by_count > 0) {
        ORM_StringBufferAppend(sb, " ORDER BY ");
        for (int i = 0; i < qs->order_by_count; i++) {
            if (i > 0) {
                ORM_StringBufferAppend(sb, ", ");
            }
            
            // Check for descending order (field starts with '-')
            if (qs->order_by_fields[i][0] == '-') {
                ORM_StringBufferAppendF(sb, "%s DESC", qs->order_by_fields[i] + 1);
            } else {
                ORM_StringBufferAppend(sb, qs->order_by_fields[i]);
            }
        }
    }
    
    // LIMIT clause
    if (qs->limit_value > 0) {
        ORM_StringBufferAppendF(sb, " LIMIT %d", qs->limit_value);
    }
    
    // OFFSET clause
    if (qs->offset_value > 0) {
        ORM_StringBufferAppendF(sb, " OFFSET %d", qs->offset_value);
    }
    
    return ORM_StringBufferRelease(sb);
}

ORM_ResultSet* ORM_Execute(ORM_QuerySet* qs) {
    if (!qs) {
        return NULL;
    }
    
    // Build SQL
    char* sql = ORM_BuildSQL(qs);
    if (!sql) {
        return NULL;
    }
    
    // Create result set
    ORM_ResultSet* rs = (ORM_ResultSet*)malloc(sizeof(ORM_ResultSet));
    if (!rs) {
        free(sql);
        return NULL;
    }
    
    rs->records = NULL;
    rs->count = 0;
    rs->capacity = 0;
    rs->current_index = 0;
    rs->model = qs->model;
    
    // Actual execution would happen in the backend (Task 06)
    // For now, just return the empty result set
    free(sql);
    
    return rs;
}

void ORM_ResultSetFree(ORM_ResultSet* rs) {
    if (!rs) {
        return;
    }
    
    if (rs->records) {
        for (int i = 0; i < rs->count; i++) {
            if (rs->records[i]) {
                free(rs->records[i]);
            }
        }
        free(rs->records);
    }
    
    free(rs);
}

size_t ORM_Count(ORM_QuerySet* qs) {
    if (!qs || !qs->model) {
        return 0;
    }
    
    ORM_StringBuffer* sb = ORM_StringBufferNew(256);
    if (!sb) {
        return 0;
    }
    
    // SELECT COUNT(*) FROM table WHERE ...
    ORM_StringBufferAppendF(sb, "SELECT COUNT(*) FROM %s", qs->model->table_name);
    
    if (qs->filter_tree) {
        ORM_StringBufferAppend(sb, " WHERE ");
        char* where_sql = ORM_ExpressionToSQL(qs->filter_tree);
        ORM_StringBufferAppend(sb, where_sql);
        free(where_sql);
    }
    
    char* sql = ORM_StringBufferRelease(sb);
    
    // Actual execution would happen in backend
    // For now, just free and return 0
    free(sql);
    return 0;
}

int ORM_Exists(ORM_QuerySet* qs) {
    if (!qs) {
        return 0;
    }
    
    // EXISTS is typically implemented as COUNT > 0 or LIMIT 1
    return ORM_Count(qs) > 0;
}
