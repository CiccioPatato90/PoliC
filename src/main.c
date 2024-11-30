#include <stdio.h>
#include <string.h>
#include "processing.h"
#include "csv_parser.h"
#include "db_manager.h"

int main(int argc, char **argv) {

    DBManager *db_manager = create_db_manager(DB_TYPE_SQLITE);

    //&& db_manager->initialize() == 0
    if (db_manager){
        //if valid connection retrieved from manager
        if ((argc > 1 && strcmp(argv[1], "loadredis") == 0) && check_redis_populated() == 0) {
            parse_csv_file("res/processed_data.csv", db_manager->get_connection());
        }
        else if ((argc > 1 && strcmp(argv[1], "kmeans") == 0)) {
            compute_kpp(db_manager, 0, 4, 0, 0.0);
        }
        else {
            // Regular execution
            printf("Running main program...\n");
        }
        return 0;
    }
}
