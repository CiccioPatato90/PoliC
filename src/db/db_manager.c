#include "db_manager.h"

// Forward declarations of specific DB manager creation functions
//DBManager *create_redis_manager();
DBManager *create_sqlite_manager();

DBManager *create_db_manager(DBType db_type) {
    switch (db_type) {
        /*case DB_TYPE_REDIS:
            return create_redis_manager();*/
        case DB_TYPE_SQLITE:
            return create_sqlite_manager();
        default:
            return (DBManager *) 0;
    }
}
