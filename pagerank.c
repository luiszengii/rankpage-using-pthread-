#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE

/* To swap version, uncomment the correspoding define macro */
// #define SEQUENTIAL
#define PARALLEL

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "pagerank.h"

/* func that checks if the algor is converged */
int check_converge(double *old_score_list, double *new_score_list, int npages)
{
    double result = 0;
    for (int i = 0; i < npages; i++)
    {
        double diff = old_score_list[i] - new_score_list[i];
        result += diff * diff;
    }
    result = sqrt(result);

    //check if converged by compare with EPSILON
    if (result < EPSILON)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/* -------------------------------------------------------------------- */
/* THE PARALLEL VERSION CODE */
#ifdef PARALLEL

/* the data struct that would be passed in the calculate_score_parallel() */
typedef struct
{
    int tid;
    int start, end;
} thread_range;

/* global vars */
int npages, ncores, nthreads;
double dampener;
page** page_list;
double *old_score_list;
double *new_score_list;
pthread_t *threads;//thread pool
thread_range *thread_ranges;//stores the range of pages each thread handles
pthread_mutex_t lock_score = PTHREAD_MUTEX_INITIALIZER;

void calculate_score(page *page, int page_index, double *old_score_list, double *new_score_list, int npages, double dampener);
void *calculate_score_parallel(void *arg);
int check_converge(double *old_score_list, double *new_score_list, int npages);

void pagerank(list *plist, int num_cores, int num_pages, int num_edges, double dampener_arg)
{
    /* start timing by clock_gettime()*/
    struct timespec start, finish;
    double elapsed;
    clock_gettime(CLOCK_REALTIME, &start);

    // the boolean value
    int converged = 0;

    /* initialize global variables */
    npages = num_pages;
    ncores = num_cores;
    dampener = dampener_arg;
    old_score_list = (double *)malloc(npages * sizeof(double*));
    new_score_list = (double *)malloc(npages * sizeof(double*));
    //list of all pages in a array
    node *next, *curr;
    int index;
    curr = plist->head;
    page_list = (page**)malloc(npages * sizeof(page*));
    while (curr != NULL)
    {
        next = curr->next;
        index = curr->page->index;
        page_list[index] = curr->page;
        curr = next;
    }

    /* init scores before first iter */
    for (int i = 0; i < npages; i++)
    {
        old_score_list[i] = 1.0 / (double)npages;
    }


    /* start hyper-threading: 2 thread per core */
    nthreads = ncores * 2;
    //deciding the number of threads based on the page number
    if (nthreads < npages)
    {
        /* split pages for each thread (thread_range) */
        double range_size = (double)npages / nthreads;
        threads = (pthread_t *)malloc(nthreads * sizeof(pthread_t));
        thread_ranges = (thread_range *)malloc(nthreads * sizeof(thread_range));

        //init the first thread with id=0, range[0,range_size]
        thread_ranges[0].tid = 0;
        thread_ranges[0].start = 0;
        thread_ranges[0].end = floor(range_size);

        //iter through all thread and assign each with the correct range
        for (int i = 1; i < nthreads; i++)
        {
            thread_ranges[i].tid = i;
            thread_ranges[i].start = thread_ranges[i - 1].end;
            if (i < (nthreads - 1))
            {
                thread_ranges[i].end = thread_ranges[i].start + floor(range_size);
            }
            else
            {
                thread_ranges[i].end = npages;
            }
        }
    }
    else
    {
        nthreads = npages;
        //if no need to split, assign each thread_range one page
        threads = (pthread_t *)malloc(nthreads * sizeof(pthread_t));
        thread_ranges = (thread_range *)malloc(nthreads * sizeof(thread_range));
        for (int i = 0; i < nthreads; i++)
        {
            thread_ranges[i].tid = i;
            thread_ranges[i].start = i;
            thread_ranges[i].end = i + 1;
        }
    }

    /* begin iterations */
    //since the iteration has to be sequential, so we
    while (!converged)
    {
        /* then for all thread_ranges, update the scores in new_score_list*/
        // create threads and calculate the range assigned to each thread
        for (int i = 0; i < nthreads; i++)
        {
            pthread_create(&threads[i], NULL, &calculate_score_parallel, (void *)&thread_ranges[i]);
        }

        // join to wait all the scores are updated
        for (int i = 0; i < nthreads; i++)
        {
            pthread_join(threads[i], NULL);
        }

        /* check if converged */
        converged = check_converge(old_score_list, new_score_list, npages);

        /* giving the old score list new values */
        memcpy(old_score_list, new_score_list, sizeof(*new_score_list)*npages);
    }

    /* print output after converged*/
    curr = plist->head;
    while (curr != NULL)
    {
        next = curr->next;
        index = curr->page->index;
        printf("%s %.4lf\n", curr->page->name, new_score_list[index]);
        curr = next;
    }

    /* end timing and print out the time elapsed */
    clock_gettime(CLOCK_REALTIME, &finish);
    elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    elapsed *= 1000.0;
    // printf("time elapsed: %lfms with printing", elapsed);

    //write the result in data.txt
    FILE *fp = fopen("./data.txt", "a");
    fprintf(fp, "%f ms (Parallel on %d threads %d cores)\n", elapsed, nthreads, ncores);
    fclose(fp);

    /* free the memory allocated for threads and thread_ranges */
    free(threads);
    free(thread_ranges);
}

/* function calculating the new score for all pages in a thread_range in each iter(for parallel) */
// same as sequential but iterate through all the pages in thread_range
void *calculate_score_parallel(void *arg)
{
    thread_range *thread_range_ptr = (thread_range *)arg;
    for (int i = thread_range_ptr->start; i < thread_range_ptr->end; i++)
    {
        /* calculate the new score for all pages inside the range */
        page *page = page_list[i];
        double score = (1.0 - dampener) / (double)npages;

        list *inlinks = page->inlinks;
        if (inlinks != NULL) /* check if inlinks is empty */
        {
            node *next;
            node *curr;
            //loop through all pages in inlinks
            curr = inlinks->head;
            int index;
            while (curr != NULL)
            {
                next = curr->next;
                index = curr->page->index;
                score += dampener * old_score_list[index] / (double)curr->page->noutlinks;
                curr = next;
            }
        }

        //update page score with certain index
        pthread_mutex_lock(&lock_score);
        new_score_list[i] = score;
        pthread_mutex_unlock(&lock_score);
    }
    usleep(5000);
    return 0;
}

#endif
/* END OF PARALLEL VERSION CODE */

//-----------------------------------------------------------------

/* SEQUENTIAL VERSION CODE */
#ifdef SEQUENTIAL

/* function calculating the new score in new iter and returns the new score */
double calculate_score(page *page, double* old_score_list, int npages, double dampener)
{
    double score = (1.0 - dampener) / (double)npages;
    list *inlinks = page->inlinks;

    if (inlinks != NULL) /* check if inlinks is empty */
    {
        node *next;
        node *curr;
        //loop through all pages in inlinks
        curr = inlinks->head;
        int index;
        while (curr != NULL)
        {
            next = curr->next;
            index = curr->page->index;
            score += dampener * old_score_list[index]/(double)curr->page->noutlinks;
            curr = next;
        }
    }
    usleep(500);//addting time to see if multi is good(needed #define _DEFAULT_SOURCE)
    return score;
}

void pagerank(list *plist, int ncores, int npages, int nedges, double dampener)
{
    /* start timing by clock_gettime()*/
    struct timespec start, finish;
    double elapsed;
    clock_gettime(CLOCK_REALTIME, &start);

    //EPSILON = 0.005
    int converged = 0;

    /* initialize list of rank score */
    double old_score_list[npages];
    double new_score_list[npages]; //for comparison later

    /* init scores */
    for (int i = 0; i < npages; i++)
    {
        old_score_list[i] = 1.0 / (double)npages;
    }

    /* begin iterations */
    while (!converged)
    {
        /* calculate new scores of each page */
        //loop through all pages
        node *next, *curr;
        double new_score;
        int index;
        curr = plist->head;
        while (curr != NULL)
        {
            next = curr->next;
            //calculating new score, and add them to new_score_list
            index = curr->page->index;
            new_score = calculate_score(curr->page, old_score_list, npages, dampener);
            new_score_list[index] = new_score;
            curr = next;
        }
        /* check if converged */
        converged = check_converge(old_score_list, new_score_list, npages);
        /* giving the old score list new values */
        memcpy(old_score_list, new_score_list, sizeof(new_score_list));
    }

    /* print output */
    node* next;
    node* curr = plist->head;
    int index;
    while (curr != NULL)
    {
        next = curr->next;
        index = curr->page->index;
        printf("%s %.4lf\n", curr->page->name, new_score_list[index]);
        curr = next;
    }

    /* end timing and print out the time elapsed */
    clock_gettime(CLOCK_REALTIME, &finish);
    elapsed = (finish.tv_sec - start.tv_sec);
    printf("elapsed in s: %f\n", elapsed);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("elapsed in nsec: %f\n", elapsed);
    elapsed *= 1000.0;
    // printf("time elapsed in sequential program: %lfms with printing", elapsed);

    //write the result in data.txt
    FILE *fp = fopen("./data.txt", "a");
    fprintf(fp, "%f ms (sequential)\n", elapsed);
    fclose(fp);
}

#endif
/* END OF SEQUENTIAL VERSION CODE */




/*
######################################
### DO NOT MODIFY BELOW THIS POINT ###
######################################
*/

int main(void)
{

    /*
    ######################################################
    ### DO NOT MODIFY THE MAIN FUNCTION OR HEADER FILE ###
    ######################################################
    */

    list *plist = NULL;

    double dampener;
    int ncores, npages, nedges;

    /* read the input then populate settings and the list of pages */
    read_input(&plist, &ncores, &npages, &nedges, &dampener);

    /* run pagerank and output the results */
    pagerank(plist, ncores, npages, nedges, dampener);

    /* clean up the memory used by the list of pages */
    page_list_destroy(plist);

    return 0;
}
