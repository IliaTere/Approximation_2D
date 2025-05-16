#include "all_includes.h"

static double* results = nullptr;
static pthread_mutex_t results_mutex = PTHREAD_MUTEX_INITIALIZER;

int init_reduce_sum(int p) {
    pthread_mutex_lock(&results_mutex);
    if (results == nullptr) {
        results = new double[p];
        if (results == nullptr) {
            pthread_mutex_unlock(&results_mutex);
            return -1;
        }
    }
    pthread_mutex_unlock(&results_mutex);
    return 0;
}

double reduce_sum_det(int p, int k, double s) {
    pthread_mutex_lock(&results_mutex);
    double sum = 0; 
    int l;
    results[k] = s;
    pthread_mutex_unlock(&results_mutex);
    
    reduce_sum<int>(p);
    
    pthread_mutex_lock(&results_mutex);
    for (l = 0; l < p; ++l) {
        sum += results[l];
    }
    pthread_mutex_unlock(&results_mutex);

    reduce_sum<int>(p);
    return sum;
}

void free_results() {
    pthread_mutex_lock(&results_mutex);
    delete[] results;
    results = nullptr;
    pthread_mutex_unlock(&results_mutex);
}
