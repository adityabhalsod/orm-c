# Task 10: Validation Framework

## Objective
Implement a comprehensive validation framework with field-level and model-level validators, custom validation rules, and clear error reporting for data integrity.

## Deliverables
1. Validation rule system
2. Built-in validators
3. Custom validator support
4. Error collection and reporting
5. Validation hooks in model lifecycle

## Input Requirements
- Field values and types
- Validation rules and constraints
- Custom validation functions
- Model state
- Error message templates

## Output Specifications
- Validation results (pass/fail)
- Detailed error messages
- Field-specific errors
- Validation error collections
- Suggested corrections

## Implementation Guide

### 1. Validation Structure
```c
typedef struct {
    bool (*validate)(void* value, void* params);
    char* error_message;
    void* params;
} ORM_Validator;

typedef struct {
    char* field;
    char* message;
    char* code;
    void* invalid_value;
} ORM_ValidationError;

typedef struct {
    ORM_ValidationError** errors;
    int error_count;
    int capacity;
    bool has_errors;
} ORM_ValidationResult;

// Validation context
typedef struct {
    ORM_ModelMeta* model;
    void* instance;
    ORM_ValidationResult* result;
    bool partial;  // For partial updates
} ORM_ValidationContext;
```

### 2. Built-in Validators
```c
// Required validator
bool ORM_ValidateRequired(void* value, void* params);

// Length validators
typedef struct {
    int min;
    int max;
} ORM_LengthParams;
bool ORM_ValidateLength(void* value, void* params);

// Range validators
typedef struct {
    double min;
    double max;
} ORM_RangeParams;
bool ORM_ValidateRange(void* value, void* params);

// Pattern validator
typedef struct {
    const char* pattern;
    int flags;
} ORM_RegexParams;
bool ORM_ValidateRegex(void* value, void* params);

// Email validator
bool ORM_ValidateEmail(void* value, void* params);

// URL validator
bool ORM_ValidateURL(void* value, void* params);

// Choice validator
typedef struct {
    void** choices;
    int choice_count;
} ORM_ChoiceParams;
bool ORM_ValidateChoice(void* value, void* params);

// Unique validator
bool ORM_ValidateUnique(void* value, void* params);
```

### 3. Validator Registration
```c
// Field validator registration
#define ORM_FIELD_VALIDATORS(field, ...) \
    static ORM_Validator field##_validators[] = { __VA_ARGS__ }

// Common validator macros
#define REQUIRED() \
    {.validate = ORM_ValidateRequired, .error_message = "This field is required"}

#define MIN_LENGTH(n) \
    {.validate = ORM_ValidateLength, \
     .params = &(ORM_LengthParams){.min = n, .max = INT_MAX}, \
     .error_message = "Minimum length is " #n}

#define MAX_LENGTH(n) \
    {.validate = ORM_ValidateLength, \
     .params = &(ORM_LengthParams){.min = 0, .max = n}, \
     .error_message = "Maximum length is " #n}

#define RANGE(min, max) \
    {.validate = ORM_ValidateRange, \
     .params = &(ORM_RangeParams){.min = min, .max = max}, \
     .error_message = "Value must be between " #min " and " #max}

#define EMAIL() \
    {.validate = ORM_ValidateEmail, .error_message = "Enter a valid email"}
```

### 4. Model Validation
```c
// Model validation hooks
typedef struct {
    bool (*clean)(void* instance, ORM_ValidationContext* ctx);
    bool (*clean_fields)(void* instance, ORM_ValidationContext* ctx);
    bool (*validate_unique)(void* instance, ORM_ValidationContext* ctx);
} ORM_ModelValidation;

// Full model validation
ORM_ValidationResult* ORM_ValidateModel(void* instance);

// Partial validation
ORM_ValidationResult* ORM_ValidateFields(
    void* instance, 
    const char** fields, 
    int field_count
);

// Clean data (transform and validate)
bool ORM_CleanModel(void* instance);
```

### 5. Custom Validators
```c
// Custom validator function signature
typedef bool (*ORM_CustomValidator)(
    void* value,
    void* instance,
    ORM_ValidationContext* ctx
);

// Register custom validator
void ORM_RegisterValidator(
    const char* name,
    ORM_CustomValidator validator
);

// Use custom validator
#define CUSTOM(name) \
    {.validate = ORM_GetValidator(#name), \
     .error_message = "Custom validation failed"}

// Cross-field validation
bool ORM_ValidateCrossFields(
    void* instance,
    const char** fields,
    int field_count,
    ORM_CustomValidator validator
);
```

