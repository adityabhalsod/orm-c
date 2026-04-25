# C-ORM Library Implementation Prompt - Master Document

## Project Mission
Create a Django ORM-inspired Object-Relational Mapping library written in pure C with zero external dependencies, providing an intuitive API for database operations while maintaining high performance and memory safety.

## Task-Based Development Approach

This project is divided into focused, manageable tasks. Each task has its own detailed prompt document that can be used independently for implementation:

### Core Tasks
1. **[Model System](./tasks/task-01-model-system.md)** - Model definition and registration
2. **[Field Types](./tasks/task-02-field-types.md)** - Comprehensive field type system
3. **[Memory Management](./tasks/task-03-memory-management.md)** - Custom memory pools and lifecycle
4. **[Query Builder](./tasks/task-04-query-builder.md)** - Fluent API for queries
5. **[SQL Generation](./tasks/task-05-sql-generation.md)** - SQL statement construction
6. **[SQLite Backend](./tasks/task-06-sqlite-backend.md)** - SQLite database integration
7. **[Result Sets](./tasks/task-07-result-sets.md)** - Query result handling
8. **[Relationships](./tasks/task-08-relationships.md)** - Foreign keys and associations
9. **[Migrations](./tasks/task-09-migrations.md)** - Schema management
10. **[Validation](./tasks/task-10-validation.md)** - Data validation framework
11. **[Transactions](./tasks/task-11-transactions.md)** - Transaction management
12. **[Connection Pool](./tasks/task-12-connection-pool.md)** - Connection pooling
13. **[PostgreSQL Backend](./tasks/task-13-postgresql-backend.md)** - PostgreSQL support
14. **[MySQL Backend](./tasks/task-14-mysql-backend.md)** - MySQL support
15. **[Performance](./tasks/task-15-performance.md)** - Optimization and caching

## How to Use Task Prompts

Each task prompt is self-contained and includes:
- Clear objectives and deliverables
- Input/output specifications
- Implementation guidelines
- Testing requirements
- Integration points with other components

Use each prompt independently when working on that specific component, or follow them sequentially for complete implementation.

## Development Priorities

1. **Correctness over performance** - Ensure data integrity first
2. **Simplicity over features** - Keep API intuitive
3. **Safety over convenience** - Prevent memory leaks and crashes
4. **Portability over optimization** - Work across platforms
5. **Documentation over assumptions** - Be explicit

## Success Metrics

- Zero external dependencies achieved
- All CRUD operations functional
- Memory leak-free (Valgrind clean)
- Cross-platform compilation
- < 500KB compiled library size
- > 100 operations/second throughput
- Complete API documentation
- 10+ working examples
