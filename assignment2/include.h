#ifndef INCLUDE_H
#define INCLUDE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define TRUE  		(uint32_t)1
#define FALSE 		(uint32_t)0

#define TRACE(...) 	fprintf(stderr, "TRACE  \t" __VA_ARGS__)
#define WARN(...) 	fprintf(stderr, "WARNING\t"__VA_ARGS__)
#define ERROR(...)  fprintf(stderr, "ERROR  \t"__VA_ARGS__)

#endif