#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../src/core/model.h"

// ============================================================================
// Advanced Model Definitions - Demonstrating All Field Types
// ============================================================================

// Blog Post model - demonstrates all field types
typedef struct {
    int id;
    char title[200];
    char content[2000];         // Using TEXT type
    char author[100];
    int is_published;            // BOOLEAN
    int64_t created_at;         // DATETIME
    int64_t published_date;     // DATE
    int64_t last_edit_time;     // TIME
    int64_t view_count;         // BIGINT
    float rating;               // FLOAT
    double revenue;             // DOUBLE
    char metadata[512];         // BLOB
    int category_id;            // FOREIGN_KEY
} BlogPost;

#undef ORM_CURRENT_MODEL
#define ORM_CURRENT_MODEL BlogPost

ORM_MODEL_DEFINE(BlogPost, "blog_posts",
    ORM_FIELD_INT(id, ORM_PRIMARY_KEY | ORM_AUTO_INCREMENT),
    ORM_FIELD_STRING(title, 200, ORM_NOT_NULL),
    ORM_FIELD_TEXT(content, 2000, ORM_NOT_NULL),
    ORM_FIELD_STRING(author, 100, ORM_NOT_NULL),
    ORM_FIELD_BOOLEAN(is_published, 0),
    ORM_FIELD_DATETIME(created_at, ORM_NOT_NULL),
    ORM_FIELD_DATE(published_date, 0),
    ORM_FIELD_TIME(last_edit_time, 0),
    ORM_FIELD_BIGINT(view_count, 0),
    ORM_FIELD_FLOAT(rating, 0),
    ORM_FIELD_DOUBLE(revenue, 0),
    ORM_FIELD_BLOB(metadata, 512, 0),
    ORM_FIELD_FOREIGN_KEY(category_id, 0)
);

// Category model - simple reference model
typedef struct {
    int id;
    char name[100];
    char description[500];
} Category;

#undef ORM_CURRENT_MODEL
#define ORM_CURRENT_MODEL Category

ORM_MODEL_DEFINE(Category, "categories",
    ORM_FIELD_INT(id, ORM_PRIMARY_KEY | ORM_AUTO_INCREMENT),
    ORM_FIELD_STRING(name, 100, ORM_NOT_NULL | ORM_UNIQUE),
    ORM_FIELD_STRING(description, 500, 0)
);

// User model with defaults
typedef struct {
    int id;
    char username[50];
    char email[100];
    int age;
    int is_active;
    int64_t signup_date;
} User;

#undef ORM_CURRENT_MODEL
#define ORM_CURRENT_MODEL User

ORM_MODEL_DEFINE(User, "users",
    ORM_FIELD_INT(id, ORM_PRIMARY_KEY | ORM_AUTO_INCREMENT),
    ORM_FIELD_STRING(username, 50, ORM_NOT_NULL | ORM_UNIQUE),
    ORM_FIELD_STRING(email, 100, ORM_NOT_NULL),
    ORM_FIELD_INT(age, 0),
    ORM_FIELD_BOOLEAN(is_active, 0),
    ORM_FIELD_DATETIME(signup_date, ORM_NOT_NULL)
);

// ============================================================================
// Example Functions
// ============================================================================

void print_separator(const char* title) {
    printf("\n");
    printf("================================================================================\n");
    printf("%s\n", title);
    printf("================================================================================\n");
}

void example_all_field_types(void) {
    print_separator("Example 1: All Field Types");
    
    // Create a blog post with all field types
    BlogPost* post = BlogPost_new();
    
    post->id = 1;
    strcpy(post->title, "Advanced C ORM Features");
    strcpy(post->content, "This is a comprehensive example demonstrating all field types...");
    strcpy(post->author, "John Doe");
    post->is_published = 1;
    post->created_at = 1698765432000LL;  // Timestamp in milliseconds
    post->published_date = 20231031;      // YYYYMMDD format
    post->last_edit_time = 143000;        // HHMMSS format
    post->view_count = 1234567890LL;
    post->rating = 4.7f;
    post->revenue = 1234.56;
    post->category_id = 5;
    
    // Set some binary metadata
    memset(post->metadata, 0, sizeof(post->metadata));
    strcpy(post->metadata, "Binary metadata can go here");
    
    printf("Created BlogPost:\n");
    printf("  ID: %d\n", post->id);
    printf("  Title: %s\n", post->title);
    printf("  Author: %s\n", post->author);
    printf("  Published: %s\n", post->is_published ? "Yes" : "No");
    printf("  Created: %lld\n", (long long)post->created_at);
    printf("  Views: %lld\n", (long long)post->view_count);
    printf("  Rating: %.2f\n", post->rating);
    printf("  Revenue: $%.2f\n", post->revenue);
    printf("  Category ID: %d\n", post->category_id);
    
    BlogPost_free(post);
}

