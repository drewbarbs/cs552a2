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
#define SERVER_PORT 5050
#define RECV_BUF_SIZE 1024
#define MSG_BUF_SIZE 100
#define MAX_FNAME_LEN 100

static pthread_mutex_t worker_pool_mutex = PTHREAD_MUTEX_INITIALIZER;
static thread_pool_t *tp;
static ringbuff_t *message_buffer;

void handle_client(void *cli_structp) 
{
  cli_t *cli = (cli_t *) cli_structp;
  cli_req_t req;
  int sd = cli->sd;
  char recvbuf[RECV_BUF_SIZE];
  char *request_str = NULL;
  int num_read;
  memset(&recvbuf, 0, RECV_BUF_SIZE);
  
  worker_data_t *worker_data = calloc(1, sizeof(worker_data_t));
  pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);
  worker_data->mutex = mutex;
  worker_data->msg_buf = message_buffer;
  worker_data->cancelled = false;
  
  if (cli->hostname != NULL) {
	DPRINTF(("Thread now serving client : %s\n", cli->hostname));
  }
  
  while (1) {
	num_read = read(sd, &recvbuf, RECV_BUF_SIZE-1);
	if (num_read > 0) {
	  /* Need to copy client request so that it can be
		 printed later (for debug purposes) */
	  free(request_str);
	  request_str = calloc(num_read + 1, sizeof(char));
	  strncpy(request_str, recvbuf, num_read);
	  /* Parse client request */
	  req = parse_request(recvbuf);
	  switch (req.request) {
	  case start_movie:
		worker_data->req = req;
		thread_pool_execute(tp, movie_worker, worker_data);
		break;
	  case seek_movie:
		pthread_mutex_lock(&mutex);
		worker_data->req = req;
		pthread_mutex_unlock(&mutex);
		break;
	  case stop_movie:
		pthread_mutex_lock(&mutex);
		worker_data->cancelled = true;
		pthread_mutex_unlock(&mutex);
		/* break; */
	  case invalid:
	  case close_conn:
		goto close_connection;
		// break;
	  default:
		err_abort(EINVAL, request_str);
	  }
	}
  }
 close_connection:

  free(request_str);
  if (cli->hostname != NULL) {
	free(cli->hostname);
  }
  free(cli);
  close(cli->sd);
  DPRINTF(("Client listener exiting\n"));
}

void servConn (int port) 
{

  int sd, new_sd, cli_len;
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

	thread_pool_execute(tp, handle_client, (void *) new_client);
	

  }
}

void destroy_cli(cli_t *cli) 
{
  free(cli->hostname);
  free(cli->request_str);
  free(cli);
}

int main () 
{
  tp = thread_pool_create(NUM_THREADS);
  message_buffer = ring_buffer_create(MSG_BUF_SIZE);
  servConn (SERVER_PORT); /* Server port. */

  return 0;
}