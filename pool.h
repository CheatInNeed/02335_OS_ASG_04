/**
 * @file   pool.h
 * @Author 02335 team
 * @date   November, 2024
 * @brief  Thread pool interface
 */

#ifndef POOL_H_INCLUDED
#define POOL_H_INCLUDED

#include "task.h"


/**
 * @name    pool_init
 * @brief   Initializes the thread pool with a number of worker threads
 */
void pool_init(int threads);

/**
 * @name    pool_submit
 * @brief   Submits a created task for execution on the pool.
 *          The pool must have been initialized before any submissions are made.
 */
void pool_submit(Task * t);

/**
 * @name    pool_adjust
 * @brief   Changes the number of worker threads dynamically
 *          The pool must have been initialized
 *          Only to be implemented in Task 3.
 */
void pool_adjust(int threads);


#endif /* POOL_H_INCLUDED */

