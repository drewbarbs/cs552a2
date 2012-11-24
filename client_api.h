#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include "ring_buffer.h"

#define START_MOVIE_STR "start_movie"
#define STOP_MOVIE_STR "stop_movie"
#define SEEK_MOVIE_STR "seek_movie"


typedef enum REQUEST { start_movie, stop_movie, seek_movie, close_conn, invalid } REQUEST;

typedef struct cli_t {
  int sd; // TCP socket for connection to client
  char *hostname;
  char *request_str;
} cli_t;

typedef struct cli_req_t {
  char *client_name;
  int priority;
  REQUEST request;
  char *movie_name;
  bool repeat;
  int frame_number;
  char *original_str;
} cli_req_t;

typedef struct worker_data_t {
  cli_req_t req;
  bool cancelled;
  pthread_mutex_t *mutex;
  ringbuff_t *msg_buf;
} worker_data_t;


cli_req_t parse_request(char *request_str);

void *movie_worker(void *worker_data);