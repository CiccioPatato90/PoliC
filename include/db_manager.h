#ifndef DB_MANAGER_H
#define DB_MANAGER_H
#include <model.h>

typedef enum { DB_TYPE_REDIS, DB_TYPE_SQLITE } DBType;

typedef struct {
    int (*initialize)();
    void *(*get_connection)();
    void (*close_connection)(void *connection);
    void (*cleanup)();
    int (*count)(int* labels);
    RowArray* (*select_rows)(char* query, int include_label);
    void (*insert_rows)(RowArray* rows);
} DBManager;

// Function to create a new DBManager for a specific database type
DBManager *create_db_manager(DBType db_type);
int check_redis_populated();

#endif

