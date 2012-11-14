#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>
#include "errors.h"
#include "ring_buffer.h"
#include "thread_pool.h"

#define EPHEMERAL_IDLE_TIME 10 // in sec
#define MAX_NUM_EPHEMERAL 100

/*
  Cleanup routine called by an ephemeral thread
  before exiting. Deallocates memory reserved
  for its thread_info_t structure
 */
void cleanup_ephemeral(void *thread_info)
{
  thread_info_t *self = (thread_info_t*) thread_info;
  self->tp_data->num_tempthreads--;
  free(self);
}

/*
  Worker thread logic: repeatedly waits for new
  tasks and executes them.
 */
void *thread_func(void *thread_info) 
{
  thread_info_t *self = (thread_info_t*) thread_info;
  /* Declare variables used to synchronize access to tasks */
  pthread_mutex_t *task_mutex_ptr = &self->tp_data->available_task_mutex;
  pthread_cond_t *task_cond_ptr = &self->tp_data->task_available;
  struct timespec cond_time; /* Used by ephemeral threads to do timed waits on
								tasks */
  bool holding_mutex = false; 


  int status; // Used to check return values of PThread funcs for errors

  /* Declare variables used to represent tasks */
  task_t *task;
  void (*task_func) (void *);
  void *arg;

  pthread_detach(pthread_self());
  pthread_cleanup_push(cleanup_ephemeral, thread_info);
  
  while(1) {
	if (!holding_mutex) {
	  status = pthread_mutex_lock(task_mutex_ptr);
	  if (status != 0)
		err_abort(status, "Lock mutex");
	}
	self->tp_data->num_waiting++;
	if (self->ephemeral) {
	  cond_time.tv_sec = time(NULL) + EPHEMERAL_IDLE_TIME;
	  cond_time.tv_nsec = 0;
	  DPRINTF(("Ephemeral thread about to timed wait on work.\n"));
	  status = pthread_cond_timedwait(
						  task_cond_ptr, task_mutex_ptr, &cond_time);
	  if (status == ETIMEDOUT) {
		DPRINTF (("Condition wait for work timed out.\n"));
		self->tp_data->num_waiting--;
		pthread_mutex_unlock(task_mutex_ptr);
		pthread_cond_signal(task_cond_ptr);
		break;
	  }
	} else {
	  DPRINTF(("Permanent thread about to wait on work.\n"));
	  status = pthread_cond_wait(task_cond_ptr, task_mutex_ptr);
	}
	if (status != 0)
	  err_abort(status, "Waiting on work");
	holding_mutex = true;
	self->tp_data->num_waiting--;	

	if (self->tp_data->num_outstanding == 0) {
	  DPRINTF(("Thread complaining about having been woken up for no reason.\n"));
	  continue;
	}
	task = ring_buffer_remove(self->tp_data->task_buffer);
	assert(task != NULL);
	
	task_func = task->func;
	arg = task->arg;
	free(task);
	self->tp_data->num_outstanding--;

	status = pthread_mutex_unlock(task_mutex_ptr);
	holding_mutex = false;

	if (status != 0)
	  err_abort(status, "Unlock mutex before executing task");
	task_func(arg);
	task_func = arg = NULL;

  }

  DPRINTF(("Worker thread exiting\n"));
  if (self->ephemeral) {
	pthread_cleanup_pop(1);
  }
  return NULL;
}

