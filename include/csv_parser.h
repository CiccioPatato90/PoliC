#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include <hiredis/hiredis.h>
#include <model.h>

void parse_csv_file(const char *filename, redisContext *redis);


#endif 