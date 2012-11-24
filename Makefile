all:
	gcc -Wall -g -pthread server.c thread_pool.c ring_buffer.c client_api.c
debug:
	gcc -Wall -g -DDEBUG -DNDEBUG -pthread server.c thread_pool.c ring_buffer.c client_api.c
clean:
	ls | grep -v '.c\|.h\|Makefile\|README\|.txt' | xargs rm -rf