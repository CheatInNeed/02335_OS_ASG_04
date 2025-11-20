

/** 
 * Program performing parallel search in a text using a simple thread pool
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/time.h>

#include "pool.h"

#define MAX_SIZE (10 * 1024 * 1024)  // Max  text size (10 MB)

#define WARMUPS 2                    // Warmup searches to fill cache etc.
#define RUNS    5                    // Number of regular runs to get stable average


/* Program parameters */
static char * text_file_name;
static char * pattern;
static int tasks = 1;
static int threads = 1;
static char * data_file_name = NULL;

/* Search text */
static FILE * file;
static char * text;
static int text_length;
static int pattern_length;

/* Data file */
static FILE * data_file = NULL;

typedef struct {
  int from;  // Start position
  int to;    // End position (up to, not included)
} Interval;

uint64_t micros(void) {
  struct timeval now;
  gettimeofday(&now,NULL);
  return (uint64_t) now.tv_sec * 1000000 + now.tv_usec;
}  

/*
 * Read positional args: file pattern [tasks [threads [data_file]]]
 */
void read_args(int argc, char ** argv) {
  int n; 

  if (argc < 3) {
    printf("Usage: search <text file> <pattern> [<tasks> [<threads> [<data file>] ] ]\n");
    exit(1);
  }
  
  text_file_name = argv[1];
  file = fopen(text_file_name, "r");
  if (file == NULL) {
    printf("ERROR: File %s could not be opened\n", text_file_name);
    exit(1);
  }

  text = malloc(MAX_SIZE + 1);
  if (text == NULL) {
    printf("ERROR: File buffer could not be allocated\n");
    exit(1);
  }
  
  text_length = fread(text, 1, MAX_SIZE, file);
  if (text_length < 0 || ferror(file)) {
    printf("ERROR: While reading text filen");
    exit(1);
  }    
  if (!feof(file)) {
    printf("Warning: File %s, was truncated\n", text_file_name);
  }    

  text[text_length] = '\0';
  
  pattern = argv[2];
  pattern_length = strlen(pattern);
  
  if (argc <= 3) return;
  n = atoi(argv[3]);
  if (n > 1) tasks = n;

  if (argc <= 4) return;
  n = atoi(argv[4]);
  if (n > 1) threads = n;

  if (argc <= 5) return;
  data_file_name = argv[5];
  data_file = fopen(data_file_name, "a");
  if (data_file == NULL) {
    printf("ERROR: Data file %s could not be opened\n", data_file_name);
    exit(1);
  }

}




/**
 * Search task.  Deliberately inefficient.  NOT TO BE CHANGED.
 *
 * Returns the number of times, the search pattern occurs in the text slice:
 *   text[slice->from, slice->to - 1] 
 */
void * search (void * arg) {
  Interval * slice = arg;
  int i, j, found;

  int times = 0;
  
  for (i = slice->from; i <= slice->to - pattern_length; i++) {
    found = 1; 
    for (j = 0; j < pattern_length; j++) {
      if (i+j < 0 || i+j >= text_length) {
	printf("ERROR: Search beyond text length at pos %d\n", i+j);
	exit(1);
      }
      if (text[i+j] != pattern[j]) {
	found = 0;
	// We should really break here, but continue to consume some CPU cycles
      }
    }
    if (found) { times++; }
  }

  return (void *) (long int) times;
}



int main(int argc, char ** argv) {
  int i, k, ret;
  uint64_t start, end;
  int result_single, result_multiple;
  unsigned int total_time_single, total_time_multiple;

  read_args(argc, argv);
  
  printf("Running search with: \n"
	 "  file = %s, file length = %d\n"
	 "  pattern = '%s', pattern length = %d\n"
	 "  tasks = %d, threads = %d\n", text_file_name, text_length,
	 pattern, pattern_length, tasks, threads);
  if (data_file != NULL) {
    printf("  Data file = %s\n", data_file_name);
  }
  printf("\n");

  pool_init(threads);

  /***************** Warmup search using single task  ******************/

  total_time_single = 0;

  for (k = 0; k < WARMUPS; k++) {
  
    printf("Warmup run no. %d using single task.", k);

    start = micros();
  
    Interval * full = malloc(sizeof(Interval));
    full->from = 0;
    full->to = text_length;
    Task * task = task_create(full, search);
    pool_submit(task);

    task_await(task);

    int result = (long int) task->res;
    free(task->arg);
    task_dismiss(task);

    end = micros();

    total_time_single += end - start;

    printf(" Occurences = %d, time = %lu [us]\n", result, end - start);

 }

  printf("Average of warmup runs: %.1f [us]\n\n",  (float) total_time_single/WARMUPS);



  /***************** Search using single task ******************/

  total_time_single = 0;

  for (k = 0; k < RUNS; k++) {
  
    printf("Proper run no. %d using single task.", k);

    start = micros();
  
    Interval * full = malloc(sizeof(Interval));
    full->from = 0;
    full->to = text_length;
    Task * task = task_create(full, search);
    pool_submit(task);

    task_await(task);

    int result = (long int) task->res;
    free(task->arg);
    task_dismiss(task);

    end = micros();

    printf(" Occurences = %d, time = %lu [us]\n", result, end - start);
 
    total_time_single += end - start;

    result_single = result;
  }

  printf("Average of regular single task runs: %.1f [us]\n\n",
	 (float) total_time_single/RUNS);

  
  /***************** Search using multiple task ******************/

  total_time_multiple = 0;

  Task ** taskp = malloc(sizeof(Task *)*tasks);
  
  for (k = 0; k < RUNS; k++) {
  
    printf("Proper run no. %d using %d tasks.", k, tasks);

    start = micros();
  
    for (i = 0; i < tasks; i++) {
      Interval * chunk = malloc(sizeof(Interval));
      chunk->from = 0;                         // TODO: find proper limits
      chunk->to = text_length;                 
      taskp[i] = task_create(chunk, search);
      pool_submit(taskp[i]);
    }

    for (i = 0; i < tasks; i++) {
      task_await(taskp[i]);
    }
  
    int total = 0;
    
    for (i = 0; i < tasks; i++) {
      total += (uintptr_t) taskp[i]->res;    // Add occurrences
      free(taskp[i]->arg);                   // Free interval
      task_dismiss(taskp[i]);
    }
    
    end = micros();

    printf(" Occurences = %d, time = %lu [us]\n", total, end - start);
    total_time_multiple += end - start;
     
    result_multiple = total; 
  }

  printf("Average of multiple task runs: %.1f [us]\n\n",
	 (float) total_time_multiple/RUNS);


  /* Result handling */

  float speedup = (float) total_time_single/ (float) total_time_multiple;
 
  printf("  Speedup = %f\n\n", speedup);
    
  if (result_single != result_multiple) {
    printf("  WARNING: RESULTS DIFFER. Single: %d, multiple: %d\n\n",
	   result_single, result_multiple);
  }

  if (result_single == result_multiple && data_file !=NULL) {
    fprintf(data_file, "%d, %d, %.1f, %.1f, %f\n", tasks, threads, (float) total_time_single/RUNS, 
             (float) total_time_multiple/RUNS, speedup);
    printf("\nSearch data written to %s\n", data_file_name);
  }
  

  return 0;
}

