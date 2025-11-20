/**
 * @file   task.h
 * @Author 02335 team
 * @date   November, 2024
 * @brief  Task structure and operation prototypes
 */

#ifndef TASK_H_INCLUDED
#define TASK_H_INCLUDED

#include <pthread.h>
#include <stddef.h>

typedef struct {
  void * arg;                // Pointer to argument struct
  void * res;                // Pointer to result struct
  void * (*comp)(void *);    // Computation function
  int stage;
  pthread_cond_t done;       // Condition for awaing completion   
} Task;

/**
 * @name    task_create
 * @brief   Creates a task to be executed.    
 * @retval  Handle to the task
 */
Task * task_create(void * a, void * (*f) (void *) );

/**
 * @name    task_execute
 * @brief   Executes the computation function of a task and sets the result,
 *          This completes the task.  A task can only be executed once.
 */
void task_execute(Task * t);

/**
 * @name    task_await
 * @brief   Awaits the completion of a task.  After the call, the task is 
 *          known to be completed and the result is stable.
 */
void task_await(Task * t);

/**
 * @name    task_dismiss
 * @brief   Dismisses the resources used by a task.   Before call, the task
 *          must be completed, and its argument and result resources must have
 *          been freed by the caller.
 *          After the call, the task must not be referenced again.
 */
void task_dismiss(Task * t);

#endif /* TASK_H_INCLUDED */

