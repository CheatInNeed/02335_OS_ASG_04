

/** 
 * Program that demonstrates the workings of the simple thread pool
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "pool.h"

static int tasks = 1;
static int threads = 1;

/*
 * Read positional args: [tasks [threads]]
 */
void read_args(int argc, char ** argv) {
  int n; 

  if (argc <= 1) return;
  n = atoi(argv[1]);
  if (n > 1) tasks = n;

  if (argc <= 2) return;
  n = atoi(argv[2]);
  if (n > 1) threads = n;
}


void * compute (void * arg) {
  int no = (long int)  arg + 1;    // We start numbering at 1
  int i, k;

  char * indent = malloc(no*2 + 1);
  memset(indent,' ', no*2);
  indent[no*2] = '\0';
  
  printf("%sTask %d starting\n", indent, no);

  for (k = 0; k < 5; k++) {
    sleep(1);  
    printf("%s|\n", indent);
  }
  
  printf("%sTask %d ending\n", indent, no);
  
  return NULL;
}

int main(int argc, char ** argv) {
  int i, ret;

  read_args(argc, argv);
  printf("------ Running demo with %d tasks and %d threads ----\n\n", tasks, threads);

  pool_init(threads);

  Task ** taskp = malloc(sizeof(Task *)*tasks);
  
  for (i = 0; i < tasks; i++) {
    taskp[i] = task_create((void *)(long int) i, compute);
    pool_submit(taskp[i]);
 }

  for (i = 0; i < tasks; i++) {
    task_await(taskp[i]);
  }
  printf("\n---------- All tasks completed ----------\n");
  
  for (i = 0; i < tasks; i++) {
    task_dismiss(taskp[i]);
  }

  return 0;
}

