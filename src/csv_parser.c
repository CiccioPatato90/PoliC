#include "csv_parser.h"

#define MAXCHAR 1000

#include <csv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>
#include "cjson/cJSON.h"

// Structure to hold parsing state
typedef struct {
    Row current_row;
    int field_index;
    int row_count;
    int lbl_0;
    int lbl_1;
    int lbl_2;
    int lbl_3;
    redisContext *ctxt;
} CSVParserState;

// Global variable to interrupt parsing after N rows (if needed)
int csv_interrupted = 0;

// Callback function for each field
void field_callback(void *field, size_t len, void *data) {
    CSVParserState *state = (CSVParserState *)data;

    // Allocate memory for the field string
    char *field_str = malloc(len + 1);
    if (field_str == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    memcpy(field_str, field, len);
    field_str[len] = '\0';

    //printf("FIELD_STR: %s\n", field_str);

    // Assign the field to the appropriate member of Row
    if (state->field_index == 0) {
        // pod_id
        state->current_row.pod_id = atoi(field_str);
    } else if (state->field_index >= 1 && state->field_index <= 24) {
        // a0 to a23
        int a_index = state->field_index - 1;
        state->current_row.a[a_index] = atof(field_str);
    } else if (state->field_index == 25) {
        // label
        state->current_row.label = atoi(field_str);
    } else {
        // Handle extra fields if any
        fprintf(stderr, "LABEL NOT RECOGNIZED");
    }

    free(field_str);  // Free the allocated string
    state->field_index++;
}

void row_callback(int c, void *data) {
    CSVParserState *state = (CSVParserState *)data;

    // Ensure that all required fields are present
    if (state->field_index >= 26) {
        cJSON *json = cJSON_CreateObject();
        
        if (!json) {
            fprintf(stderr, "Failed to create cJSON object\n");
            return;
        }


        cJSON_AddNumberToObject(json, "pod_id", state->current_row.pod_id);
        cJSON *a_array = cJSON_CreateDoubleArray(state->current_row.a, 24);
        if (!a_array) {
            fprintf(stderr, "Failed to create cJSON array\n");
            cJSON_Delete(json);
            return;
        }
        cJSON_AddItemToObject(json, "values", a_array);
        cJSON_AddNumberToObject(json, "label", state->current_row.label);

        // Serialize JSON to string
        char *json_str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
        if (!json_str) {
            fprintf(stderr, "Failed to print JSON\n");
            return;
        }


        // Generate unique record ID
        redisReply *reply = redisCommand(state->ctxt, "INCR label:%d:counter", state->current_row.label);
        if (!reply || reply->type != REDIS_REPLY_INTEGER) {
            fprintf(stderr, "Failed to increment counter for label:%d\n", state->current_row.label);
            if (reply) freeReplyObject(reply);
            free(json_str);
            return;
        }
        long long record_id = reply->integer;
        freeReplyObject(reply);


        reply = redisCommand(state->ctxt, "HSET label:%d %lld %s", state->current_row.label, record_id, json_str);
        if (!reply || (reply->type != REDIS_REPLY_INTEGER && reply->type != REDIS_REPLY_STATUS)) {
            fprintf(stderr, "Failed to HSET record for label:%d\n", state->current_row.label);
            if (reply) freeReplyObject(reply);
            free(json_str);
            return;
        }
        freeReplyObject(reply);

        // Optionally, output for debugging
        //printf("Stored record under label:%d with record_id:%lld\n", state->current_row.label, record_id);
        free(json_str);

    } else {
        fprintf(stderr, "Incomplete data for a row. Fields parsed: %d\n", state->field_index);
    }

    // Reset field index and current_row for the next row
    state->field_index = 0;
    memset(&(state->current_row), 0, sizeof(Row));

    // Increment the row count
    state->row_count++;

    // Stop after N lines if desired (e.g., 10)
    /*size_t MAX_ROWS = 10;
    if (state->row_count >= MAX_ROWS) {
        csv_interrupted = 1;
    }*/
}



// Parsing function
void parse_csv_file(const char *filename, redisContext *ctxt) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return;
    }

    struct csv_parser parser;
    if (csv_init(&parser, 0) != 0) {
        fprintf(stderr, "Failed to initialize csv parser\n");
        fclose(fp);
        return;
    }

    // Set the delimiter to ';' if needed
    //csv_set_delim(&parser, ';');  // Change ';' to your delimiter if different

    CSVParserState state = {
        .current_row = {0},
        .field_index = 0,
        .ctxt = ctxt,
        .row_count = 0,
        .lbl_0=0,
        .lbl_1=0,
        .lbl_2=0,
        .lbl_3=0,
    };

    char buf[1024];
    size_t bytes_read;

    csv_interrupted = 0;  // Reset interruption flag

    while ((bytes_read = fread(buf, 1, sizeof(buf), fp)) > 0 && !csv_interrupted) {
        if (csv_parse(&parser, buf, bytes_read, field_callback, row_callback, &state) != bytes_read) {
            fprintf(stderr, "Error while parsing file: %s\n", csv_strerror(csv_error(&parser)));
            break;
        }
    }

    // Finish parsing
    csv_fini(&parser, field_callback, row_callback, &state);

    // Cleanup
    csv_free(&parser);
    fclose(fp);
}