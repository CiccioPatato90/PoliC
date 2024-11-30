#include <model.h>
#include "db_manager.h"
#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DBNAME "bin/records.db"
#define RECLUSTERED_TABLE "reclustered_energy_records"
#define LABEL_COL 25
#define POD_ID_COL 0

int sqlite_count(int* labels);
int int_callback(void *int_ptr, int argc, char **argv, char **azColName);
sqlite3* open_connection();
RowArray* select_rows(char* sql, int include_label);
void close_connection(void* db);
int string_to_int(const char *str) {
    int result = 0;
    if (sscanf(str, "%d", &result) != 1) {
        // Handle error if the conversion fails
        printf("Conversion failed or input was not a valid integer.\n");
        return 0; // Default or error value
    }
    return result;
}
int int_callback(void *int_ptr, int argc, char **argv, char **azColName){
    // azColName stores teh query and argv[i] the i-th result

    for (int i = 0; i < argc; i++) {
        *(int *)int_ptr = string_to_int(argv[0]);
    }

    printf("\n");

    return 0;
}
void print_error(const char *format, sqlite3 *db) {
    printf(format, sqlite3_errmsg(db));
    printf("\n");
}

int sqlite_count(int* labels){
    sqlite3 *db = open_connection();
    char *zErrMsg = 0;
    int rc;
    int result = 0;
    char *sql = "SELECT COUNT (*) FROM energy_records;";

    rc = sqlite3_exec(db, sql, int_callback, &result, &zErrMsg);

    if (rc != SQLITE_OK ) {
        fprintf(stderr, "Failed to select data\n");
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        close_connection(db);
        return 0;
    }

    close_connection(db);

    printf("Count of rows in energy_records: %d\n", result);  // Print the count
    return result;
}

RowArray* select_rows(char* sql, int include_label) {
    sqlite3 *db = open_connection();
    char *zErrMsg = 0;
    int rc;
    RowArray *result_array = init_row_array();
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK || stmt == NULL) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        free_row_array(result_array);  // Clean up on failure
        sqlite3_close(db);
        return NULL;
    }

    if(include_label) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Row row;
            memset(&row, 0, sizeof(Row));  // Initialize all fields to zero

            // Extract column 0 as an integer
            row.pod_id = sqlite3_column_int(stmt, POD_ID_COL);

            // Extract columns 1 to 25 as floats into the `values` array
            for (int i = 0; i < 25; i++) {
                row.a[i] = (float)sqlite3_column_double(stmt, i + 1);
            }

            // Extract column 26 as the label (integer)
            row.label = sqlite3_column_int(stmt, LABEL_COL);
            add_row(result_array, &row);
        }
    }
    else {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Row row;
            memset(&row, 0, sizeof(Row));  // Initialize all fields to zero

            // Extract column 0 as an integer
            row.pod_id = sqlite3_column_int(stmt, POD_ID_COL);

            // Extract columns 1 to 25 as floats into the `values` array
            for (int i = 0; i < 25; i++) {
                row.a[i] = (float)sqlite3_column_double(stmt, i + 1);
            }

            //do not extract label
            add_row(result_array, &row);
        }
    }


    sqlite3_finalize(stmt);

    sqlite3_close(db);
    return result_array;  // Return the populated array of Rows
}



sqlite3* open_connection(){
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    int flags = SQLITE_OPEN_READWRITE;
    rc = sqlite3_open_v2(DBNAME, &db, flags, NULL);

    if(rc != SQLITE_OK) {
        print_error("sqlite_open: %s", db);
        sqlite3_close_v2(db);
        return(0);
    }
    fprintf(stderr, "Opened database %s successfully\n", DBNAME);
    return db;
}


void* open_connection_fake(){
    return (void*) open_connection();
}


// Function to initialize the SQLITE3
int sqlite_initialize() {
    return 0;
}

// Function to cleanup Redis resources
void sqlite_cleanup() {
    // In this simple version, no additional resources to clean up
}


void close_connection(void* db){
    sqlite3_close_v2((sqlite3*) db);
}

void insert_rows(RowArray* rows) {
    sqlite3 *db = open_connection();
    char *zErrMsg = 0;
    int rc;
    sqlite3_stmt *stmt;

    char *sql = "INSERT INTO reclustered_energy_records (pod_id, a01, a02, a03, a04, a05, "
                "a06, a07, a08, a09, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, "
                "a20, a21, a22, a23, a24, label) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
                "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    // Begin transaction
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot begin transaction: %s\n", sqlite3_errmsg(db));
        exit(2);
    }

    // Prepare the SQL statement
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        exit(2);
    }

    // Iterate over each row in the RowArray
    for (int i = 0; i < rows->count; i++) {
        Row *row = &rows->rows[i];

        // Bind the pod_id
        sqlite3_bind_int(stmt, 1, row->pod_id);

        // Bind the 24 double values
        for (int j = 0; j < 24; j++) {
            sqlite3_bind_double(stmt, j + 2, row->a[j]);
        }

        // Bind the label
        sqlite3_bind_int(stmt, 26, row->label);

        // Execute the statement
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Execution failed at row %d: %s\n", i, sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            free(row);
            sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
            exit(2);
        }

        // Reset the statement for the next iteration
        sqlite3_reset(stmt);
        //free(row);
    }

    // Finalize the statement
    sqlite3_finalize(stmt);

    // Commit transaction
    rc = sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot commit transaction: %s\n", sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        exit(2);
    }

    printf("All rows inserted successfully.\n");
    close_connection(db);
}


DBManager *create_sqlite_manager() {
    DBManager *manager = (DBManager *)malloc(sizeof(DBManager));
    manager->initialize = sqlite_initialize;
    manager->get_connection = open_connection_fake;
    manager->close_connection = close_connection;
    manager->cleanup = sqlite_cleanup;
    manager->count = sqlite_count;
    manager->select_rows = select_rows;
    manager->insert_rows = insert_rows;

    return manager;
}
