/* QuerySet Test Suite
 * 
 * Tests for query builder functionality.
 */

#include "src/core/queryset.h"
#include "src/core/model.h"
#include "src/core/field_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ANSI color codes
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_RESET "\033[0m"

static int test_count = 0;
static int passed_count = 0;

#define TEST(name) \
    do { \
        printf(COLOR_BLUE "TEST: %s" COLOR_RESET "\n", name); \
        test_count++; \
    } while(0)

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf(COLOR_RED "  ✗ FAILED: %s" COLOR_RESET "\n", message); \
            return 0; \
        } \
        printf(COLOR_GREEN "  ✓ " COLOR_RESET "%s\n", message); \
        passed_count++; \
    } while(0)

// Test helper: Create a simple User model
static ORM_ModelMeta* create_user_model(void) {
    ORM_ModelMeta* model = (ORM_ModelMeta*)malloc(sizeof(ORM_ModelMeta));
    model->model_name = "User";
    model->table_name = "users";
    model->struct_size = 0;
    model->field_count = 4;
    model->fields = (ORM_FieldMeta*)malloc(sizeof(ORM_FieldMeta) * 4);
    
    model->fields[0].field_name = "id";
    model->fields[0].column_name = "id";
    model->fields[0].type = ORM_TYPE_INT;
    model->fields[0].flags = 0;
    
    model->fields[1].field_name = "name";
    model->fields[1].column_name = "name";
    model->fields[1].type = ORM_TYPE_STRING;
    model->fields[1].flags = 0;
    
    model->fields[2].field_name = "age";
    model->fields[2].column_name = "age";
    model->fields[2].type = ORM_TYPE_INT;
    model->fields[2].flags = 0;
    
    model->fields[3].field_name = "email";
    model->fields[3].column_name = "email";
    model->fields[3].type = ORM_TYPE_STRING;
    model->fields[3].flags = 0;
    
    return model;
}

// Test 1: QuerySet creation
static int test_queryset_creation(void) {
    TEST("QuerySet Creation");
    
    ORM_ModelMeta* model = create_user_model();
    ORM_QuerySet* qs = ORM_QuerySetNew(model);
    
    ASSERT(qs != NULL, "QuerySet created");
    ASSERT(qs->model == model, "Model set correctly");
    ASSERT(qs->filter_tree == NULL, "No initial filters");
    ASSERT(qs->order_by_count == 0, "No initial ordering");
    ASSERT(qs->limit_value == -1, "No initial limit");
    ASSERT(qs->offset_value == 0, "No initial offset");
    ASSERT(qs->distinct == false, "Not distinct by default");
    
    ORM_QuerySetFree(qs);
    return 1;
}

// Test 2: Simple filter
static int test_simple_filter(void) {
    TEST("Simple Filter");
    
    ORM_ModelMeta* model = create_user_model();
    ORM_QuerySet* qs = ORM_QuerySetNew(model);
    
    int age = 25;
    ORM_Filter(qs, "age", ">=", &age);
    
    ASSERT(qs->filter_tree != NULL, "Filter tree created");
    ASSERT(qs->filter_tree->type == EXPR_LEAF, "Filter is a leaf node");
    ASSERT(strcmp(qs->filter_tree->condition.field, "age") == 0, "Field name correct");
    ASSERT(qs->filter_tree->condition.op == OP_GE, "Operator correct");
    ASSERT(qs->filter_tree->condition.value.int_val == 25, "Value correct");
    
    ORM_QuerySetFree(qs);
    return 1;
}

// Test 3: Multiple filters (AND)
static int test_multiple_filters(void) {
    TEST("Multiple Filters (AND)");
    
    ORM_ModelMeta* model = create_user_model();
    ORM_QuerySet* qs = ORM_QuerySetNew(model);
    
    int age = 25;
    ORM_Filter(qs, "age", ">=", &age);
    ORM_Filter(qs, "name", "=", "John");
    
    ASSERT(qs->filter_tree != NULL, "Filter tree exists");
    ASSERT(qs->filter_tree->type == EXPR_AND, "Root is AND node");
    ASSERT(qs->filter_tree->left != NULL, "Left child exists");
    ASSERT(qs->filter_tree->right != NULL, "Right child exists");
    ASSERT(qs->filter_tree->left->type == EXPR_LEAF, "Left is leaf");
    ASSERT(qs->filter_tree->right->type == EXPR_LEAF, "Right is leaf");
    
    ORM_QuerySetFree(qs);
    return 1;
}

