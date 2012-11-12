#ifndef __ERRORS_H
#define __ERRORS_H

/*
 * Debug Macros from "Programming with POSIX Threads"
 *
 *
 */
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
# define DPRINTF(arg) printf arg
#else
# define DPRINTF(arg)
#endif

//#ifdef DEBUG
#define err_abort(code, text) do { \
  fprintf (stderr, "%s at \"%s\":%d: %s\n", \
		   text, __FILE__, __LINE__, strerror (code)); \
  abort(); \
  } while (0)
#define errno_abort(text) do { \
  fprintf (stderr, "%s at \"%s\":%d: %s\n", \
		   text, __FILE__, __LINE__, strerror (errno)); \
  abort(); \
  } while (0)
//#endif
#endif
