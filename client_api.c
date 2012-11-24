#include "client_api.h"
#include "errors.h"
#include <string.h>
#include <stdlib.h>

extern ringbuff_t message_buffer;

/* This method trashes request_str (in place) */
cli_req_t parse_request(char *request_str) 
{
  char *token, *context_req, *context_arg;
  cli_req_t req;
  char req_delims[] = ":";
  char arg_delims[] = ",";
  token = strtok_r(request_str, req_delims, 
				   &context_req);
  req.client_name = token;
  token = strtok_r(NULL, req_delims,
				 &context_req);
  req.priority = atoi(token);
  token = strtok_r(NULL, req_delims,
				 &context_req);
  if (strcmp(token, START_MOVIE_STR) == 0) {
	req.request = start_movie;
	token = strtok_r(NULL, arg_delims,
				   &context_req);
	req.movie_name = token;
	token = strtok_r(NULL, arg_delims,
				   &context_req);
	req.repeat = atoi(token);
  } else if (strcmp(token, STOP_MOVIE_STR) == 0) {
	req.request = stop_movie;
	token = strtok_r(NULL, req_delims,
				   &context_req);
	req.movie_name = token;
  } else if (strcmp(token, SEEK_MOVIE_STR) == 0) {
	req.request = seek_movie;
	token = strtok_r(NULL, arg_delims,
				   &context_req);
	req.movie_name = token;
	token = strtok_r(NULL, arg_delims,
				   &context_req);
	req.frame_number = atoi(token);
  } else  { // Unrecognized request 
	DPRINTF(("Got invalid request type: %s\n", token));
  }
  
  return req;
  
}

void *movie_worker(void *worker_data)
{
  worker_data_t *data = (worker_data_t *) worker_data;
  DPRINTF(("New movie worker created for client %s, movie %s\n",
		   data->req.client_name, data->req.movie_name));
  pthread_mutex_lock(data->mutex);
  while (!data->cancelled) {
	sleep(1);
	pthread_mutex_unlock(data->mutex);
  }
  DPRINTF(("Stopping movie worker for client %s, movie %s\n",
		   data->req.client_name, data->req.movie_name));
  return NULL;
}
