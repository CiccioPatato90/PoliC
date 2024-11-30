#ifndef PROCESSING_H
#define PROCESSING_H



// Include necessary headers
#include <stdio.h>
#include <stdlib.h>
#include <cjson/cJSON.h>
#include <math.h>
#include <hiredis/hiredis.h>
#include <model.h>
#include "db_manager.h"


// Function prototypes
int nearest(Row* pt, Row* cent, int n_cluster);
int bisectionSearch(double *x, int n, double v);
double distn(Row* a, Row* b);
Row* lloyd(Row* pts, int num_pts, int num_clusters, int maxTimes);
void kppFaster(Row* pts, int num_pts, Row* centroids,int num_clusters);
void compute_kpp(DBManager *db_manager, int num_points, int num_clusters, int max_iter, double radius);

#endif // PROCESSING_H
