#ifndef __THREAD_POOL_H
#define __THREAD_POOL_H
#include "ring_buffer.h"
#include <stdbool.h>
#include <pthread.h>

typedef struct task_t {
  void  (*func) (void *); // may want to change to linked list
  void *arg;
} task_t;

typedef struct tp_data_t {
  // Global info
  pthread_cond_t task_available;
  pthread_mutex_t available_task_mutex;
  int num_waiting;
  int num_outstanding;
  int num_permathreads;
  int num_tempthreads;
  ringbuff_t *task_buffer;
} tp_data_t;

typedef struct thread_info_t {
  pthread_t tid;
  bool ephemeral; // Indicates this worker_thread is temporary
  tp_data_t * tp_data;  
} thread_info_t;

typedef struct thread_pool_t {
  thread_info_t *permathreads;
    // may want to add functionality to immediately keep track of 
  // ephemeral threads to be able to immediately kill them off
  tp_data_t shared;
} thread_pool_t;


thread_pool_t *thread_pool_create(int);
void thread_pool_destroy(thread_pool_t*);
void thread_pool_execute(thread_pool_t *tp, void (*task)(void *), void *arg);

#endif
