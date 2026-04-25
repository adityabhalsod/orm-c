/* Result Set Management
 * 
 * Efficient result set handling with iteration, type-safe access,
 * and memory management for database query results.
 */

#ifndef ORM_RESULT_SET_H
#define ORM_RESULT_SET_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include "field_types.h"
#include "model.h"

// Forward declarations
typedef struct ORM_ResultSet ORM_ResultSet;
typedef struct ORM_ResultIterator ORM_ResultIterator;
typedef struct ORM_Cursor ORM_Cursor;
typedef struct ORM_ColumnMeta ORM_ColumnMeta;

// ============================================================================
// Column Metadata
// ============================================================================

struct ORM_ColumnMeta {
    char* name;
    ORM_FieldType type;
    int index;
    size_t size;
};

// ============================================================================
// Result Set Structure
// ============================================================================

struct ORM_ResultSet {
    ORM_ModelMeta* model;           // Associated model (can be NULL for raw queries)
    int row_count;                  // Total number of rows
    int column_count;               // Number of columns
    ORM_ColumnMeta* columns;        // Column metadata
    void*** rows;                   // 2D array: [row][column]
    int current_row;                // Current position for iteration (-1 = before first)
    bool owns_data;                 // Whether this result set owns the data
};

// ============================================================================
// Result Set Operations
// ============================================================================

/* Create a result set with specified dimensions */
ORM_ResultSet* ORM_CreateResultSet(int row_count, int column_count);

/* Create result set from model metadata */
ORM_ResultSet* ORM_CreateResultSetFromModel(ORM_ModelMeta* model, int row_count);

/* Free result set and all associated data */
void ORM_FreeResultSet(ORM_ResultSet* rs);

/* Add a column to the result set */
int ORM_AddColumn(ORM_ResultSet* rs, const char* name, ORM_FieldType type, size_t size);

/* Set column metadata */
void ORM_SetColumnMeta(ORM_ResultSet* rs, int col_index, const char* name, 
                       ORM_FieldType type, size_t size);

// ============================================================================
// Iteration
// ============================================================================

/* Move to next row (returns true if row available) */
bool ORM_ResultNext(ORM_ResultSet* rs);

/* Get current row data */
void* ORM_ResultCurrent(ORM_ResultSet* rs, int column);

/* Reset iterator to beginning */
void ORM_ResultReset(ORM_ResultSet* rs);

/* Check if there are more rows */
bool ORM_ResultHasNext(ORM_ResultSet* rs);

/* Get specific row (without changing current position) */
void** ORM_GetRow(ORM_ResultSet* rs, int row_index);

// ============================================================================
// Type-Safe Data Access
// ============================================================================

/* Get value by column name */
void* ORM_GetValue(ORM_ResultSet* rs, int row, const char* column);

/* Get value by column index */
void* ORM_GetValueByIndex(ORM_ResultSet* rs, int row, int column);

/* Type-specific getters by name */
int ORM_GetInt(ORM_ResultSet* rs, int row, const char* column);
int64_t ORM_GetBigInt(ORM_ResultSet* rs, int row, const char* column);
float ORM_GetFloat(ORM_ResultSet* rs, int row, const char* column);
double ORM_GetDouble(ORM_ResultSet* rs, int row, const char* column);
const char* ORM_GetString(ORM_ResultSet* rs, int row, const char* column);
bool ORM_GetBool(ORM_ResultSet* rs, int row, const char* column);
time_t ORM_GetDateTime(ORM_ResultSet* rs, int row, const char* column);

/* Type-specific getters by index */
int ORM_GetIntByIndex(ORM_ResultSet* rs, int row, int column);
int64_t ORM_GetBigIntByIndex(ORM_ResultSet* rs, int row, int column);
double ORM_GetDoubleByIndex(ORM_ResultSet* rs, int row, int column);
const char* ORM_GetStringByIndex(ORM_ResultSet* rs, int row, int column);

