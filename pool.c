
/** 
 * Implementation of a simple thread pool to which tasks may be submitted for 
 * concurrent and potentially parallel execution.
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/* Implements */
#include "pool.h"

/* Uses */
#include "aq.h"


/* Worker prototype */
void * worker(void *);  

static AlarmQueue task_queue;

static int workers = 0;

/*
 * Protection of pool operations
 */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void pool_init(int threads){
  int i, res;
  
  if (threads <= 0) {
    printf("Warning: Thread pool initialized with non-positive number of worker threads\n");
    return;
  }
 
  pthread_mutex_lock(&mutex);
  if (workers > 0) {
    pthread_mutex_unlock(&mutex);
    printf("Warning: Thread pool already initialized\n");
    return;
  }
    
  workers = threads;

  task_queue = aq_create();

  if (task_queue == NULL) {
    printf("ERROR: Thread pool could not create alarm queue\n");
    exit(1);
  }

  for (i = 0; i < workers; i++) {
    pthread_t * new = malloc(sizeof(pthread_t));
    res = pthread_create(new, NULL, worker, NULL);
    if (res < 0) {
     printf("ERROR: Thread pool could not create worker thread\n");
     exit(1);
    }
  }

  pthread_mutex_unlock(&mutex);
}

void pool_submit(Task * t) {
  if (t == NULL) {
    printf("ERROR: Task submitted to thread pool is NULL\n");
    exit(1);
  }
  
  pthread_mutex_lock(&mutex);
  if (workers == 0) {
    pthread_mutex_unlock(&mutex);
    printf("ERROR: Task submitted to unitialized thread pool\n");
    exit(1);
  }
    
  aq_send(task_queue, t, AQ_NORMAL);
  
  pthread_mutex_unlock(&mutex);
}

  
void * worker (void * arg) {
  Task * task;

  while (1) {
    /* Pull task from task queue */
    int kind = aq_recv(task_queue,  (void **) &task);
    if (kind == AQ_NORMAL) {
      /* Normal messages are assumed to be Tasks to be executed */
      task_execute(task);
    } else {
      /* Alarm messages are assumed not to be used */
    }
  }

  return NULL;
}


void pool_adjust(int threads) {

}

