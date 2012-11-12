#include "thread_pool.h"
#include <stdbool.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <sched.h>
#include "ring_buffer.h"
#include "errors.h"

#define EPHEMERAL_IDLE_TIME 10 // in sec

void cleanup_ephemeral(void *thread_info)
{
  thread_info_t *self = (thread_info_t*) thread_info;  
  free(self);
}

void *thread_func(void *thread_info) 
{
  thread_info_t *self = (thread_info_t*) thread_info;
  pthread_mutex_t *task_mutex_ptr = &self->tp_data->available_task_mutex;
  pthread_cond_t *task_cond_ptr = &self->tp_data->task_available;
  struct timespec cond_time;
  int status;
  bool holding_mutex = false;
  task_t *task;
  void (*task_func) (void *);
  void *arg;

  pthread_detach(pthread_self());

  while(1) {
	if (!holding_mutex) {
	  status = pthread_mutex_lock(task_mutex_ptr);
	  if (status != 0)
		err_abort(status, "Lock mutex");
	}
	
	self->tp_data->num_waiting++;
	status = 0;
	if (self->ephemeral) {
	  cond_time.tv_sec = time(NULL) + EPHEMERAL_IDLE_TIME;
	  cond_time.tv_nsec = 0;
	  DPRINTF(("About to timed wait\n"));
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
	  DPRINTF(("waiting on condition"));
	  status = pthread_cond_wait(task_cond_ptr, task_mutex_ptr);
	}
	if (status != 0)
	  err_abort(status, "Waiting on work");
	holding_mutex = true;
	self->tp_data->num_waiting--;	

	// TODO: replace with a ring buffer for tasks
	task = ring_buffer_remove(self->tp_data->task_buffer);
	if (task == NULL) {
	  DPRINTF(("Found null task\n"));
	  continue;
	}

	task_func = task->func;
	arg = task->arg;
	free(task);

	status = pthread_mutex_unlock(task_mutex_ptr);
	if (status != 0)
	  err_abort(status, "Unlock mutex before executing task");
	task_func(arg);
	
	task_func = arg = NULL;
	holding_mutex = false;
  }

  DPRINTF(("Exiting\n"));
  return NULL;
}

void thread_pool_execute(thread_pool_t *tp, void (*func)(void *), void *arg)
{
  DPRINTF(("Scheduling task\n"));
  pthread_mutex_lock(&tp->shared.available_task_mutex);
  task_t *task = calloc(1, sizeof(task_t));
  task->func = func;
  task->arg = arg;
  tp->shared.task = task;
  pthread_cond_signal(&tp->shared.task_available);
  DPRINTF(("Unlocking available task mutex: %d threads reportedly waiting\n", tp->shared.num_waiting));
  pthread_mutex_unlock(&tp->shared.available_task_mutex);
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
  tp->task_buffer = ring_buffer_create(size);
  tp->num_permathreads = size;
  tp->shared.num_waiting = 0;

  pthread_mutexattr_init(&mutex_attr);
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_init(&tp->shared.available_task_mutex, &mutex_attr);
  pthread_mutexattr_destroy(&mutex_attr);

  pthread_cond_init(&tp->shared.task_available, NULL);

  for (i = 0; i < size; i++) {
  	thread_info = tp->permathreads + i;
  	thread_info->ephemeral = true;
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
  thread_pool_t *tp = thread_pool_create(10);
  thread_pool_execute(tp, print_str, 1);
  thread_pool_execute(tp, print_str, 2);
  thread_pool_execute(tp, print_str, 3);
  thread_pool_execute(tp, print_str, 4);
  pthread_exit(0);
  return 0;
}