void thread_pool_execute(thread_pool_t *tp, void (*func)(void *), void *arg)
{
  thread_info_t *thread_info;
  int status, num_jobs_in_system;
  task_t *task = calloc(1, sizeof(task_t));
  DPRINTF(("About to place new task on buffer\n"));
  task->func = func;
  task->arg = arg;
  
  /* 
	 First need to add new task to the task buffer.
	 In the case that the buffer is full, I unlock the
	 mutex to allow worker threads to make progress,
	 and try again later 
  */
  do {
	pthread_mutex_lock(&tp->shared.available_task_mutex);
	status = ring_buffer_add(tp->shared.task_buffer, task);
	if (status != 0) {
	  DPRINTF(("Waiting for free space on buffer"));
	  pthread_mutex_unlock(&tp->shared.available_task_mutex);
	}
  } while (status != 0);
  tp->shared.num_outstanding++;
  num_jobs_in_system = tp->shared.num_outstanding + tp->shared.num_permathreads
	+ tp->shared.num_tempthreads - tp->shared.num_waiting;

  /* I need to check if the number of uncompleted jobs in thread pool 
	(including newly submitted task) is greater than the # of permanent threads +
	# of ephemeral threads. If so, I will need to create an additional
	ephemeral thread to handle this task.
  */
  if (num_jobs_in_system > (tp->shared.num_permathreads + tp->shared.num_tempthreads)) {
	DPRINTF(("Creating new temp thread, as there are currently %d tasks "
			 "in system and %d total threads\n", 
			 num_jobs_in_system,
			 (tp->shared.num_permathreads + tp->shared.num_tempthreads)));
	assert(tp->shared.num_waiting <= tp->shared.num_outstanding);
	thread_info = calloc(1, sizeof(thread_info_t));
	thread_info->tp_data = &tp->shared;
	thread_info->ephemeral = true;
  	pthread_create(&thread_info->tid,
  				   NULL, thread_func, thread_info);
	tp->shared.num_tempthreads++;
  }
  
  /*
	Finally, before announcing the availability of a new task,
	I need to ensure a thread will be waiting on the signal 
	(to avoid the "lost signal" problem)
   */
  while (tp->shared.num_waiting < tp->shared.num_outstanding) { // <= ?
	  pthread_mutex_unlock(&tp->shared.available_task_mutex);
	  sched_yield();
	  pthread_mutex_lock(&tp->shared.available_task_mutex);
   }
  pthread_cond_signal(&tp->shared.task_available);
  pthread_mutex_unlock(&tp->shared.available_task_mutex);
  DPRINTF(("Unlocked available task mutex: %d threads reportedly waiting\n", tp->shared.num_waiting));
}


thread_pool_t *thread_pool_create(int size) 
{
  thread_pool_t *tp;
  thread_info_t *thread_info;
  pthread_mutexattr_t mutex_attr;
  int i = 0;
   //TODO: add checks that calloc succeeded
  tp = calloc(1, sizeof(thread_pool_t));
  tp->permathreads = calloc(size, sizeof(thread_info_t));
  //TODO add check that ring buffer create succeeded
  tp->shared.task_buffer = ring_buffer_create(size + MAX_NUM_EPHEMERAL);
  tp->shared.num_permathreads = size;
  tp->shared.num_tempthreads = tp->shared.num_waiting = tp->shared.num_outstanding = 0;

  pthread_mutexattr_init(&mutex_attr);
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_init(&tp->shared.available_task_mutex, &mutex_attr);
  pthread_mutexattr_destroy(&mutex_attr);

  pthread_cond_init(&tp->shared.task_available, NULL);

  for (i = 0; i < size; i++) {
  	thread_info = tp->permathreads + i;
  	thread_info->ephemeral = false;
	thread_info->tp_data = &tp->shared;
  	pthread_create(&thread_info->tid,
  				   NULL, thread_func, thread_info);
  }
  while (tp->shared.num_waiting < size) {
	DPRINTF(("Yielding to child threads to allow them to get to waiting stage\n"));
	sched_yield();
  }
  DPRINTF(("Created thread pool of size %d\n", size));
  return tp;
}

void print_str(void *arg)
{
  int num = (int) arg;
  printf("Thread here, printing! %d\n", num);
}

int main(int argc, char *argv[]) {
  thread_pool_t *tp = thread_pool_create(8);
  thread_pool_execute(tp, print_str, 1);
  thread_pool_execute(tp, print_str, 2);
  thread_pool_execute(tp, print_str, 3);
  thread_pool_execute(tp, print_str, 4);
  thread_pool_execute(tp, print_str, 5);
  thread_pool_execute(tp, print_str, 6);
  thread_pool_execute(tp, print_str, 7);
  thread_pool_execute(tp, print_str, 8);
  thread_pool_execute(tp, print_str, 9);
  thread_pool_execute(tp, print_str, 10);
  thread_pool_execute(tp, print_str, 11);
  thread_pool_execute(tp, print_str, 12);
  thread_pool_execute(tp, print_str, 13);
  thread_pool_execute(tp, print_str, 14);
  thread_pool_execute(tp, print_str, 15);
  sleep(10);
  return 0;
}


