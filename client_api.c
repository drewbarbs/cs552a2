#include "client_api.h"
#include "errors.h"
#include <string.h>
#include <stdlib.h>

/* Helper functions */
static void add_to_buffer(void *msg);

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
	  snprintf((char *) msg,
			   msg_len,
			   "thd %d:sd %d:msg #%d:%s\n",
			   thread_pool_getid(data->tp),
			   data->cli->sd,
			   ++msg_num,
			   original_req_str_cpy);
	}
	sleep(2);
	DPRINTF(("%s", (char *) msg));
	pthread_mutex_unlock(data->mutex);
  }
  DPRINTF(("Stopping movie worker for client %s, movie %s\n",
		   data->req.client_name, data->req.movie_name));
  free(original_req_str_cpy);
  free(data);
}
