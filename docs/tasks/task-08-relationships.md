# Task 08: Relationships and Foreign Keys

## Objective
Implement a comprehensive relationship system supporting one-to-one, one-to-many, and many-to-many relationships with foreign key constraints, cascade operations, and efficient loading strategies.

## Deliverables
1. Foreign key field implementation
2. Relationship definitions
3. Lazy and eager loading
4. Cascade operations
5. Reverse relationships

## Input Requirements
- Model relationships definitions
- Foreign key constraints
- Loading strategies
- Cascade rules
- Join specifications

## Output Specifications
- Related object access
- Automatic joins
- Cascade delete/update
- Relationship validation
- Query optimization

## Implementation Guide

### 1. Relationship Types
```c
typedef enum {
    REL_ONE_TO_ONE,
    REL_ONE_TO_MANY,
    REL_MANY_TO_ONE,
    REL_MANY_TO_MANY
} ORM_RelationType;

typedef struct {
    ORM_RelationType type;
    const char* field_name;
    const char* related_model;
    const char* related_field;
    const char* through_table;  // For M2M
    bool cascade_delete;
    bool cascade_update;
    bool nullable;
} ORM_Relationship;

// Define relationships
#define ORM_FOREIGN_KEY(field, model, ...) \
    { .type = REL_MANY_TO_ONE, \
      .field_name = #field, \
      .related_model = #model, \
      __VA_ARGS__ }

#define ORM_ONE_TO_MANY(field, model, related) \
    { .type = REL_ONE_TO_MANY, \
      .field_name = #field, \
      .related_model = #model, \
      .related_field = #related }
```

### 2. Foreign Key Fields
```c
typedef struct {
    int id;  // Foreign key value
    void* cached_object;  // Cached related object
    bool is_loaded;
    ORM_Relationship* relationship;
} ORM_ForeignKey;

// Foreign key operations
void* ORM_GetRelated(void* model, const char* field_name);
void ORM_SetRelated(void* model, const char* field_name, void* related);
bool ORM_LoadRelated(void* model, const char* field_name);
void ORM_ClearRelatedCache(void* model);
```

### 3. Loading Strategies
```c
// Lazy loading
typedef struct {
    void* (*loader)(int id);
    int foreign_key_id;
    bool is_loaded;
    void* cached_value;
} ORM_LazyLoader;

void* ORM_LazyLoad(ORM_LazyLoader* loader);

// Eager loading with select_related
QuerySet* ORM_SelectRelated(QuerySet* qs, const char* fields);

// Prefetch for avoiding N+1 queries
QuerySet* ORM_PrefetchRelated(QuerySet* qs, const char* fields);

// Loading strategy configuration
typedef enum {
    LOAD_LAZY,      // Load on access
    LOAD_EAGER,     // Load immediately
    LOAD_NONE       // Don't load
} ORM_LoadStrategy;
```

### 4. Many-to-Many Support
```c
typedef struct {
    const char* through_table;
    const char* left_field;
    const char* right_field;
    void** cached_objects;
    int cached_count;
} ORM_ManyToMany;

// M2M operations
QuerySet* ORM_GetM2MRelated(void* model, const char* field);
void ORM_AddM2M(void* model, const char* field, void* related);
void ORM_RemoveM2M(void* model, const char* field, void* related);
void ORM_ClearM2M(void* model, const char* field);
void ORM_SetM2M(void* model, const char* field, void** related, int count);
```

### 5. Cascade Operations
```c
typedef enum {
    CASCADE_NOTHING,    // Do nothing
    CASCADE_DELETE,     // Delete related
    CASCADE_SET_NULL,   // Set FK to NULL
    CASCADE_SET_DEFAULT,// Set to default
    CASCADE_RESTRICT    // Prevent deletion
} ORM_CascadeAction;

// Cascade handlers
int ORM_CascadeDelete(void* model, ORM_Relationship* rel);
int ORM_CascadeUpdate(void* model, ORM_Relationship* rel);
bool ORM_CheckCascadeConstraints(void* model);
```

### 6. Reverse Relationships
```c
// Automatic reverse relationship
typedef struct {
    const char* accessor_name;
    ORM_ModelMeta* related_model;
    const char* foreign_key_field;
} ORM_ReverseRelation;

// Access reverse relationships
QuerySet* ORM_GetReverseRelated(void* model, const char* accessor);

// Example: author.posts_set->all()
#define ORM_RELATED_NAME(model, name) \
    model##_##name##_set

// Register reverse relation
void ORM_RegisterReverseRelation(
    ORM_ModelMeta* model,
    const char* field,
    ORM_ModelMeta* related,
    const char* accessor
);
```