### 6. Error Handling
```c
// Error management
void ORM_AddError(
    ORM_ValidationResult* result,
    const char* field,
    const char* message,
    const char* code
);

void ORM_ClearErrors(ORM_ValidationResult* result);

char* ORM_FormatErrors(ORM_ValidationResult* result);

// Error checking
bool ORM_HasErrors(ORM_ValidationResult* result);
bool ORM_HasFieldError(ORM_ValidationResult* result, const char* field);
ORM_ValidationError* ORM_GetFieldError(
    ORM_ValidationResult* result,
    const char* field
);

// JSON error format
char* ORM_ErrorsToJSON(ORM_ValidationResult* result);
```

## Testing Requirements

### Unit Tests
```c
void test_required_validator() {
    ORM_ValidationResult* result = ORM_CreateValidationResult();
    
    // Test NULL value
    char* null_value = NULL;
    bool valid = ORM_ValidateRequired(null_value, NULL);
    assert(!valid);
    
    // Test empty string
    char* empty = "";
    valid = ORM_ValidateRequired(empty, NULL);
    assert(!valid);
    
    // Test valid value
    char* value = "test";
    valid = ORM_ValidateRequired(value, NULL);
    assert(valid);
    
    ORM_FreeValidationResult(result);
}

void test_length_validator() {
    ORM_LengthParams params = {.min = 3, .max = 10};
    
    assert(!ORM_ValidateLength("ab", &params));      // Too short
    assert(ORM_ValidateLength("test", &params));     // Valid
    assert(!ORM_ValidateLength("very long string", &params)); // Too long
}

void test_email_validator() {
    assert(ORM_ValidateEmail("user@example.com", NULL));
    assert(ORM_ValidateEmail("user+tag@example.co.uk", NULL));
    assert(!ORM_ValidateEmail("invalid", NULL));
    assert(!ORM_ValidateEmail("@example.com", NULL));
    assert(!ORM_ValidateEmail("user@", NULL));
}

void test_model_validation() {
    // Create user with invalid data
    User* user = User_new();
    user->age = -5;  // Invalid age
    strcpy(user->email, "invalid-email");
    
    // Validate
    ORM_ValidationResult* result = ORM_ValidateModel(user);
    
    assert(ORM_HasErrors(result));
    assert(ORM_HasFieldError(result, "age"));
    assert(ORM_HasFieldError(result, "email"));
    
    // Fix errors
    user->age = 25;
    strcpy(user->email, "user@example.com");
    
    result = ORM_ValidateModel(user);
    assert(!ORM_HasErrors(result));
    
    ORM_FreeValidationResult(result);
    User_free(user);
}

void test_custom_validator() {
    // Register custom validator
    ORM_RegisterValidator("password_strength", validate_password_strength);
    
    // Use in model
    char* weak_password = "123";
    char* strong_password = "Str0ng!Pass";
    
    ORM_CustomValidator validator = ORM_GetValidator("password_strength");
    assert(!validator(weak_password, NULL, NULL));
    assert(validator(strong_password, NULL, NULL));
}
```

## Integration Points
- **Model System**: Integrates with model save/update
- **Field Types**: Validates based on field types
- **Query Builder**: Validates query parameters
- **Forms**: Provides validation for form inputs
- **API**: Returns validation errors

## Success Criteria
- [ ] All built-in validators work correctly
- [ ] Custom validators can be registered
- [ ] Model validation prevents invalid saves
- [ ] Clear error messages generated
- [ ] Cross-field validation supported
- [ ] No memory leaks in error handling
- [ ] < 1ms validation for typical models

## Example Usage
```c
// Define model with validators
typedef struct {
    int id;
    char username[50];
    char email[255];
    int age;
    char password[100];
} User;

// Register validators
ORM_FIELD_VALIDATORS(username,
    REQUIRED(),
    MIN_LENGTH(3),
    MAX_LENGTH(50),
    CUSTOM(no_profanity)
);

ORM_FIELD_VALIDATORS(email,
    REQUIRED(),
    EMAIL(),
    UNIQUE()
);

ORM_FIELD_VALIDATORS(age,
    REQUIRED(),
    RANGE(0, 150)
);

// Validate before save
User* user = User_new();
strcpy(user->username, "jo");  // Too short
strcpy(user->email, "invalid");
user->age = -5;

ORM_ValidationResult* result = ORM_ValidateModel(user);
if (ORM_HasErrors(result)) {
    char* errors = ORM_FormatErrors(result);
    printf("Validation errors:\n%s\n", errors);
    free(errors);
} else {
    User_save(user);
}

// Custom model validation
bool User_clean(User* user, ORM_ValidationContext* ctx) {
    // Cross-field validation
    if (user->age < 18 && contains_adult_content(user->bio)) {
        ORM_AddError(ctx->result, "bio", 
                    "Minors cannot have adult content", "age_content");
        return false;
    }
    return true;
}
```

## Files to Create
- `src/validation/validator.h` - Validation interface
- `src/validation/validator.c` - Core validation
- `src/validation/builtin.c` - Built-in validators
- `src/validation/custom.c` - Custom validator support
- `src/validation/errors.c` - Error handling
- `tests/test_validation.c` - Validation tests