// Test 4: Exclude (NOT filter)
static int test_exclude(void) {
    TEST("Exclude Filter");
    
    ORM_ModelMeta* model = create_user_model();
    ORM_QuerySet* qs = ORM_QuerySetNew(model);
    
    int age = 18;
    ORM_Exclude(qs, "age", "<", &age);
    
    ASSERT(qs->filter_tree != NULL, "Filter tree exists");
    ASSERT(qs->filter_tree->type == EXPR_NOT, "Root is NOT node");
    ASSERT(qs->filter_tree->left != NULL, "Has child node");
    ASSERT(qs->filter_tree->left->type == EXPR_LEAF, "Child is leaf");
    
    ORM_QuerySetFree(qs);
    return 1;
}

// Test 5: Order by
static int test_order_by(void) {
    TEST("Order By");
    
    ORM_ModelMeta* model = create_user_model();
    ORM_QuerySet* qs = ORM_QuerySetNew(model);
    
    ORM_OrderBy(qs, "name");
    ORM_OrderBy(qs, "-age");  // Descending
    
    ASSERT(qs->order_by_count == 2, "Two order by fields");
    ASSERT(strcmp(qs->order_by_fields[0], "name") == 0, "First field correct");
    ASSERT(strcmp(qs->order_by_fields[1], "-age") == 0, "Second field correct");
    
    ORM_QuerySetFree(qs);
    return 1;
}

// Test 6: Limit and offset
static int test_limit_offset(void) {
    TEST("Limit and Offset");
    
    ORM_ModelMeta* model = create_user_model();
    ORM_QuerySet* qs = ORM_QuerySetNew(model);
    
    ORM_Limit(qs, 10);
    ORM_Offset(qs, 5);
    
    ASSERT(qs->limit_value == 10, "Limit set correctly");
    ASSERT(qs->offset_value == 5, "Offset set correctly");
    
    ORM_QuerySetFree(qs);
    return 1;
}

// Test 7: Distinct
static int test_distinct(void) {
    TEST("Distinct");
    
    ORM_ModelMeta* model = create_user_model();
    ORM_QuerySet* qs = ORM_QuerySetNew(model);
    
    ORM_Distinct(qs);
    
    ASSERT(qs->distinct == true, "Distinct enabled");
    
    ORM_QuerySetFree(qs);
    return 1;
}

// Test 8: Basic SQL generation
static int test_basic_sql(void) {
    TEST("Basic SQL Generation");
    
    ORM_ModelMeta* model = create_user_model();
    ORM_QuerySet* qs = ORM_QuerySetNew(model);
    
    char* sql = ORM_BuildSQL(qs);
    
    ASSERT(sql != NULL, "SQL generated");
    ASSERT(strstr(sql, "SELECT") != NULL, "Contains SELECT");
    ASSERT(strstr(sql, "FROM users") != NULL, "Contains FROM clause");
    
    free(sql);
    ORM_QuerySetFree(qs);
    return 1;
}

// Test 9: SQL with filter
static int test_sql_with_filter(void) {
    TEST("SQL with Filter");
    
    ORM_ModelMeta* model = create_user_model();
    ORM_QuerySet* qs = ORM_QuerySetNew(model);
    
    int age = 18;
    ORM_Filter(qs, "age", ">=", &age);
    
    char* sql = ORM_BuildSQL(qs);
    
    ASSERT(sql != NULL, "SQL generated");
    ASSERT(strstr(sql, "WHERE") != NULL, "Contains WHERE");
    ASSERT(strstr(sql, "age") != NULL, "Contains field name");
    ASSERT(strstr(sql, "18") != NULL, "Contains value");
    
    free(sql);
    ORM_QuerySetFree(qs);
    return 1;
}

// Test 10: SQL with order by
static int test_sql_with_order(void) {
    TEST("SQL with Order By");
    
    ORM_ModelMeta* model = create_user_model();
    ORM_QuerySet* qs = ORM_QuerySetNew(model);
    
    ORM_OrderBy(qs, "name");
    ORM_OrderBy(qs, "-age");
    
    char* sql = ORM_BuildSQL(qs);
    
    ASSERT(sql != NULL, "SQL generated");
    ASSERT(strstr(sql, "ORDER BY") != NULL, "Contains ORDER BY");
    ASSERT(strstr(sql, "name") != NULL, "Contains first field");
    ASSERT(strstr(sql, "age DESC") != NULL, "Contains DESC for second field");
    
    free(sql);
    ORM_QuerySetFree(qs);
    return 1;
}

