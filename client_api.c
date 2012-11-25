#include "client_api.h"
#include "errors.h"
#include <string.h>
#include <stdlib.h>

/* Helper functions */
void add_to_buffer(rb_struct *rbs, void *data, int sd, int priority);

/* This method trashes request_str (in place) */
cli_req_t parse_request(char *request_str) 
{
  char *token, *context;
  cli_req_t req;
  char req_delims[] = ":";
  char arg_delims[] = ",";
  token = strtok_r(request_str, req_delims, 
				   &context);
  req.client_name = token;
  token = strtok_r(NULL, req_delims,
				 &context);
  req.priority = atoi(token);
  token = strtok_r(NULL, req_delims,
				 &context);
  if (strcmp(token, START_MOVIE_STR) == 0) {
	req.request = start_movie;
	token = strtok_r(NULL, arg_delims,
				   &context);
	req.movie_name = token;
	token = strtok_r(NULL, arg_delims,
				   &context);
	req.repeat = atoi(token);
  } else if (strcmp(token, STOP_MOVIE_STR) == 0) {
	req.request = stop_movie;
	token = strtok_r(NULL, req_delims,
				   &context);
	req.movie_name = token;
  } else if (strcmp(token, SEEK_MOVIE_STR) == 0) {
	req.request = seek_movie;
	token = strtok_r(NULL, arg_delims,
				   &context);
	req.movie_name = token;
	token = strtok_r(NULL, arg_delims,
				   &context);
	req.frame_number = atoi(token);
  } else  { // Unrecognized request 
	DPRINTF(("Got invalid request type: %s\n", token));
  }
  
  return req;
  
}

/* It will be the worker's responsibility to free the original_req_str,
   tokenized_req_str, and the worker_data struct itself */
void movie_worker(void *worker_data)
{
  unsigned int msg_num = 0;
  void *msg;
  char *original_req_str_cpy = NULL;
  size_t msg_len;
  worker_data_t *data = (worker_data_t *) worker_data;
  DPRINTF(("New movie worker created for client %s, movie %s\n",
		   data->req.client_name, data->req.movie_name));
  
  while (1) {
	pthread_mutex_lock(data->mutex);
	if (data->cancelled) {
	  pthread_mutex_unlock(data->mutex);
	  break;
	}
	
	if (data->original_req_str != NULL) {
	  free(original_req_str_cpy);
	  original_req_str_cpy = malloc(strlen(data->original_req_str) + 1);
	  strcpy(original_req_str_cpy, data->original_req_str);
	  free(data->original_req_str);
	  data->original_req_str = NULL;
	  /* Need to allocate large enough buffer for return message */
	  msg_len = strlen("thd 9999:sd 999:msg #9999 :\n") + 200
		+ strlen(original_req_str_cpy) + 1;
	  msg = malloc(msg_len);
	  snprintf((char *) msg,
			   msg_len,
			   "thd %d:sd %d:msg #%d:%s\n",
			   thread_pool_getid(data->tp),
			   data->cli->sd,
			   ++msg_num,
			   original_req_str_cpy);
	  free(data->tokenized_req_str);
	  data->tokenized_req_str = NULL;
	} else {
	  msg = malloc(msg_len);
	  snprintf((char *) msg,
			   msg_len,
			   "thd %d:sd %d:msg #%d:%s\n",
			   thread_pool_getid(data->tp),
			   data->cli->sd,
			   ++msg_num,
			   original_req_str_cpy);
	}
	sleep(2);
	add_to_buffer(data->msg_rb, msg, data->cli->sd, data->req.priority);
	pthread_mutex_unlock(data->mutex);
  }
  DPRINTF(("Stopping movie worker for client %s, movie %s\n",
		   data->req.client_name, data->req.movie_name));
  free(original_req_str_cpy);
  free(data);
}

rb_struct *rb_struct_create()
{
  rb_struct *rbs = malloc(sizeof(rb_struct));
  rbs->mutex = malloc(sizeof(pthread_mutex_t));
  rbs->has_space = malloc(sizeof(pthread_cond_t));
  rbs->has_greater_than_M = malloc(sizeof(pthread_cond_t));
  pthread_mutex_init(rbs->mutex, NULL);
  pthread_cond_init(rbs->has_space, NULL);
  pthread_cond_init(rbs->has_greater_than_M, NULL);
  rbs->buf = ring_buffer_create(MSG_BUF_SIZE);
  return rbs;
}

void rb_struct_destroy(rb_struct *rbs)
{
  pthread_mutex_destroy(rbs->mutex);
  pthread_cond_destroy(rbs->has_space);
  pthread_cond_destroy(rbs->has_greater_than_M);
  ring_buffer_destroy(rbs->buf);
  free(rbs->mutex);
  free(rbs->has_space);
  free(rbs->has_greater_than_M);
  free(rbs);
}

void add_to_buffer(rb_struct *msg_rb, void *data, int sd, int priority)
{
  buf_item *itemp = malloc(sizeof(buf_item));
  itemp->data = data;
  itemp->sd = sd;
  itemp->priority = priority;
  pthread_mutex_lock(msg_rb->mutex);
  while (ring_buffer_full(msg_rb->buf)) {
	pthread_cond_wait(msg_rb->has_space, msg_rb->mutex);
  }
  ring_buffer_add(msg_rb->buf, itemp);
  if (ring_buffer_getsize(msg_rb->buf) > M) {
	pthread_cond_signal(msg_rb->has_greater_than_M);
  }
  pthread_mutex_unlock(msg_rb->mutex);
}
