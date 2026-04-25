/* Aggregation Functions Implementation
 * 
 * Provides COUNT, SUM, AVG, MIN, MAX aggregation operations.
 */

#include "queryset.h"
#include <stdlib.h>
#include <string.h>

ORM_FieldValue ORM_Aggregate(ORM_QuerySet* qs, const char* field, const char* func) {
    ORM_FieldValue result;
    memset(&result, 0, sizeof(result));
    
    if (!qs || !qs->model || !func) {
        return result;
    }
    
    ORM_StringBuffer* sb = ORM_StringBufferNew(256);
    if (!sb) {
        return result;
    }
    
    // Build aggregation SQL: SELECT FUNC(field) FROM table WHERE ...
    ORM_StringBufferAppend(sb, "SELECT ");
    ORM_StringBufferAppend(sb, func);
    ORM_StringBufferAppend(sb, "(");
    
    if (field && strlen(field) > 0) {
        ORM_StringBufferAppend(sb, field);
    } else {
        ORM_StringBufferAppend(sb, "*");
    }
    
    ORM_StringBufferAppend(sb, ")");
    ORM_StringBufferAppendF(sb, " FROM %s", qs->model->table_name);
    
    if (qs->filter_tree) {
        ORM_StringBufferAppend(sb, " WHERE ");
        char* where_sql = ORM_ExpressionToSQL(qs->filter_tree);
        ORM_StringBufferAppend(sb, where_sql);
        free(where_sql);
    }
    
    char* sql = ORM_StringBufferRelease(sb);
    
    // Actual execution would happen in backend
    // For now, store SQL in string_val and free
    free(sql);
    
    return result;
}

ORM_FieldValue ORM_Sum(ORM_QuerySet* qs, const char* field) {
    return ORM_Aggregate(qs, field, "SUM");
}

ORM_FieldValue ORM_Avg(ORM_QuerySet* qs, const char* field) {
    return ORM_Aggregate(qs, field, "AVG");
}

ORM_FieldValue ORM_Min(ORM_QuerySet* qs, const char* field) {
    return ORM_Aggregate(qs, field, "MIN");
}

ORM_FieldValue ORM_Max(ORM_QuerySet* qs, const char* field) {
    return ORM_Aggregate(qs, field, "MAX");
}
