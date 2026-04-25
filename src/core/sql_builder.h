/* SQL Builder System
 * 
 * Generates SQL statements from QuerySets with parameterization and dialect support.
 */

#ifndef ORM_SQL_BUILDER_H
#define ORM_SQL_BUILDER_H

#include "queryset.h"
#include "model.h"
#include "field_types.h"
#include "../utils/memory.h"
#include <stdbool.h>

// ============================================================================
// SQL Dialect Support
// ============================================================================

typedef enum {
    ORM_DIALECT_SQLITE,
    ORM_DIALECT_POSTGRESQL,
    ORM_DIALECT_MYSQL,
    ORM_DIALECT_MSSQL
} ORM_Dialect;

// Get dialect name
const char* ORM_DialectName(ORM_Dialect dialect);

// ============================================================================
// Parameter Binding
// ============================================================================

typedef struct {
    void* value;
    ORM_FieldType type;
    size_t size;
} ORM_Parameter;

// ============================================================================
// SQL Builder Structure
// ============================================================================

typedef struct {
    ORM_Dialect dialect;
    ORM_StringBuffer* sql;
    ORM_StringBuffer* select_clause;
    ORM_StringBuffer* from_clause;
    ORM_StringBuffer* where_clause;
    ORM_StringBuffer* join_clause;
    ORM_StringBuffer* order_clause;
    ORM_StringBuffer* group_clause;
    ORM_Parameter* parameters;
    int param_count;
    int param_capacity;
} ORM_SQLBuilder;

// ============================================================================
// Builder Lifecycle
// ============================================================================

/* Create new SQL builder for specific dialect */
ORM_SQLBuilder* ORM_CreateSQLBuilder(ORM_Dialect dialect);

/* Free SQL builder and all resources */
void ORM_FreeSQLBuilder(ORM_SQLBuilder* builder);

/* Reset builder for reuse */
void ORM_ResetSQLBuilder(ORM_SQLBuilder* builder);

// ============================================================================
// Parameter Management
// ============================================================================

/* Add parameter and return placeholder */
int ORM_AddParameter(ORM_SQLBuilder* builder, void* value, ORM_FieldType type, size_t size);

/* Get placeholder string for parameter index */
const char* ORM_GetPlaceholder(ORM_SQLBuilder* builder, int param_index);

/* Get all parameters */
ORM_Parameter* ORM_GetParameters(ORM_SQLBuilder* builder, int* count);

// ============================================================================
// Query Builders
// ============================================================================

/* Build SELECT query from QuerySet */
char* ORM_BuildSelectSQL(ORM_SQLBuilder* builder, ORM_ModelMeta* model, ORM_QuerySet* qs);

/* Build INSERT query for instance */
char* ORM_BuildInsertSQL(ORM_SQLBuilder* builder, ORM_ModelMeta* model, void* instance);

/* Build UPDATE query for instance */
char* ORM_BuildUpdateSQL(ORM_SQLBuilder* builder, ORM_ModelMeta* model, void* instance, 
                         const char** fields, int field_count);

/* Build DELETE query from QuerySet */
char* ORM_BuildDeleteSQL(ORM_SQLBuilder* builder, ORM_ModelMeta* model, ORM_QuerySet* qs);

/* Build CREATE TABLE query */
char* ORM_BuildCreateTableSQL(ORM_SQLBuilder* builder, ORM_ModelMeta* model);

/* Build DROP TABLE query */
char* ORM_BuildDropTableSQL(ORM_SQLBuilder* builder, ORM_ModelMeta* model);

// ============================================================================
// Clause Builders
// ============================================================================

/* Build WHERE clause from expression tree */
void ORM_BuildWhereClause(ORM_SQLBuilder* builder, ORM_QueryNode* filter_tree);

/* Build ORDER BY clause */
void ORM_BuildOrderClause(ORM_SQLBuilder* builder, char** fields, int count);

/* Build LIMIT/OFFSET clause */
void ORM_BuildLimitClause(ORM_SQLBuilder* builder, int limit, int offset);

// ============================================================================
// SQL Escaping and Safety
// ============================================================================

/* Escape string for SQL (prevents injection) */
char* ORM_EscapeSQLString(const char* str, ORM_Dialect dialect);

/* Quote identifier (table/column name) */
char* ORM_QuoteIdentifier(const char* identifier, ORM_Dialect dialect);

/* Validate identifier is safe */
bool ORM_ValidateIdentifier(const char* identifier);

/* Unescape string from SQL */
char* ORM_UnescapeSQLString(const char* str, ORM_Dialect dialect);

// ============================================================================
// Utility Functions
// ============================================================================

/* Convert field value to SQL string representation */
char* ORM_FieldValueToSQL(ORM_SQLBuilder* builder, ORM_FieldValue* value, ORM_FieldType type);

/* Get SQL type name for field type */
const char* ORM_DialectTypeName(ORM_FieldType type, ORM_Dialect dialect);

/* Build column definition for CREATE TABLE */
char* ORM_BuildColumnDefinition(ORM_FieldMeta* field, ORM_Dialect dialect);

#endif /* ORM_SQL_BUILDER_H */
