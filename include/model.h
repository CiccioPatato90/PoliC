#ifndef MODEL_H
#define MODEL_H

#define INITIAL_CAPACITY 1000
#include <stdlib.h>

typedef struct {
    int pod_id;
    double a[24];  // a0 to a23
    int label;     // 0 to 3
} Row;

typedef struct {
    Row *rows;
    int count;
    int capacity;
} RowArray;

// Function to initialize the RowArray with an initial capacity
static RowArray* init_row_array() {
    RowArray *array = (RowArray*)malloc(sizeof(RowArray));
    array->rows = (Row*)malloc(INITIAL_CAPACITY * sizeof(Row));
    array->count = 0;
    array->capacity = INITIAL_CAPACITY;
    return array;
}

// Function to free the RowArray
static void free_row_array(RowArray *array) {
    free(array->rows);
    free(array);
}

// Function to add a Row to RowArray, growing the array as needed
static void add_row(RowArray *array, Row *row) {
    if (array->count >= array->capacity) {
        array->capacity *= 2;
        array->rows = (Row*)realloc(array->rows, array->capacity * sizeof(Row));
    }
    array->rows[array->count++] = *row;
}


#endif // MODEL_H
