#include <stdlib.h>
#include <stdbool.h>
#include "errors.h"
#include "ring_buffer.h"

ringbuff_t *ring_buffer_create(int size) 
{
  ringbuff_t *rb = (ringbuff_t *) calloc(1, sizeof(ringbuff_t));
  if (rb == NULL) {
	err_abort(0, "Calloc ringbuff_t");
  }
  rb->buff = (void **) calloc(size, sizeof(void *));
  if (rb->buff == NULL) {
	err_abort(0, "Calloc ring buffer array");
  }

  rb->buff_size = size;
  rb->buff_start = rb->num_items = 0;

  return rb;
}

/* 
   Adds the pointer to the ring buffer,
   returns 0 on success, BUFFER_FULL on
   error
*/
int ring_buffer_add(ringbuff_t *rb, void *item)
{
  int next_slot = 0;
  
  if (rb->num_items == rb->buff_size)
	return BUFFER_FULL;
  
  next_slot = (rb->buff_start + rb->num_items) % rb->buff_size;
  rb->buff[next_slot] = item;
  rb->num_items++;

  return 0;
}

/*
  Returns the item at the front of the ring buffer,
  or NULL if the buffer is empty
 */
void *ring_buffer_remove(ringbuff_t *rb)
{
  int first_item = -1;
  if (rb->num_items == 0)
	return NULL;
  
  first_item = rb->buff_start;
  rb->num_items--;
  rb->buff_start = (first_item + 1) % rb->buff_size;
  return rb->buff[first_item];
}

int ring_buffer_numitems(ringbuff_t *rb)
{
  return rb->num_items;
}

bool ring_buffer_full(ringbuff_t *rb)
{
  return rb->num_items == rb->buff_size;
}
