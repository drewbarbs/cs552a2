#include "ring_buffer.h"
#include <stdio.h>

int main(int argc, char *argv[]) 
{
  int first = 1;
  int second = 2;
  int third = 3;
  int fourth = 4;
  int fifth = 5;
 
  ringbuff_t *rb = ring_buffer_create(5);
  
  printf("Adding item 1\n");
  ring_buffer_add(rb, &first);

  printf("Adding item 2\n");
  ring_buffer_add(rb, &second);

  printf("Adding item 3\n");
  ring_buffer_add(rb, &third);

  printf("Adding item 4\n");
  ring_buffer_add(rb, &fourth);

  printf("Adding item 5\n");
  ring_buffer_add(rb, &fifth);

  printf("Popping item: %d\n", * (int *) ring_buffer_remove(rb));
  printf("Popping item: %d\n", * (int *) ring_buffer_remove(rb));
  printf("Popping item: %d\n", * (int *) ring_buffer_remove(rb));
  printf("Popping item: %d\n", * (int *) ring_buffer_remove(rb));
  printf("Popping item: %d\n", * (int *) ring_buffer_remove(rb));

  return 0;
}
