/*
  server.c - Implements the server component of this project.
  Uses the socket code from the "servConn" function provided in 
  the "support" files for this project largely entirely unmodified.
*/

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "thread_pool.h"
#include "ring_buffer.h"
#include "errors.h"
#include "client_api.h"

#define MAX_BACKLOG 10 /* Max length to which queue for TCP connections
						  (not yet accepted) may grow */

#define NUM_THREADS 5
#define SERVER_PORT 5051
#define RECV_BUF_SIZE 1024
#define MAX_FNAME_LEN 100

static pthread_mutex_t worker_pool_mutex = PTHREAD_MUTEX_INITIALIZER;
static thread_pool_t *tp;
//static ringbuff_t *message_buffer;
static rb_struct *msg_rb;

/* Helper functions */
static void destroy_cli(cli_t *cli);
static void client_listen(void *cli_structp) ;
static void dispatch_func(void *);

void destroy_cli(cli_t *cli) 
{
  close(cli->sd);
  if (cli->hostname != NULL) {
	free(cli->hostname);
  }
  free(cli);
}

int sort_func(const void *arg1, const void *arg2)
{
  buf_item *a1 = *((buf_item **) arg1);
  buf_item *a2 = *((buf_item **) arg2);
  if (a1->priority < a2->priority) {
	return -1;
  } else if (a1->priority == a2->priority) {
	if (a1->msg_num < a2->msg_num)
	  return -1;
	else if (a1->msg_num == a2->msg_num)
	  return 0;
	else
	  return 1;
  } else {
	return 1;
  }
}

void dispatch_func(void *arg) 
{
  //  rb_struct *msg_rb = (rb_struct *) arg;
  buf_item *item = NULL;
  bool buf_was_full;
  while (1) {
	pthread_mutex_lock(msg_rb->mutex);
	while (ring_buffer_getsize(msg_rb->buf) < M) {
	  pthread_cond_wait(msg_rb->has_greater_than_M, msg_rb->mutex);
	}
	buf_was_full = ring_buffer_full(msg_rb->buf);
	ring_buffer_sort(msg_rb->buf, sort_func);
	while ((item = (buf_item *) ring_buffer_remove(msg_rb->buf)) != NULL) {
	  write(item->sd, item->data, item->data_len);
	  DPRINTF(("Dispatcher Printing %s\n", item->data));
	  free(item->data);
	  free(item);
	}
	if (buf_was_full) {
	  pthread_cond_signal(msg_rb->has_space);
	}
	pthread_mutex_unlock(msg_rb->mutex);
  }
}

void client_listen(void *cli_structp) 
{
  cli_t *cli = (cli_t *) cli_structp;
  cli_req_t req;
  int sd = cli->sd;
  char recvbuf[RECV_BUF_SIZE];
  char *req_str_to_tokenize = NULL, *original_req_str = NULL;
  int num_read;
  memset(&recvbuf, 0, RECV_BUF_SIZE);
  
  /* It will be the worker's responsibility to free
	 his worker_data */
  worker_data_t *worker_data = calloc(1, sizeof(worker_data_t));
  pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);
  worker_data->cli = cli;
  worker_data->mutex = mutex;
  worker_data->msg_rb = msg_rb;
  worker_data->tp = tp; /* Worker thread needs this to find out its
						   thread id */
  worker_data->cancelled = false;
  
  if (cli->hostname != NULL) {
	DPRINTF(("Thread now serving client : %s\n", cli->hostname));
  }
  
  while (1) {
	num_read = read(sd, &recvbuf, RECV_BUF_SIZE-1);
	if (num_read > 0) {
	  /* Need to copy client request so that it can be
		 printed later (for debug purposes) */
	  req_str_to_tokenize = calloc(num_read + 1, sizeof(char));
	  original_req_str = calloc(num_read + 1, sizeof(char));
	  strncpy(req_str_to_tokenize, recvbuf, num_read);
	  strncpy(original_req_str, recvbuf, num_read);
	  /* Parse client request */
	  req = parse_request(req_str_to_tokenize);
	  switch (req.request) {
	  case start_movie:
		worker_data->req = req;
		worker_data->original_req_str = original_req_str;
		worker_data->tokenized_req_str = req_str_to_tokenize;
		thread_pool_execute(tp, movie_worker, worker_data);
		break;
	  case seek_movie:
		pthread_mutex_lock(mutex);
		worker_data->req = req;
		worker_data->original_req_str = original_req_str;
		worker_data->tokenized_req_str = req_str_to_tokenize;
		pthread_mutex_unlock(mutex);
		break;
	  case stop_movie:
	  case invalid:
	  case close_conn:
		pthread_mutex_lock(mutex);
		worker_data->cancelled = true;
		pthread_mutex_unlock(mutex);
		goto close_connection;
		// break;
	  default:
		err_abort(EINVAL, original_req_str);
	  }
	}
  }
 close_connection:

  /* Need to ensure that mutex is unlocked before
	 trying to destroy it */
  pthread_mutex_lock(mutex);
  pthread_mutex_unlock(mutex);
  pthread_mutex_destroy(mutex);
  destroy_cli(cli);
  DPRINTF(("Client listener exiting\n"));
}

void servConn (int port) 
{

  int sd, new_sd;
  unsigned int cli_len;
  struct sockaddr_in name, cli_name;
  cli_t *new_client;
  char *cli_hbuf;
  size_t cli_hsize;
  int sock_opt_val = 1;
  
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("(servConn): socket() error");
    exit (-1);
  }

  if (setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, (char *) &sock_opt_val,
				  sizeof(sock_opt_val)) < 0) {
    perror ("(servConn): Failed to set SO_REUSEADDR on INET socket");
    exit (-1);
  }

  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  
  if (bind (sd, (struct sockaddr *)&name, sizeof(name)) < 0) {
    perror ("(servConn): bind() error");
    exit (-1);
  }

  listen (sd, MAX_BACKLOG);

  for (;;) {
	cli_len = sizeof (cli_name);
	new_sd = accept (sd, (struct sockaddr *) &cli_name, &cli_len);
	if (new_sd < 0) {
	  perror ("(servConn): accept() error");
	  exit (-1);
	}
	DPRINTF(("Assigning new socket descriptor:  %d\n", new_sd));
	DPRINTF(("Serving %s:%d\n", inet_ntoa(cli_name.sin_addr),
			 ntohs(cli_name.sin_port)));

	new_client = calloc(1, sizeof(cli_t));
	new_client->sd = new_sd;

	cli_hbuf = calloc(1025, sizeof(char));
	cli_hsize = 1025;
	
	if (getnameinfo((struct sockaddr *) &cli_name, sizeof(cli_name), 
					cli_hbuf, cli_hsize,
					NULL, 0, 0)) {
	  DPRINTF(("Error resolving hostname\n"));
	  free(cli_hbuf);
	  new_client->hostname = NULL;
	} else {
	  new_client->hostname = cli_hbuf;
	}

	thread_pool_execute(tp, client_listen, (void *) new_client);
  }
}

int main () 
{
  tp = thread_pool_create(NUM_THREADS);
  msg_rb = rb_struct_create();
  thread_pool_execute(tp, dispatch_func, NULL);
  servConn (SERVER_PORT); /* Server port. */

  return 0;
}