## Testing Requirements

### Unit Tests
```c
void test_foreign_key() {
    // Create related objects
    Author* author = Author_create(.name = "John");
    Post* post = Post_create(
        .title = "Test Post",
        .author_id = author->id
    );
    
    // Test lazy loading
    Author* loaded = ORM_GetRelated(post, "author");
    assert(loaded != NULL);
    assert(loaded->id == author->id);
    
    // Test caching
    Author* cached = ORM_GetRelated(post, "author");
    assert(cached == loaded); // Same pointer
}

void test_many_to_many() {
    Post* post = Post_create(.title = "Test");
    Tag* tag1 = Tag_create(.name = "python");
    Tag* tag2 = Tag_create(.name = "django");
    
    // Add M2M relations
    ORM_AddM2M(post, "tags", tag1);
    ORM_AddM2M(post, "tags", tag2);
    
    // Query M2M
    QuerySet* tags = ORM_GetM2MRelated(post, "tags");
    assert(ORM_QuerySetCount(tags) == 2);
    
    // Remove M2M
    ORM_RemoveM2M(post, "tags", tag1);
    tags = ORM_GetM2MRelated(post, "tags");
    assert(ORM_QuerySetCount(tags) == 1);
}

void test_cascade_delete() {
    Author* author = Author_create(.name = "John");
    Post* post1 = Post_create(.author_id = author->id);
    Post* post2 = Post_create(.author_id = author->id);
    
    // Delete author should cascade to posts
    Author_delete(author);
    
    assert(Post_get(post1->id) == NULL);
    assert(Post_get(post2->id) == NULL);
}

void test_select_related() {
    // Create test data
    create_test_authors_and_posts();
    
    // Without select_related (N+1 queries)
    QuerySet* posts = Post_objects()->all();
    
    // With select_related (1 query)
    QuerySet* optimized = Post_objects()
        ->select_related("author")
        ->all();
    
    // Both should return same data
    assert(ORM_QuerySetCount(posts) == ORM_QuerySetCount(optimized));
}
```

## Integration Points
- **Model System**: Extends model fields with relationships
- **Query Builder**: Adds JOIN support
- **SQL Generation**: Creates JOIN clauses
- **Result Sets**: Handles nested objects
- **Migrations**: Creates foreign key constraints

## Success Criteria
- [ ] All relationship types work correctly
- [ ] Lazy loading prevents unnecessary queries
- [ ] Select_related reduces queries to 1
- [ ] Cascade operations maintain integrity
- [ ] M2M relationships work both directions
- [ ] No N+1 query problems
- [ ] Foreign key constraints enforced

## Example Usage
```c
// Define models with relationships
typedef struct {
    int id;
    char name[100];
} Author;

typedef struct {
    int id;
    char title[200];
    int author_id;  // Foreign key
    ORM_ForeignKey author;
} Post;

ORM_MODEL_DEFINE(Author, "authors",
    ORM_FIELD_INT(id, PRIMARY_KEY),
    ORM_FIELD_STRING(name, 100, NOT_NULL)
);

ORM_MODEL_DEFINE(Post, "posts",
    ORM_FIELD_INT(id, PRIMARY_KEY),
    ORM_FIELD_STRING(title, 200, NOT_NULL),
    ORM_FOREIGN_KEY(author, Author, 
        .cascade_delete = true,
        .related_name = "posts"
    )
);

// Use relationships
Post* post = Post_get(1);
Author* author = ORM_GetRelated(post, "author");
printf("Post by: %s\n", author->name);

// Reverse relationship
QuerySet* author_posts = ORM_GetReverseRelated(author, "posts");

// Eager loading
QuerySet* posts = Post_objects()
    ->select_related("author")
    ->filter("published", "=", true);

// Many-to-many
ORM_AddM2M(post, "tags", tag);
QuerySet* post_tags = ORM_GetM2MRelated(post, "tags");
```

## Files to Create
- `src/core/relationships.h` - Relationship definitions
- `src/core/relationships.c` - Core implementation
- `src/core/foreign_key.c` - Foreign key support
- `src/core/many_to_many.c` - M2M implementation
- `src/core/cascade.c` - Cascade operations
- `tests/test_relationships.c` - Relationship tests
