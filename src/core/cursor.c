/* Cursor-Based Pagination Implementation
 * 
 * Provides efficient pagination for large result sets using cursor tokens.
 */

#define _GNU_SOURCE
#include "result_set.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Cursor Implementation
// ============================================================================

ORM_Cursor* ORM_CreateCursor(ORM_ResultSet* rs, int page_size) {
    if (!rs || page_size <= 0) {
        return NULL;
    }
    
    ORM_Cursor* cursor = (ORM_Cursor*)calloc(1, sizeof(ORM_Cursor));
    if (!cursor) {
        return NULL;
    }
    
    cursor->result_set = rs;
    cursor->page_size = page_size;
    cursor->current_offset = 0;
    cursor->has_next = rs->row_count > page_size;
    cursor->has_previous = false;
    cursor->cursor_token = NULL;
    
    return cursor;
}

void ORM_FreeCursor(ORM_Cursor* cursor) {
    if (!cursor) {
        return;
    }
    
    if (cursor->cursor_token) {
        free(cursor->cursor_token);
    }
    
    free(cursor);
}

ORM_ResultSet* ORM_CursorNextPage(ORM_Cursor* cursor) {
    if (!cursor || !cursor->has_next) {
        return NULL;
    }
    
    int start = cursor->current_offset;
    int end = start + cursor->page_size;
    
    if (end > cursor->result_set->row_count) {
        end = cursor->result_set->row_count;
    }
    
    ORM_ResultSet* page = ORM_SliceResultSet(cursor->result_set, start, end);
    
    if (page) {
        // Update cursor state
        cursor->current_offset = end;
        cursor->has_next = end < cursor->result_set->row_count;
        cursor->has_previous = start > 0;
        
        // Update cursor token
        if (cursor->cursor_token) {
            free(cursor->cursor_token);
        }
        
        cursor->cursor_token = (char*)malloc(64);
        snprintf(cursor->cursor_token, 64, "offset:%d:size:%d", 
                 cursor->current_offset, cursor->page_size);
    }
    
    return page;
}

ORM_ResultSet* ORM_CursorPreviousPage(ORM_Cursor* cursor) {
    if (!cursor || !cursor->has_previous) {
        return NULL;
    }
    
    int end = cursor->current_offset - cursor->page_size;
    int start = end - cursor->page_size;
    
    if (start < 0) {
        start = 0;
    }
    
    ORM_ResultSet* page = ORM_SliceResultSet(cursor->result_set, start, end);
    
    if (page) {
        // Update cursor state
        cursor->current_offset = start;
        cursor->has_next = end < cursor->result_set->row_count;
        cursor->has_previous = start > 0;
        
        // Update cursor token
        if (cursor->cursor_token) {
            free(cursor->cursor_token);
        }
        
        cursor->cursor_token = (char*)malloc(64);
        snprintf(cursor->cursor_token, 64, "offset:%d:size:%d", 
                 cursor->current_offset, cursor->page_size);
    }
    
    return page;
}

const char* ORM_GetCursorToken(ORM_Cursor* cursor) {
    return cursor ? cursor->cursor_token : NULL;
}

ORM_Cursor* ORM_FromCursorToken(ORM_ResultSet* rs, const char* token, int page_size) {
    if (!rs || !token) {
        return NULL;
    }
    
    // Parse token: "offset:N:size:M"
    int offset = 0;
    int size = page_size;
    
    if (sscanf(token, "offset:%d:size:%d", &offset, &size) != 2) {
        return NULL;
    }
    
    ORM_Cursor* cursor = ORM_CreateCursor(rs, size);
    if (cursor) {
        cursor->current_offset = offset;
        cursor->has_next = offset < rs->row_count;
        cursor->has_previous = offset > 0;
        
        cursor->cursor_token = strdup(token);
    }
    
    return cursor;
}
