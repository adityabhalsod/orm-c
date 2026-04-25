#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/core/model.h"

// ============================================================================
// Model Definitions
// ============================================================================

// User model
typedef struct {
    int id;
    char username[50];
    char email[100];
    int age;
    int is_active;
} User;

#undef ORM_CURRENT_MODEL
#define ORM_CURRENT_MODEL User

ORM_MODEL_DEFINE(User, "users",
    ORM_FIELD_INT(id, ORM_PRIMARY_KEY | ORM_AUTO_INCREMENT),
    ORM_FIELD_STRING(username, 50, ORM_NOT_NULL | ORM_UNIQUE),
    ORM_FIELD_STRING(email, 100, ORM_NOT_NULL),
    ORM_FIELD_INT(age, 0),
    ORM_FIELD_BOOLEAN(is_active, 0)
);

// Product model
typedef struct {
    int id;
    char name[100];
    char description[500];
    double price;
    int stock;
} Product;

#undef ORM_CURRENT_MODEL
#define ORM_CURRENT_MODEL Product

ORM_MODEL_DEFINE(Product, "products",
    ORM_FIELD_INT(id, ORM_PRIMARY_KEY | ORM_AUTO_INCREMENT),
    ORM_FIELD_STRING(name, 100, ORM_NOT_NULL),
    ORM_FIELD_STRING(description, 500, 0),
    ORM_FIELD_DOUBLE(price, ORM_NOT_NULL),
    ORM_FIELD_INT(stock, ORM_NOT_NULL)
);

// Order model
typedef struct {
    int id;
    int user_id;
    int product_id;
    int quantity;
    double total_price;
} Order;

#undef ORM_CURRENT_MODEL
#define ORM_CURRENT_MODEL Order

ORM_MODEL_DEFINE(Order, "orders",
    ORM_FIELD_INT(id, ORM_PRIMARY_KEY | ORM_AUTO_INCREMENT),
    ORM_FIELD_INT(user_id, ORM_NOT_NULL),
    ORM_FIELD_INT(product_id, ORM_NOT_NULL),
    ORM_FIELD_INT(quantity, ORM_NOT_NULL),
    ORM_FIELD_DOUBLE(total_price, ORM_NOT_NULL)
);

// ============================================================================
// Example Functions
// ============================================================================

void print_separator(void) {
    printf("\n");
    printf("================================================================================\n");
}

void example_create_instances(void) {
    print_separator();
    printf("Example 1: Creating Model Instances\n");
    print_separator();
    
    // Create a user
    User* user = (User*)User_new();
    user->id = 1;
    strcpy(user->username, "alice");
    strcpy(user->email, "alice@example.com");
    user->age = 28;
    user->is_active = 1;
    
    printf("Created user:\n");
    printf("  ID: %d\n", user->id);
    printf("  Username: %s\n", user->username);
    printf("  Email: %s\n", user->email);
    printf("  Age: %d\n", user->age);
    printf("  Active: %s\n", user->is_active ? "Yes" : "No");
    
    // Create a product
    Product* product = (Product*)Product_new();
    product->id = 1;
    strcpy(product->name, "Laptop");
    strcpy(product->description, "High-performance laptop with 16GB RAM");
    product->price = 999.99;
    product->stock = 50;
    
    printf("\nCreated product:\n");
    printf("  ID: %d\n", product->id);
    printf("  Name: %s\n", product->name);
    printf("  Description: %s\n", product->description);
    printf("  Price: $%.2f\n", product->price);
    printf("  Stock: %d\n", product->stock);
    
    // Clean up
    User_free(user);
    Product_free(product);
}

void example_model_introspection(void) {
    print_separator();
    printf("Example 2: Model Introspection\n");
    print_separator();
    
    // Get User model metadata
    ORM_ModelMeta* user_meta = ORM_GetModel("User");
    if (user_meta) {
        printf("Model: %s\n", user_meta->model_name);
        printf("Table: %s\n", user_meta->table_name);
        printf("Size: %zu bytes\n", user_meta->struct_size);
        printf("Fields: %d\n\n", user_meta->field_count);
        
        // Print all fields
        printf("Field Details:\n");
        for (int i = 0; i < user_meta->field_count; i++) {
            ORM_FieldMeta* field = &user_meta->fields[i];
            printf("  [%d] %s\n", i, field->field_name);
            printf("      Type: %s\n", ORM_FieldTypeToString(field->type));
            printf("      Column: %s\n", field->column_name);
            printf("      Size: %zu bytes\n", field->size);
            printf("      Offset: %zu\n", field->offset);
            printf("      Flags: 0x%02X", field->flags);
            
            // Print flag meanings
            if (field->flags) {
                printf(" (");
                if (field->flags & ORM_PRIMARY_KEY) printf(" PRIMARY_KEY");
                if (field->flags & ORM_AUTO_INCREMENT) printf(" AUTO_INCREMENT");
                if (field->flags & ORM_NOT_NULL) printf(" NOT_NULL");
                if (field->flags & ORM_UNIQUE) printf(" UNIQUE");
                if (field->flags & ORM_INDEX) printf(" INDEX");
                printf(" )");
            }
            printf("\n");
        }
    }
}