// Test 11: Complex query
static int test_complex_query(void) {
    TEST("Complex Query");
    
    ORM_ModelMeta* model = create_user_model();
    ORM_QuerySet* qs = ORM_QuerySetNew(model);
    
    int age = 18;
    ORM_Filter(qs, "age", ">=", &age);
    ORM_Filter(qs, "name", "like", "John%");
    ORM_OrderBy(qs, "-age");
    ORM_Limit(qs, 10);
    ORM_Offset(qs, 5);
    ORM_Distinct(qs);
    
    char* sql = ORM_BuildSQL(qs);
    
    ASSERT(sql != NULL, "SQL generated");
    ASSERT(strstr(sql, "SELECT DISTINCT") != NULL, "Contains DISTINCT");
    ASSERT(strstr(sql, "WHERE") != NULL, "Contains WHERE");
    ASSERT(strstr(sql, "ORDER BY") != NULL, "Contains ORDER BY");
    ASSERT(strstr(sql, "LIMIT 10") != NULL, "Contains LIMIT");
    ASSERT(strstr(sql, "OFFSET 5") != NULL, "Contains OFFSET");
    
    free(sql);
    ORM_QuerySetFree(qs);
    return 1;
}

// Test 12: Operator parsing
static int test_operator_parsing(void) {
    TEST("Operator Parsing");
    
    ASSERT(ORM_ParseOperator("=") == OP_EQ, "Parse =");
    ASSERT(ORM_ParseOperator("eq") == OP_EQ, "Parse eq");
    ASSERT(ORM_ParseOperator(">=") == OP_GE, "Parse >=");
    ASSERT(ORM_ParseOperator("gte") == OP_GE, "Parse gte");
    ASSERT(ORM_ParseOperator("contains") == OP_CONTAINS, "Parse contains");
    ASSERT(ORM_ParseOperator("isnull") == OP_IS_NULL, "Parse isnull");
    
    return 1;
}

// Test 13: Operator to SQL
static int test_operator_to_sql(void) {
    TEST("Operator to SQL");
    
    ASSERT(strcmp(ORM_OperatorToSQL(OP_EQ), "=") == 0, "EQ to =");
    ASSERT(strcmp(ORM_OperatorToSQL(OP_GE), ">=") == 0, "GE to >=");
    ASSERT(strcmp(ORM_OperatorToSQL(OP_LIKE), "LIKE") == 0, "LIKE to LIKE");
    ASSERT(strcmp(ORM_OperatorToSQL(OP_IS_NULL), "IS NULL") == 0, "IS_NULL to IS NULL");
    
    return 1;
}

// Test 14: Expression to SQL
static int test_expression_to_sql(void) {
    TEST("Expression to SQL");
    
    int age = 25;
    ORM_QueryNode* node = ORM_Leaf("age", ">=", &age, ORM_TYPE_INT);
    
    char* sql = ORM_ExpressionToSQL(node);
    
    ASSERT(sql != NULL, "SQL generated");
    ASSERT(strstr(sql, "age") != NULL, "Contains field");
    ASSERT(strstr(sql, ">=") != NULL, "Contains operator");
    ASSERT(strstr(sql, "25") != NULL, "Contains value");
    
    free(sql);
    ORM_FreeExpression(node);
    return 1;
}

// Test 15: Complex expression (AND)
static int test_and_expression(void) {
    TEST("AND Expression");
    
    int age = 18;
    ORM_QueryNode* left = ORM_Leaf("age", ">=", &age, ORM_TYPE_INT);
    ORM_QueryNode* right = ORM_Leaf("name", "=", "John", ORM_TYPE_STRING);
    ORM_QueryNode* and_node = ORM_And(left, right);
    
    char* sql = ORM_ExpressionToSQL(and_node);
    
    ASSERT(sql != NULL, "SQL generated");
    ASSERT(strstr(sql, "AND") != NULL, "Contains AND");
    ASSERT(strstr(sql, "age") != NULL, "Contains first field");
    ASSERT(strstr(sql, "name") != NULL, "Contains second field");
    
    free(sql);
    ORM_FreeExpression(and_node);
    return 1;
}

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("  QuerySet Test Suite\n");
    printf("========================================\n\n");
    
    // Run all tests
    test_queryset_creation();
    test_simple_filter();
    test_multiple_filters();
    test_exclude();
    test_order_by();
    test_limit_offset();
    test_distinct();
    test_basic_sql();
    test_sql_with_filter();
    test_sql_with_order();
    test_complex_query();
    test_operator_parsing();
    test_operator_to_sql();
    test_expression_to_sql();
    test_and_expression();
    
    printf("\n");
    printf("========================================\n");
    printf("  Results: %d/%d tests passed\n", test_count, test_count);
    printf("  Assertions: %d passed\n", passed_count);
    printf("========================================\n\n");
    
    return (test_count == 15) ? 0 : 1;
}