void example_model_introspection(void) {
    print_separator("Example 2: Model Introspection");
    
    ORM_ModelMeta* blog_meta = ORM_GetModel("BlogPost");
    
    printf("Model: %s\n", blog_meta->model_name);
    printf("Table: %s\n", blog_meta->table_name);
    printf("Size: %zu bytes\n", blog_meta->struct_size);
    printf("Fields: %d\n\n", blog_meta->field_count);
    
    printf("Field Details:\n");
    for (int i = 0; i < blog_meta->field_count; i++) {
        ORM_FieldMeta* field = &blog_meta->fields[i];
        printf("  [%2d] %-20s %-12s offset:%4zu size:%4zu flags:0x%02x\n",
               i,
               field->field_name,
               ORM_FieldTypeToString(field->type),
               field->offset,
               field->size,
               field->flags);
    }
}

void example_default_values(void) {
    print_separator("Example 3: Default Values");
    
    ORM_ModelMeta* user_meta = ORM_GetModel("User");
    
    // Set default values for new users
    printf("Setting default values...\n");
    
    // Default age: 18
    ORM_FieldMeta* age_field = ORM_GetField(user_meta, "age");
    int default_age = 18;
    ORM_SetDefaultValue(age_field, &default_age, sizeof(int));
    printf("  Default age: %d\n", default_age);
    
    // Default is_active: true (1)
    ORM_FieldMeta* active_field = ORM_GetField(user_meta, "is_active");
    int default_active = 1;
    ORM_SetDefaultValue(active_field, &default_active, sizeof(int));
    printf("  Default is_active: %d\n", default_active);
    
    // Create a new user and apply defaults
    User* user = User_new();
    strcpy(user->username, "newuser");
    strcpy(user->email, "newuser@example.com");
    
    printf("\nBefore applying defaults:\n");
    printf("  Age: %d\n", user->age);
    printf("  Is Active: %d\n", user->is_active);
    
    ORM_ApplyDefaults(user_meta, user);
    
    printf("\nAfter applying defaults:\n");
    printf("  Age: %d\n", user->age);
    printf("  Is Active: %d\n", user->is_active);
    
    User_free(user);
    
    // Clean up
    if (age_field->default_value) {
        free(age_field->default_value);
        age_field->default_value = NULL;
        age_field->flags &= ~ORM_HAS_DEFAULT;
    }
    if (active_field->default_value) {
        free(active_field->default_value);
        active_field->default_value = NULL;
        active_field->flags &= ~ORM_HAS_DEFAULT;
    }
}

void example_crud_functions(void) {
    print_separator("Example 4: CRUD Function Stubs");
    
    printf("Note: CRUD functions are generated but return errors until\n");
    printf("database backends are implemented in later tasks.\n\n");
    
    User* user = User_new();
    user->id = 1;
    strcpy(user->username, "testuser");
    strcpy(user->email, "test@example.com");
    user->age = 25;
    
    printf("Attempting to call CRUD functions:\n");
    
    // Try to save (will return error)
    int result = User_save(user);
    printf("  User_save() returned: %d (expected: %d = ORM_ERROR)\n", result, ORM_ERROR);
    
    // Try to get (will return NULL)
    User* fetched = User_get(1);
    printf("  User_get(1) returned: %p (expected: NULL)\n", (void*)fetched);
    
    // Try to get objects (will return NULL)
    QuerySet* qs = User_objects();
    printf("  User_objects() returned: %p (expected: NULL)\n", (void*)qs);
    
    // Try to delete (will return error)
    result = User_delete(user);
    printf("  User_delete() returned: %d (expected: %d = ORM_ERROR)\n", result, ORM_ERROR);
    
    printf("\nThese will be implemented when database backends (SQLite, PostgreSQL, MySQL)\n");
    printf("are added in Tasks 06, 13, and 14.\n");
    
    User_free(user);
}