void example_field_lookup(void) {
    print_separator();
    printf("Example 3: Field Lookup and Access\n");
    print_separator();
    
    ORM_ModelMeta* meta = ORM_GetModel("Product");
    
    // Get primary key
    ORM_FieldMeta* pk = ORM_GetPrimaryKey(meta);
    if (pk) {
        printf("Primary Key: %s\n", pk->field_name);
    }
    
    // Look up specific fields
    ORM_FieldMeta* name_field = ORM_GetField(meta, "name");
    if (name_field) {
        printf("\nName field:\n");
        printf("  Type: %s\n", ORM_FieldTypeToString(name_field->type));
        printf("  Max size: %zu\n", name_field->size);
        printf("  Required: %s\n", 
               ORM_FieldHasFlag(name_field, ORM_NOT_NULL) ? "Yes" : "No");
    }
    
    ORM_FieldMeta* price_field = ORM_GetField(meta, "price");
    if (price_field) {
        printf("\nPrice field:\n");
        printf("  Type: %s\n", ORM_FieldTypeToString(price_field->type));
        printf("  Required: %s\n",
               ORM_FieldHasFlag(price_field, ORM_NOT_NULL) ? "Yes" : "No");
    }
}

void example_dynamic_field_access(void) {
    print_separator();
    printf("Example 4: Dynamic Field Value Access\n");
    print_separator();
    
    // Create a product
    Product product;
    product.id = 42;
    strcpy(product.name, "Wireless Mouse");
    product.price = 29.99;
    product.stock = 100;
    
    ORM_ModelMeta* meta = ORM_GetModel("Product");
    
    // Access fields dynamically
    printf("Accessing fields dynamically:\n");
    
    ORM_FieldMeta* id_field = ORM_GetField(meta, "id");
    int* id_ptr = (int*)ORM_GetFieldValue(&product, id_field);
    printf("  ID: %d\n", *id_ptr);
    
    ORM_FieldMeta* name_field = ORM_GetField(meta, "name");
    char* name_ptr = (char*)ORM_GetFieldValue(&product, name_field);
    printf("  Name: %s\n", name_ptr);
    
    ORM_FieldMeta* price_field = ORM_GetField(meta, "price");
    double* price_ptr = (double*)ORM_GetFieldValue(&product, price_field);
    printf("  Price: $%.2f\n", *price_ptr);
    
    ORM_FieldMeta* stock_field = ORM_GetField(meta, "stock");
    int* stock_ptr = (int*)ORM_GetFieldValue(&product, stock_field);
    printf("  Stock: %d\n", *stock_ptr);
}

void example_registry_operations(void) {
    print_separator();
    printf("Example 5: Registry Operations\n");
    print_separator();
    
    printf("Total models registered: %d\n\n", ORM_GetModelCount());
    
    printf("Registered models:\n");
    for (int i = 0; i < ORM_GetModelCount(); i++) {
        ORM_ModelMeta* model = g_orm_registry->models[i];
        printf("  [%d] %s -> %s (%d fields)\n",
               i, model->model_name, model->table_name, model->field_count);
    }
}

void example_field_validation(void) {
    print_separator();
    printf("Example 6: Field Validation Check\n");
    print_separator();
    
    ORM_ModelMeta* user_meta = ORM_GetModel("User");
    
    printf("Checking User model constraints:\n\n");
    
    // Check username field
    ORM_FieldMeta* username = ORM_GetField(user_meta, "username");
    printf("username field:\n");
    printf("  NOT NULL: %s\n", 
           ORM_FieldHasFlag(username, ORM_NOT_NULL) ? "Yes" : "No");
    printf("  UNIQUE: %s\n",
           ORM_FieldHasFlag(username, ORM_UNIQUE) ? "Yes" : "No");
    printf("  Max length: %zu\n", username->size);
    
    // Check email field
    ORM_FieldMeta* email = ORM_GetField(user_meta, "email");
    printf("\nemail field:\n");
    printf("  NOT NULL: %s\n",
           ORM_FieldHasFlag(email, ORM_NOT_NULL) ? "Yes" : "No");
    printf("  UNIQUE: %s\n",
           ORM_FieldHasFlag(email, ORM_UNIQUE) ? "Yes" : "No");
    printf("  Max length: %zu\n", email->size);
    
    // Check id field
    ORM_FieldMeta* id = ORM_GetField(user_meta, "id");
    printf("\nid field:\n");
    printf("  PRIMARY KEY: %s\n",
           ORM_FieldHasFlag(id, ORM_PRIMARY_KEY) ? "Yes" : "No");
    printf("  AUTO INCREMENT: %s\n",
           ORM_FieldHasFlag(id, ORM_AUTO_INCREMENT) ? "Yes" : "No");
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    printf("\n");
    printf("********************************************************************************\n");
    printf("*                         ORM-C EXAMPLE USAGE                                 *\n");
    printf("********************************************************************************\n");
    
    // Initialize ORM
    printf("\nInitializing ORM...\n");
    ORM_Init();
    
    // Register models
    printf("Registering models...\n");
    ORM_RegisterModel(&User_meta);
    ORM_RegisterModel(&Product_meta);
    ORM_RegisterModel(&Order_meta);
    printf("Models registered successfully!\n");
    
    // Run examples
    example_create_instances();
    example_model_introspection();
    example_field_lookup();
    example_dynamic_field_access();
    example_registry_operations();
    example_field_validation();
    
    // Cleanup
    print_separator();
    printf("Cleaning up...\n");
    ORM_Shutdown();
    printf("ORM shutdown complete.\n");
    
    print_separator();
    printf("All examples completed successfully!\n");
    print_separator();
    printf("\n");
    
    return 0;
}
