#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include "ring_buffer.h"
#include "thread_pool.h"

#define START_MOVIE_STR "start_movie"
#define STOP_MOVIE_STR "stop_movie"
#define SEEK_MOVIE_STR "seek_movie"

#define M 10
#define MSG_BUF_SIZE 100

typedef enum REQUEST { start_movie, stop_movie, seek_movie, close_conn, invalid } REQUEST;

typedef struct buf_item {
  void *data;
  int datalen;
  int sd;
  int priority;
} buf_item;

typedef struct cli_t {
  int sd; // TCP socket for connection to client
  char *hostname;
} cli_t;

typedef struct cli_req_t {
  char *client_name;
  int priority;
  REQUEST request;
  char *movie_name;
  bool repeat;
  int frame_number;
} cli_req_t;

typedef struct rb_struct {
  ringbuff_t *buf;
  pthread_cond_t *has_space;
  pthread_cond_t *has_greater_than_M;
  pthread_mutex_t *mutex;
} rb_struct;

typedef struct worker_data_t {
  char *tokenized_req_str;
  char *original_req_str;
  cli_req_t req;
  bool cancelled;
  cli_t *cli;
  pthread_mutex_t *mutex;
  rb_struct *msg_rb;
  thread_pool_t *tp;
} worker_data_t;

cli_req_t parse_request(char *request_str);
void movie_worker(void *worker_data);
rb_struct *rb_struct_create(void);
void rb_struct_destroy(rb_struct*);
