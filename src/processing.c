#include "processing.h"

void print_row(Row row) {
    printf("pod_id: %d\n", row.pod_id);
    printf("a[] values:\n");
    for (int i = 0; i < 24; i++) {
        printf("a%d: %.4f ", i, row.a[i]);
    }
    printf("\nlabel: %d\n", row.label);
}

void compute_kpp(DBManager *db_manager, int num_points, int num_clusters, int max_iter, double radius){
    Row* centroids;
    RowArray* rows = db_manager->select_rows("select * from energy_records;", 0);

    centroids = lloyd(rows->rows, rows->count, num_clusters, max_iter);

	/*for(int i=0; i<rows->count; i++) {
		if(rows->rows[i].label == 2) {
			printf("%f with label %d\n", rows->rows[i].a[0], rows->rows[i].label);
		}
	}*/

	db_manager->insert_rows(rows);

    free(rows);
    free(centroids);
}


Row* lloyd(Row* pts, int num_pts, int num_clusters, int maxTimes){
	int i, clusterIndex;
	int changes;
	int acceptable = num_pts / 1000;	/* The maximum point changes acceptable. */


	if (num_clusters == 1 || num_pts <= 0 || num_clusters > num_pts )
		return 0;


	Row* centroids = (Row*)malloc(sizeof(Row) * num_clusters);

	if ( maxTimes < 1 )
		maxTimes = 1;

/*	Assign initial clustering randomly using the Random Partition method
	for (i = 0; i < num_pts; i++ ) {
		pts[i].group = i % num_clusters;
	}
*/

	/* or use the k-Means++ method */

/* Original version
	kpp(pts, num_pts, centroids, num_clusters);
*/
	/* Faster version */
	kppFaster(pts, num_pts, centroids, num_clusters);

	do {
		/* Calculate the centroid of each cluster.
		  ----------------------------------------*/

		/* Initialize the x, y and cluster totals. */
		for ( i = 0; i < num_clusters; i++ ) {
			centroids[i].label = 0;		/* used to count the cluster members. */
			for ( int j = 0; j < 23; j++ ) {
				centroids[i].a[j] = 0;
			}
			//centroids[i].x = 0;			/* used for x value totals. */
			//centroids[i].y = 0;			/* used for y value totals. */
		}

		/* Add each observation's x and y to its cluster total. */
		for (i = 0; i < num_pts; i++) {
			clusterIndex = pts[i].label;
			centroids[clusterIndex].label++;
			for ( int j = 0; j < 23; j++ ) {
				centroids[clusterIndex].a[j] += pts[i].a[j];
			}
			//centroids[clusterIndex].x += pts[i].x;
			//centroids[clusterIndex].y += pts[i].y;
		}

		/* Divide each cluster's x and y totals by its number of data points. */
		for ( i = 0; i < num_clusters; i++ ) {
			for ( int j = 0; j < 23; j++ ) {
				centroids[i].a[j] /= centroids[i].label;
			}
			//centroids[i].x /= centroids[i].label;
			//centroids[i].y /= centroids[i].label;
		}

		/* Find each data point's nearest centroid */
		changes = 0;
		for ( i = 0; i < num_pts; i++ ) {
			clusterIndex = nearest(&pts[i], centroids, num_clusters);
			if (clusterIndex != pts[i].label) {
				pts[i].label = clusterIndex;
				changes++;
			}
		}

		maxTimes--;
	} while ((changes > acceptable) && (maxTimes > 0));

	/* Set each centroid's group index */
	for ( i = 0; i < num_clusters; i++ )
		centroids[i].label = i;

	return centroids;
}	/* end, lloyd */

void kppFaster(Row* pts, int num_pts, Row* centroids, int num_clusters)
{
	int j;
	int selectedIndex;
	int cluster;
	double sum;
	double d;
	double random;
	double * cumulativeDistances;
	double * shortestDistance;


	cumulativeDistances = (double*) malloc(sizeof(double) * num_pts);
	shortestDistance = (double*) malloc(sizeof(double) * num_pts);


	/* Pick the first cluster centroids at random. */
	selectedIndex = rand() % num_pts;
	centroids[0] = pts[selectedIndex];

	for (j = 0; j < num_pts; ++j)
		shortestDistance[j] = HUGE_VAL;

	/* Select the centroids for the remaining clusters. */
	for (cluster = 1; cluster < num_clusters; cluster++) {

		/* For each point find its closest distance to any of
		   the previous cluster centers */
		for ( j = 0; j < num_pts; j++ ) {
			d = distn(&pts[j], &centroids[cluster-1] );

			if (d < shortestDistance[j])
				shortestDistance[j] = d;
		}

		/* Create an array of the cumulative distances. */
		sum = 0.0;
		for (j = 0; j < num_pts; j++) {
			sum += shortestDistance[j];
			cumulativeDistances[j] = sum;
		}

		/* Select a point at random. Those with greater distances
		   have a greater probability of being selected. */
		random = (float) rand() / (float) RAND_MAX * sum;
		selectedIndex = bisectionSearch(cumulativeDistances, num_pts, random);

		/* assign the selected point as the center */
		centroids[cluster] = pts[selectedIndex];
	}

	/* Assign each point the index of it's nearest cluster centroid. */
	for (j = 0; j < num_pts; j++)
		pts[j].label = nearest(&pts[j], centroids, num_clusters);

	free(shortestDistance);
	free(cumulativeDistances);
}	/* end, kppFaster */


/*-------------------------------------------------------
	dist2

	This function returns the squared euclidean distance
	between N data points.
-------------------------------------------------------*/
double distn(Row* a, Row* b) {
	double distance_squared = 0.0;
	int dimensions = sizeof(b->a) / sizeof(b->a[0]);
	//iterate over data dimensions
	for (int i = 0; i < dimensions; i++) {
		double diff = a->a[i] - b->a[i];
		distance_squared += diff * diff;
	}

	return distance_squared;
}

int bisectionSearch(double *x, int n, double v)
{
	int il, ir, i;


	if (n < 1) {
		return 0;
	}
	/* If v is less than x(0) or greater than x(n-1)  */
	if (v < x[0]) {
		return 0;
	}
	else if (v > x[n-1]) {
		return n - 1;
	}

	/*bisection search */
	il = 0;
	ir = n - 1;

	i = (il + ir) / 2;
	while ( i != il ) {
		if (x[i] <= v) {
			il = i;
		} else {
			ir = i;
		}
		i = (il + ir) / 2;
	}

	if (x[i] <= v)
		i = ir;
	return i;
}

/*------------------------------------------------------
	nearest

  This function returns the index of the cluster centroid
  nearest to the data point passed to this function.
------------------------------------------------------*/
int nearest(Row* pt, Row* cent, int n_cluster)
{
	int i, clusterIndex;
	double d, min_d;

	min_d = HUGE_VAL;
	clusterIndex = pt->label;
	for (i = 0; i < n_cluster; i++) {
		d = distn(&cent[i], pt);
		if ( d < min_d ) {
			min_d = d;
			clusterIndex = i;
		}
	}
	return clusterIndex;
}


