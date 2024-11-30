#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hiredis/hiredis.h>
#include "db_manager.h"

// Redis-specific functions
int redis_initialize();
void *redis_get_connection();
void redis_close_connection(void *connection);
void redis_cleanup();
int redis_num_elems();

// Function to initialize the Redis server
int redis_initialize() {
    if (system("pgrep redis-server > /dev/null || redis-server &") == -1) {
        perror("Error starting Redis server");
        return -1;
    }
    sleep(1); // Wait for Redis server to start
    return 0;
}

// Function to get Redis connection
void *redis_get_connection() {
    redisContext *context = redisConnect("127.0.0.1", 6379);
    if (context == NULL || context->err) {
        fprintf(stderr, "Redis connection error: %s\n", context->errstr);
        return NULL;
    }
    return context;
}

// Function to close Redis connection
void redis_close_connection(void *connection) {
    if (connection) {
        redisFree((redisContext *)connection);
    }
}

// Function to cleanup Redis resources
void redis_cleanup() {
    // In this simple version, no additional resources to clean up
}

void checkReply(redisReply * reply){
    if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
            fprintf(stderr, "Failed to execute Redis command\n");
            exit(1);
        }
}

int redis_num_elems(){
    int result = 0;  // Assume success
    redisContext *context = redis_get_connection();

    // Loop over labels 0 to 3
    for (int label = 0; label <= 3; label++) {
        // Execute HLEN command to get the number of records under the label
        redisReply *reply = redisCommand(context, "HLEN label:%d", label);
        checkReply(reply);

        long long count = reply->integer;
        result+=count;
        freeReplyObject(reply);
    }


    redis_close_connection(context);
}

int check_redis_populated(){
    int count = redis_num_elems();
    if (count <= 0) {
        // Count is zero or negative, indicating no records under this label
        printf("Empty REDIS for labels.\n");
    } else {
        // Count is greater than zero, continue to the next label
        printf("Redis already loaded\n");
    }
    return count;  // Return 0 if all labels have count > 0, else return 1
}

// Factory method to create a DBManager for Redis
DBManager *create_redis_manager() {
    DBManager *manager = (DBManager *)malloc(sizeof(DBManager));
    manager->initialize = redis_initialize;
    manager->get_connection = redis_get_connection;
    manager->close_connection = redis_close_connection;
    manager->cleanup = redis_cleanup;
    manager->count = redis_num_elems;
    return manager;
}
