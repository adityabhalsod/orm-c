#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/core/model.h"

// Test counter
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Color codes for output
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_RESET "\033[0m"

// Test macros
#define TEST_START(name) \
    do { \
        printf("\n[TEST] %s\n", name); \
        tests_run++; \
    } while(0)

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf(COLOR_RED "  ✗ FAIL: %s" COLOR_RESET "\n", message); \
            tests_failed++; \
            return; \
        } else { \
            printf(COLOR_GREEN "  ✓ PASS: %s" COLOR_RESET "\n", message); \
        } \
    } while(0)

#define TEST_END() \
    do { \
        tests_passed++; \
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n"); \
    } while(0)

// ============================================================================
// Test Model Definitions
// ============================================================================

// Simple User model
typedef struct {
    int id;
    char username[50];
    char email[100];
    int age;
} User;

// Define the User model
#undef ORM_CURRENT_MODEL
#define ORM_CURRENT_MODEL User

ORM_MODEL_DEFINE(User, "users",
    ORM_FIELD_INT(id, ORM_PRIMARY_KEY | ORM_AUTO_INCREMENT),
    ORM_FIELD_STRING(username, 50, ORM_NOT_NULL | ORM_UNIQUE),
    ORM_FIELD_STRING(email, 100, ORM_NOT_NULL),
    ORM_FIELD_INT(age, 0)
);

// Product model for testing multiple models
typedef struct {
    int id;
    char name[100];
    double price;
    int stock;
} Product;

#undef ORM_CURRENT_MODEL
#define ORM_CURRENT_MODEL Product

ORM_MODEL_DEFINE(Product, "products",
    ORM_FIELD_INT(id, ORM_PRIMARY_KEY | ORM_AUTO_INCREMENT),
    ORM_FIELD_STRING(name, 100, ORM_NOT_NULL),
    ORM_FIELD_DOUBLE(price, ORM_NOT_NULL),
    ORM_FIELD_INT(stock, ORM_NOT_NULL)
);

// Article model for testing different field types
typedef struct {
    int id;
    char title[200];
    int published;
    int64_t views;
    float rating;
} Article;

#undef ORM_CURRENT_MODEL
#define ORM_CURRENT_MODEL Article

ORM_MODEL_DEFINE(Article, "articles",
    ORM_FIELD_INT(id, ORM_PRIMARY_KEY | ORM_AUTO_INCREMENT),
    ORM_FIELD_STRING(title, 200, ORM_NOT_NULL),
    ORM_FIELD_BOOLEAN(published, 0),
    ORM_FIELD_BIGINT(views, 0),
    ORM_FIELD_FLOAT(rating, 0)
);

// TestModel for testing all field types
typedef struct {
    int id;
    char text_content[1000];
    int64_t created_date;
    int64_t created_time;
    int64_t created_datetime;
    char blob_data[256];
    int author_id;
} TestModel;

#undef ORM_CURRENT_MODEL
#define ORM_CURRENT_MODEL TestModel

ORM_MODEL_DEFINE(TestModel, "test_models",
    ORM_FIELD_INT(id, ORM_PRIMARY_KEY | ORM_AUTO_INCREMENT),
    ORM_FIELD_TEXT(text_content, 1000, 0),
    ORM_FIELD_DATE(created_date, 0),
    ORM_FIELD_TIME(created_time, 0),
    ORM_FIELD_DATETIME(created_datetime, 0),
    ORM_FIELD_BLOB(blob_data, 256, 0),
    ORM_FIELD_FOREIGN_KEY(author_id, 0)
);

// ============================================================================
// Test Functions
// ============================================================================

void test_orm_init(void) {
    TEST_START("ORM Initialization");
    
    // Note: We don't shutdown here as it would clear the models we just registered
    // The registry is already initialized from main()
    
    TEST_ASSERT(g_orm_registry != NULL, "Global registry is initialized");
    TEST_ASSERT(g_orm_registry->models != NULL, "Registry models array is allocated");
    TEST_ASSERT(g_orm_registry->count >= 0, "Registry count is valid");
    TEST_ASSERT(g_orm_registry->capacity > 0, "Registry capacity is positive");
    
    // Test double initialization
    int result = ORM_Init();
    TEST_ASSERT(result == ORM_SUCCESS, "Double initialization succeeds");
    
    TEST_END();
}

void test_model_registration(void) {
    TEST_START("Model Registration");
    
    ORM_ModelMeta* user_meta = ORM_GetModel("User");
    TEST_ASSERT(user_meta != NULL, "User model is registered");
    TEST_ASSERT(strcmp(user_meta->model_name, "User") == 0, "Model name is correct");
    TEST_ASSERT(strcmp(user_meta->table_name, "users") == 0, "Table name is correct");
    TEST_ASSERT(user_meta->struct_size == sizeof(User), "Struct size is correct");
    TEST_ASSERT(user_meta->field_count == 4, "Field count is correct");
    
    ORM_ModelMeta* product_meta = ORM_GetModel("Product");
    TEST_ASSERT(product_meta != NULL, "Product model is registered");
    TEST_ASSERT(strcmp(product_meta->table_name, "products") == 0, "Product table name is correct");
    
    ORM_ModelMeta* article_meta = ORM_GetModel("Article");
    TEST_ASSERT(article_meta != NULL, "Article model is registered");
    
    // Test model count
    int count = ORM_GetModelCount();
    TEST_ASSERT(count >= 3, "At least 3 models are registered");
    
    TEST_END();
}

void test_field_metadata(void) {
    TEST_START("Field Metadata");
    
    ORM_ModelMeta* user_meta = ORM_GetModel("User");
    TEST_ASSERT(user_meta != NULL, "User model exists");
    
    // Test field 0 (id)
    ORM_FieldMeta* id_field = &user_meta->fields[0];
    TEST_ASSERT(strcmp(id_field->field_name, "id") == 0, "ID field name is correct");
    TEST_ASSERT(id_field->type == ORM_TYPE_INT, "ID field type is INT");
    TEST_ASSERT(id_field->flags & ORM_PRIMARY_KEY, "ID has PRIMARY_KEY flag");
    TEST_ASSERT(id_field->flags & ORM_AUTO_INCREMENT, "ID has AUTO_INCREMENT flag");
    TEST_ASSERT(id_field->offset == offsetof(User, id), "ID offset is correct");
    TEST_ASSERT(id_field->size == sizeof(int), "ID size is correct");
    
    // Test field 1 (username)
    ORM_FieldMeta* username_field = &user_meta->fields[1];
    TEST_ASSERT(strcmp(username_field->field_name, "username") == 0, "Username field name is correct");
    TEST_ASSERT(username_field->type == ORM_TYPE_STRING, "Username field type is STRING");
    TEST_ASSERT(username_field->flags & ORM_NOT_NULL, "Username has NOT_NULL flag");
    TEST_ASSERT(username_field->flags & ORM_UNIQUE, "Username has UNIQUE flag");
    TEST_ASSERT(username_field->size == 50, "Username size is correct");
    
    // Test field 2 (email)
    ORM_FieldMeta* email_field = &user_meta->fields[2];
    TEST_ASSERT(strcmp(email_field->field_name, "email") == 0, "Email field name is correct");
    TEST_ASSERT(email_field->size == 100, "Email size is correct");
    
    // Test field 3 (age)
    ORM_FieldMeta* age_field = &user_meta->fields[3];
    TEST_ASSERT(strcmp(age_field->field_name, "age") == 0, "Age field name is correct");
    TEST_ASSERT(age_field->type == ORM_TYPE_INT, "Age field type is INT");
    
    TEST_END();
}

void test_field_lookup(void) {
    TEST_START("Field Lookup by Name");
    
    ORM_ModelMeta* user_meta = ORM_GetModel("User");
    
    ORM_FieldMeta* username = ORM_GetField(user_meta, "username");
    TEST_ASSERT(username != NULL, "Username field found");
    TEST_ASSERT(strcmp(username->field_name, "username") == 0, "Correct field returned");
    
    ORM_FieldMeta* email = ORM_GetField(user_meta, "email");
    TEST_ASSERT(email != NULL, "Email field found");
    
    ORM_FieldMeta* nonexistent = ORM_GetField(user_meta, "nonexistent");
    TEST_ASSERT(nonexistent == NULL, "Nonexistent field returns NULL");
    
    TEST_END();
}

void test_primary_key_lookup(void) {
    TEST_START("Primary Key Lookup");
    
    ORM_ModelMeta* user_meta = ORM_GetModel("User");
    ORM_FieldMeta* pk = ORM_GetPrimaryKey(user_meta);
    
    TEST_ASSERT(pk != NULL, "Primary key found");
    TEST_ASSERT(strcmp(pk->field_name, "id") == 0, "Primary key is 'id' field");
    TEST_ASSERT(pk->flags & ORM_PRIMARY_KEY, "Has PRIMARY_KEY flag");
    
    TEST_END();
}

void test_field_flags(void) {
    TEST_START("Field Flag Checking");
    
    ORM_ModelMeta* user_meta = ORM_GetModel("User");
    ORM_FieldMeta* id_field = &user_meta->fields[0];
    ORM_FieldMeta* username_field = &user_meta->fields[1];
    
    TEST_ASSERT(ORM_FieldHasFlag(id_field, ORM_PRIMARY_KEY), "ID has PRIMARY_KEY");
    TEST_ASSERT(ORM_FieldHasFlag(id_field, ORM_AUTO_INCREMENT), "ID has AUTO_INCREMENT");
    TEST_ASSERT(!ORM_FieldHasFlag(id_field, ORM_UNIQUE), "ID doesn't have UNIQUE (only PK)");
    
    TEST_ASSERT(ORM_FieldHasFlag(username_field, ORM_NOT_NULL), "Username has NOT_NULL");
    TEST_ASSERT(ORM_FieldHasFlag(username_field, ORM_UNIQUE), "Username has UNIQUE");
    TEST_ASSERT(!ORM_FieldHasFlag(username_field, ORM_PRIMARY_KEY), "Username doesn't have PRIMARY_KEY");
    
    TEST_END();
}

void test_model_constructor_destructor(void) {
    TEST_START("Model Constructor and Destructor");
    
    ORM_ModelMeta* user_meta = ORM_GetModel("User");
    TEST_ASSERT(user_meta->constructor != NULL, "Constructor function exists");
    TEST_ASSERT(user_meta->destructor != NULL, "Destructor function exists");
    
    // Test constructor
    User* user = (User*)user_meta->constructor();
    TEST_ASSERT(user != NULL, "Constructor creates object");
    
    // Verify memory is zeroed
    TEST_ASSERT(user->id == 0, "ID is initialized to 0");
    TEST_ASSERT(user->age == 0, "Age is initialized to 0");
    TEST_ASSERT(user->username[0] == '\0', "Username is initialized to empty");
    
    // Set some values
    user->id = 1;
    strcpy(user->username, "testuser");
    strcpy(user->email, "test@example.com");
    user->age = 25;
    
    TEST_ASSERT(user->id == 1, "Can set ID value");
    TEST_ASSERT(strcmp(user->username, "testuser") == 0, "Can set username");
    TEST_ASSERT(user->age == 25, "Can set age");
    
    // Test destructor (should not crash)
    user_meta->destructor(user);
    TEST_ASSERT(1, "Destructor executes without crash");
    
    TEST_END();
}

void test_field_value_access(void) {
    TEST_START("Field Value Access");
    
    User user;
    user.id = 42;
    user.age = 30;
    strcpy(user.username, "john_doe");
    
    ORM_ModelMeta* meta = ORM_GetModel("User");
    ORM_FieldMeta* id_field = ORM_GetField(meta, "id");
    ORM_FieldMeta* age_field = ORM_GetField(meta, "age");
    ORM_FieldMeta* username_field = ORM_GetField(meta, "username");
    
    int* id_ptr = (int*)ORM_GetFieldValue(&user, id_field);
    TEST_ASSERT(id_ptr != NULL, "ID field value pointer retrieved");
    TEST_ASSERT(*id_ptr == 42, "ID value is correct");
    
    int* age_ptr = (int*)ORM_GetFieldValue(&user, age_field);
    TEST_ASSERT(*age_ptr == 30, "Age value is correct");
    
    char* username_ptr = (char*)ORM_GetFieldValue(&user, username_field);
    TEST_ASSERT(strcmp(username_ptr, "john_doe") == 0, "Username value is correct");
    
    TEST_END();
}

void test_different_field_types(void) {
    TEST_START("Different Field Types");
    
    ORM_ModelMeta* article_meta = ORM_GetModel("Article");
    TEST_ASSERT(article_meta != NULL, "Article model exists");
    
    ORM_FieldMeta* id = ORM_GetField(article_meta, "id");
    TEST_ASSERT(id->type == ORM_TYPE_INT, "ID is INT type");
    
    ORM_FieldMeta* title = ORM_GetField(article_meta, "title");
    TEST_ASSERT(title->type == ORM_TYPE_STRING, "Title is STRING type");
    
    ORM_FieldMeta* published = ORM_GetField(article_meta, "published");
    TEST_ASSERT(published->type == ORM_TYPE_BOOLEAN, "Published is BOOLEAN type");
    
    ORM_FieldMeta* views = ORM_GetField(article_meta, "views");
    TEST_ASSERT(views->type == ORM_TYPE_BIGINT, "Views is BIGINT type");
    
    ORM_FieldMeta* rating = ORM_GetField(article_meta, "rating");
    TEST_ASSERT(rating->type == ORM_TYPE_FLOAT, "Rating is FLOAT type");
    
    TEST_END();
}

void test_model_unregistration(void) {
    TEST_START("Model Unregistration");
    
    int initial_count = ORM_GetModelCount();
    TEST_ASSERT(initial_count > 0, "Initial model count is positive");
    
    // Get a model that we'll unregister
    ORM_ModelMeta* product_meta = ORM_GetModel("Product");
    TEST_ASSERT(product_meta != NULL, "Product model exists before unregister");
    
    // Unregister the model
    int result = ORM_UnregisterModel("Product");
    TEST_ASSERT(result == ORM_SUCCESS, "Unregister returns success");
    
    // Verify it's gone
    product_meta = ORM_GetModel("Product");
    TEST_ASSERT(product_meta == NULL, "Product model not found after unregister");
    
    int new_count = ORM_GetModelCount();
    TEST_ASSERT(new_count == initial_count - 1, "Model count decreased by 1");
    
    // Try to unregister nonexistent model
    result = ORM_UnregisterModel("NonexistentModel");
    TEST_ASSERT(result == ORM_ERROR_NOT_FOUND, "Unregister nonexistent returns error");
    
    TEST_END();
}

void test_model_not_found(void) {
    TEST_START("Model Not Found");
    
    ORM_ModelMeta* meta = ORM_GetModel("NonexistentModel");
    TEST_ASSERT(meta == NULL, "Nonexistent model returns NULL");
    
    meta = ORM_GetModel("");
    TEST_ASSERT(meta == NULL, "Empty model name returns NULL");
    
    meta = ORM_GetModel(NULL);
    TEST_ASSERT(meta == NULL, "NULL model name returns NULL");
    
    TEST_END();
}

void test_registry_capacity_growth(void) {
    TEST_START("Registry Capacity Growth");
    
    int initial_capacity = g_orm_registry->capacity;
    int initial_count = g_orm_registry->count;
    
    TEST_ASSERT(initial_capacity > 0, "Initial capacity is positive");
    TEST_ASSERT(initial_count <= initial_capacity, "Count doesn't exceed capacity");
    
    // Note: We can't easily test growth without creating many models
    // This test just verifies the state is consistent
    TEST_ASSERT(g_orm_registry->models != NULL, "Models array is allocated");
    
    TEST_END();
}

void test_orm_shutdown(void) {
    TEST_START("ORM Shutdown");
    
    TEST_ASSERT(g_orm_registry != NULL, "Registry exists before shutdown");
    
    ORM_Shutdown();
    TEST_ASSERT(g_orm_registry == NULL, "Registry is NULL after shutdown");
    
    // Test double shutdown (should not crash)
    ORM_Shutdown();
    TEST_ASSERT(1, "Double shutdown doesn't crash");
    
    // Reinitialize for other tests
    ORM_Init();
    TEST_ASSERT(g_orm_registry != NULL, "Registry reinitializes after shutdown");
    
    TEST_END();
}

void test_crud_function_generation(void) {
    TEST_START("CRUD Function Generation");
    
    ORM_ModelMeta* user_meta = ORM_GetModel("User");
    TEST_ASSERT(user_meta != NULL, "User model exists");
    
    // Create a user instance
    User* user = User_new();
    TEST_ASSERT(user != NULL, "User_new() works");
    
    user->id = 1;
    strcpy(user->username, "testuser");
    strcpy(user->email, "test@example.com");
    user->age = 25;
    
    // Test save function (should return error as it's not implemented)
    int result = User_save(user);
    TEST_ASSERT(result == ORM_ERROR, "User_save() returns ORM_ERROR (not implemented)");
    
    // Test get function (should return NULL as it's not implemented)
    User* fetched = User_get(1);
    TEST_ASSERT(fetched == NULL, "User_get() returns NULL (not implemented)");
    
    // Test objects function (should return NULL as it's not implemented)
    QuerySet* qs = User_objects();
    TEST_ASSERT(qs == NULL, "User_objects() returns NULL (not implemented)");
    
    // Test delete function (should return error as it's not implemented)
    result = User_delete(user);
    TEST_ASSERT(result == ORM_ERROR, "User_delete() returns ORM_ERROR (not implemented)");
    
    // Clean up
    User_free(user);
    TEST_ASSERT(1, "User_free() executes without crash");
    
    TEST_END();
}

void test_default_values(void) {
    TEST_START("Default Values");
    
    ORM_ModelMeta* user_meta = ORM_GetModel("User");
    TEST_ASSERT(user_meta != NULL, "User model exists");
    
    // Set a default value for age field
    ORM_FieldMeta* age_field = ORM_GetField(user_meta, "age");
    TEST_ASSERT(age_field != NULL, "Age field exists");
    
    int default_age = 18;
    int result = ORM_SetDefaultValue(age_field, &default_age, sizeof(int));
    TEST_ASSERT(result == ORM_SUCCESS, "Default value set successfully");
    TEST_ASSERT(age_field->flags & ORM_HAS_DEFAULT, "HAS_DEFAULT flag is set");
    TEST_ASSERT(age_field->default_value != NULL, "Default value pointer is set");
    
    // Create a new user and apply defaults
    User* user = User_new();
    result = ORM_ApplyDefaults(user_meta, user);
    TEST_ASSERT(result == ORM_SUCCESS, "Defaults applied successfully");
    TEST_ASSERT(user->age == 18, "Default age value is applied");
    
    User_free(user);
    
    // Clean up default value
    if (age_field->default_value) {
        free(age_field->default_value);
        age_field->default_value = NULL;
        age_field->flags &= ~ORM_HAS_DEFAULT;
    }
    
    TEST_END();
}

void test_additional_field_types(void) {
    TEST_START("Additional Field Type Macros");
    
    // Register the model
    int result = ORM_RegisterModel(&TestModel_meta);
    TEST_ASSERT(result == ORM_SUCCESS || result == ORM_ERROR_DUPLICATE, 
                "TestModel registered successfully");
    
    ORM_ModelMeta* meta = ORM_GetModel("TestModel");
    TEST_ASSERT(meta != NULL, "TestModel found in registry");
    TEST_ASSERT(meta->field_count == 7, "TestModel has 7 fields");
    
    // Verify field types
    TEST_ASSERT(meta->fields[0].type == ORM_TYPE_INT, "Field 0 is INT");
    TEST_ASSERT(meta->fields[1].type == ORM_TYPE_TEXT, "Field 1 is TEXT");
    TEST_ASSERT(meta->fields[2].type == ORM_TYPE_DATE, "Field 2 is DATE");
    TEST_ASSERT(meta->fields[3].type == ORM_TYPE_TIME, "Field 3 is TIME");
    TEST_ASSERT(meta->fields[4].type == ORM_TYPE_DATETIME, "Field 4 is DATETIME");
    TEST_ASSERT(meta->fields[5].type == ORM_TYPE_BLOB, "Field 5 is BLOB");
    TEST_ASSERT(meta->fields[6].type == ORM_TYPE_FOREIGN_KEY, "Field 6 is FOREIGN_KEY");
    
    // Test constructor
    TestModel* tm = TestModel_new();
    TEST_ASSERT(tm != NULL, "TestModel_new() works");
    
    // Clean up
    TestModel_free(tm);
    
    TEST_END();
}

// ============================================================================
// Main Test Runner
// ============================================================================

void print_test_summary(void) {
    printf("\n");
    printf("================================================================================\n");
    printf("TEST SUMMARY\n");
    printf("================================================================================\n");
    printf("Total tests:  %d\n", tests_run);
    printf(COLOR_GREEN "Passed:       %d" COLOR_RESET "\n", tests_passed);
    
    if (tests_failed > 0) {
        printf(COLOR_RED "Failed:       %d" COLOR_RESET "\n", tests_failed);
    } else {
        printf("Failed:       %d\n", tests_failed);
    }
    
    printf("================================================================================\n");
    
    if (tests_failed == 0) {
        printf(COLOR_GREEN "✓ ALL TESTS PASSED!" COLOR_RESET "\n");
    } else {
        printf(COLOR_RED "✗ SOME TESTS FAILED!" COLOR_RESET "\n");
    }
    printf("================================================================================\n");
}

int main(void) {
    printf("================================================================================\n");
    printf("ORM MODEL SYSTEM TESTS\n");
    printf("================================================================================\n");
    
    // Initialize ORM
    ORM_Init();
    printf("ORM Initialized. Registry count: %d\n", ORM_GetModelCount());
    
    // Manually register models (constructor attribute may not work in all contexts)
    printf("Registering User model...\n");
    int result = ORM_RegisterModel(&User_meta);
    printf("User registration result: %d\n", result);
    
    printf("Registering Product model...\n");
    result = ORM_RegisterModel(&Product_meta);
    printf("Product registration result: %d\n", result);
    
    printf("Registering Article model...\n");
    result = ORM_RegisterModel(&Article_meta);
    printf("Article registration result: %d\n", result);
    
    printf("Registering TestModel model...\n");
    result = ORM_RegisterModel(&TestModel_meta);
    printf("TestModel registration result: %d\n", result);
    
    printf("Total models registered: %d\n\n", ORM_GetModelCount());
    
    // Run tests
    test_orm_init();
    test_model_registration();
    test_field_metadata();
    test_field_lookup();
    test_primary_key_lookup();
    test_field_flags();
    test_model_constructor_destructor();
    test_field_value_access();
    test_different_field_types();
    test_model_not_found();
    test_registry_capacity_growth();
    test_crud_function_generation();    // New test
    test_default_values();              // New test
    test_additional_field_types();      // New test
    test_model_unregistration();        // Run near end as it modifies registry
    test_orm_shutdown();                // Run last
    
    // Print summary
    print_test_summary();
    
    // Cleanup
    ORM_Shutdown();
    
    return (tests_failed == 0) ? 0 : 1;
}
