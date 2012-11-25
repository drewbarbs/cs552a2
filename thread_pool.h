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
  pthread_key_t id_key;
  int num_waiting;
  int num_outstanding;
  int num_permathreads;
  int num_tempthreads;
  ringbuff_t *task_buffer;
} tp_data_t;

typedef struct thread_info_t {
  pthread_t pt_tid; // PThreads thread identifier
  int tid; // An int thread identifier used to label messages
  bool ephemeral; // Indicates this worker_thread is temporary
  tp_data_t *tp_data;  
} thread_info_t;

typedef struct thread_pool_t {
  thread_info_t *permathreads;
    // may want to add functionality to keep track of 
  // ephemeral threads to be able to immediately kill them off
  tp_data_t shared;
} thread_pool_t;


thread_pool_t *thread_pool_create(int num_threads);
void thread_pool_destroy(thread_pool_t *tp);
void thread_pool_execute(thread_pool_t *tp, void (*task)(void *), void *arg);
int thread_pool_getid(thread_pool_t *tp);

#endif
