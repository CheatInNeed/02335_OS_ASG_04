/**
 * @file   task.c
 * @Author 02335 team
 * @date   November, 2024
 * @brief  Task operation implementations
 */

/* Implements */
#include "task.h"

/* Uses */
#include <stdlib.h>
#include <stdio.h>

/* Stages */
#define CREATED    0
#define EXECUTING  1
#define COMPLETED  2
#define DISMISSED  3

/*
 * All task are maintained under a common lock
 */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* External operation implementations */

/* Creation needs not be locked, as no other threads may yet have 
 * access to this task.
 */
Task * task_create(void * a, void * (*f)(void *) ) {
  Task * t = malloc(sizeof(Task));
  t->arg = a;
  t->res = NULL;
  t->comp = f;
  t->stage = CREATED; 
  pthread_cond_init(&t->done, NULL);
  return t;
}

void task_execute(Task * t) {
  int old_stage;
  pthread_mutex_lock(&mutex);
  old_stage = t->stage;
  if (old_stage == CREATED) {
    t->stage = EXECUTING;
  }
  pthread_mutex_unlock(&mutex);

  if (old_stage != CREATED) return;

  t->res = (*(t->comp))(t->arg);

  pthread_mutex_lock(&mutex);
  t->stage = COMPLETED;
  pthread_cond_broadcast(&t->done);
  pthread_mutex_unlock(&mutex);
}


void task_await(Task * t) {
  pthread_mutex_lock(&mutex);
  while (t->stage < COMPLETED) {
    pthread_cond_wait(&t->done, & mutex);
  }
  pthread_mutex_unlock(&mutex);
}

void task_dismiss(Task * t) {
  pthread_mutex_lock(&mutex);
  if (t->stage != COMPLETED) {
    printf("ERROR: Task not completed before being dismissed");
    exit(1);
  }
  t->stage = DISMISSED;
  pthread_cond_destroy(&t->done);
  free(t);
  pthread_mutex_unlock(&mutex);
}