void example_field_access(void) {
    print_separator("Example 5: Dynamic Field Access");
    
    Category* cat = Category_new();
    cat->id = 1;
    strcpy(cat->name, "Technology");
    strcpy(cat->description, "Tech-related posts");
    
    ORM_ModelMeta* meta = ORM_GetModel("Category");
    
    printf("Accessing fields dynamically:\n\n");
    
    // Get ID field value
    ORM_FieldMeta* id_field = ORM_GetField(meta, "id");
    int* id_ptr = (int*)ORM_GetFieldValue(cat, id_field);
    printf("  ID (via dynamic access): %d\n", *id_ptr);
    
    // Get name field value
    ORM_FieldMeta* name_field = ORM_GetField(meta, "name");
    char* name_ptr = (char*)ORM_GetFieldValue(cat, name_field);
    printf("  Name (via dynamic access): %s\n", name_ptr);
    
    // Get description field value
    ORM_FieldMeta* desc_field = ORM_GetField(meta, "description");
    char* desc_ptr = (char*)ORM_GetFieldValue(cat, desc_field);
    printf("  Description (via dynamic access): %s\n", desc_ptr);
    
    printf("\nThis allows generic code to work with any model!\n");
    
    Category_free(cat);
}

void example_registry_operations(void) {
    print_separator("Example 6: Registry Operations");
    
    printf("Registered models: %d\n\n", ORM_GetModelCount());
    
    printf("Model details:\n");
    for (int i = 0; i < g_orm_registry->count; i++) {
        ORM_ModelMeta* meta = g_orm_registry->models[i];
        printf("  [%d] %-15s -> table: %-15s (%d fields, %zu bytes)\n",
               i,
               meta->model_name,
               meta->table_name,
               meta->field_count,
               meta->struct_size);
    }
}

void example_constraint_checking(void) {
    print_separator("Example 7: Constraint Checking");
    
    ORM_ModelMeta* user_meta = ORM_GetModel("User");
    
    printf("Checking constraints on User model:\n\n");
    
    for (int i = 0; i < user_meta->field_count; i++) {
        ORM_FieldMeta* field = &user_meta->fields[i];
        printf("Field: %-15s\n", field->field_name);
        
        if (ORM_FieldHasFlag(field, ORM_PRIMARY_KEY)) {
            printf("  ✓ PRIMARY KEY\n");
        }
        if (ORM_FieldHasFlag(field, ORM_AUTO_INCREMENT)) {
            printf("  ✓ AUTO INCREMENT\n");
        }
        if (ORM_FieldHasFlag(field, ORM_NOT_NULL)) {
            printf("  ✓ NOT NULL\n");
        }
        if (ORM_FieldHasFlag(field, ORM_UNIQUE)) {
            printf("  ✓ UNIQUE\n");
        }
        if (ORM_FieldHasFlag(field, ORM_INDEX)) {
            printf("  ✓ INDEXED\n");
        }
        if (ORM_FieldHasFlag(field, ORM_HAS_DEFAULT)) {
            printf("  ✓ HAS DEFAULT\n");
        }
        
        printf("\n");
    }
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    printf("================================================================================\n");
    printf("ORM ADVANCED FEATURES DEMONSTRATION\n");
    printf("================================================================================\n");
    
    // Initialize ORM
    ORM_Init();
    
    // Register models
    ORM_RegisterModel(&BlogPost_meta);
    ORM_RegisterModel(&Category_meta);
    ORM_RegisterModel(&User_meta);
    
    // Run examples
    example_all_field_types();
    example_model_introspection();
    example_default_values();
    example_crud_functions();
    example_field_access();
    example_registry_operations();
    example_constraint_checking();
    
    // Cleanup
    print_separator("Cleanup");
    printf("Shutting down ORM...\n");
    ORM_Shutdown();
    printf("Done!\n\n");
    
    return 0;
}
