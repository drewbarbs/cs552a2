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

//TODO: implement
void ring_buffer_destroy(ringbuff_t *rb)
{
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

int ring_buffer_getsize(ringbuff_t *rb)
{
  return rb->num_items;
}

bool ring_buffer_full(ringbuff_t *rb)
{
  return rb->num_items == rb->buff_size;
}

/* Sorts the ring buffer according to a comparison function
   comp_func, which is expected to take two arguments of type (void*)
   (items in the ring buffer) and return 0 when the items being compared are
   equal, a positive number when the first item is greater than the second,
   and a negative number when the first item is less than the second */
void ring_buffer_sort(ringbuff_t *rb, int (*comp_func)(const void *a1, 
													   const void *a2))
{
  void *item, **temp_arr;
  unsigned int i = 0, num_items = ring_buffer_getsize(rb);
  if (ring_buffer_getsize(rb) == 0) {
	return;
  }
  temp_arr = malloc(sizeof(void *) * num_items);
  while ((item = ring_buffer_remove(rb)) != NULL) {
	temp_arr[i++] = item;
  }
  qsort(temp_arr, num_items, sizeof(void *), comp_func);
  for (i = 0; i < num_items; i++) {
	ring_buffer_add(rb, temp_arr[i]);
  }
  free(temp_arr);
}
