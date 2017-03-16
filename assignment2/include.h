#ifndef INCLUDE_H
#define INCLUDE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include <trees.h>
#include <scheduler.h>
#include <output.h>


#ifdef BUILD_DEBUG
#define TRACE(...) 	fprintf(stderr, "TRACE  \t%10s\t%3d\t", __func__, __LINE__);fprintf(stderr, __VA_ARGS__)
#define WARN(...) 	fprintf(stderr, "WARN  \t%10s\t%3d\t", __func__, __LINE__);fprintf(stderr, __VA_ARGS__)
#define ERROR(...)  fprintf(stderr, "ERROR  \t%10s\t%3d\t", __func__, __LINE__);fprintf(stderr, __VA_ARGS__)
#define ENTRY()		fprintf(stderr, "TRACE \t%10s\t%3d Enter {\n",__func__, __LINE__);
#define EXIT()		fprintf(stderr, "TRACE \t%10s\t%3d Exit }\n",__func__, __LINE__);
#else
#define TRACE(...) 	
#define WARN(...) 	
#define ERROR(...) 
#define ENTRY()
#define EXIT()
#endif

#define CONSOLE(...) 	fprintf(stderr,__VA_ARGS__)
#define TRUE  		(uint32_t)1
#define FALSE 		(uint32_t)0

int32_t init_dispatcher(char *);
int32_t init_scheduler();
void reset_dispatcher();
void reset_sink();
int32_t init_sink();
void init_scheduler_comn(uint32_t , uint32_t , Feed_Jobs , JOB_SCHEDULER_COMN *);
int32_t get_jobs_at_instant(uint32_t , CLL *);

void spin_scheduler();

void insertion_sort_insert(JOB *, CLL *, long int , uint32_t);

#endif