/* Null handling */
bool ORM_IsNull(ORM_ResultSet* rs, int row, const char* column);
bool ORM_IsNullByIndex(ORM_ResultSet* rs, int row, int column);

// ============================================================================
// Column Information
// ============================================================================

/* Get column index by name */
int ORM_GetColumnIndex(ORM_ResultSet* rs, const char* name);

/* Get column name by index */
const char* ORM_GetColumnName(ORM_ResultSet* rs, int index);

/* Get column type by index */
ORM_FieldType ORM_GetColumnType(ORM_ResultSet* rs, int index);

/* Get column count */
int ORM_GetColumnCount(ORM_ResultSet* rs);

/* Get row count */
int ORM_GetRowCount(ORM_ResultSet* rs);

// ============================================================================
// Iterator Pattern
// ============================================================================

struct ORM_ResultIterator {
    ORM_ResultSet* result_set;
    int current_index;
    void** current_row;
};

/* Create iterator for result set */
ORM_ResultIterator* ORM_CreateIterator(ORM_ResultSet* rs);

/* Free iterator */
void ORM_FreeIterator(ORM_ResultIterator* iter);

/* Check if iterator has next element */
bool ORM_IteratorHasNext(ORM_ResultIterator* iter);

/* Get next row from iterator */
void** ORM_IteratorNext(ORM_ResultIterator* iter);

/* Reset iterator to beginning */
void ORM_IteratorReset(ORM_ResultIterator* iter);

// Foreach macro for convenient iteration
#define ORM_FOREACH_ROW(rs) \
    for (ORM_ResultReset(rs); ORM_ResultNext(rs); )

// ============================================================================
// Cursor-Based Pagination
// ============================================================================

struct ORM_Cursor {
    ORM_ResultSet* result_set;
    char* cursor_token;
    int page_size;
    int current_offset;
    bool has_next;
    bool has_previous;
};

/* Create cursor for pagination */
ORM_Cursor* ORM_CreateCursor(ORM_ResultSet* rs, int page_size);

/* Free cursor */
void ORM_FreeCursor(ORM_Cursor* cursor);

/* Get next page */
ORM_ResultSet* ORM_CursorNextPage(ORM_Cursor* cursor);

/* Get previous page */
ORM_ResultSet* ORM_CursorPreviousPage(ORM_Cursor* cursor);

/* Get cursor token (opaque string representing position) */
const char* ORM_GetCursorToken(ORM_Cursor* cursor);

/* Create cursor from token */
ORM_Cursor* ORM_FromCursorToken(ORM_ResultSet* rs, const char* token, int page_size);

// ============================================================================
// Result Set Transformations
// ============================================================================

/* Map function over result set rows */
typedef void* (*ORM_MapFunction)(void** row, int column_count, void* context);

ORM_ResultSet* ORM_MapResultSet(ORM_ResultSet* rs, ORM_MapFunction fn, void* context);

/* Filter result set rows */
typedef bool (*ORM_FilterFunction)(void** row, int column_count, void* context);

ORM_ResultSet* ORM_FilterResultSet(ORM_ResultSet* rs, ORM_FilterFunction fn, void* context);

/* Export to JSON */
char* ORM_ResultSetToJSON(ORM_ResultSet* rs);

/* Export to CSV */
char* ORM_ResultSetToCSV(ORM_ResultSet* rs);

/* Convert to array of rows */
void*** ORM_ResultSetToArray(ORM_ResultSet* rs);

// ============================================================================
// Utility Functions
// ============================================================================

/* Print result set (for debugging) */
void ORM_PrintResultSet(ORM_ResultSet* rs);

/* Clone result set */
ORM_ResultSet* ORM_CloneResultSet(ORM_ResultSet* rs);

/* Slice result set (create subset) */
ORM_ResultSet* ORM_SliceResultSet(ORM_ResultSet* rs, int start, int end);

#endif /* ORM_RESULT_SET_H */
