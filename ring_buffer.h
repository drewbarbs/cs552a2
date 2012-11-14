/* Not thread safe */
#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

#define BUFFER_FULL 1

typedef struct ringbuff_t {
  void **buff;
  int buff_size;
  int buff_start; // Index of the first unclaimed item on 
  int num_items;

} ringbuff_t;

ringbuff_t *ring_buffer_create(int);
void ring_buffer_destroy(ringbuff_t *);
int ring_buffer_add(ringbuff_t *, void *);
void *ring_buffer_remove(ringbuff_t *);
int ring_buffer_getsize(ringbuff_t *);

#endif
