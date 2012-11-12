all:
	gcc -Wall -g -pthread thread_pool.c ring_buffer.c
debug:
	gcc -Wall -g -DDEBUG -DNDEBUG -pthread thread_pool.c ring_buffer.c
clean:
	ls | grep -v '.c\|.h\|Makefile\|README\|.txt' | xargs rm -rf