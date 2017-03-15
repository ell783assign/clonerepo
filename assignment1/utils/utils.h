#ifndef ASSIGNMENT_1_H_
#define ASSIGNMENT_1_H_

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <arpa/inet.h>

#include <dirent.h>

#include <sys/wait.h> 

#include <ftp_interface.h>

int32_t alt_gets(char *);

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

#define TRUE  		(uint32_t)1
#define FALSE 		(uint32_t)0

#define EXIT_ON_ERROR(...)				exit_on_error(__VA_ARGS__, TRUE)
#define LOG_EXCEPTION(...)				exit_on_error(__VA_ARGS__, FALSE)

typedef enum
{
	EQ = 0,
	NEQ,
	GT,
	LT,
	GTE,
	LTE,

	__MAX_MEMBER__
} COMPARE_RULE;

/**
 * @brief      Helper function (a macro) to handle error on function returns
 *
 * @param      msg           The message to be printed as diagnostic. NULL if nothing.
 * @param[in]  err_val       The error value that was returned.
 * @param[in]  expected_val  The expected value that should have been returned.
 * @param[in]  has_perror    Indicates if perror message is present for this condition
 * @param[in]  exit_prog	 Indicates if program must terminate on error or just log exception.
 */
static inline void exit_on_error(char *msg, int32_t err_val, COMPARE_RULE rule, int32_t expected_val, uint32_t has_perror, uint32_t exit_prog)
{
	int32_t error_occurred = 1;
	switch(rule)
	{
		case EQ:
			if(err_val != expected_val)
			{
				goto EXIT_LABEL;
			}
			break;
		case NEQ:
			if(err_val == expected_val)
			{
				goto EXIT_LABEL;
			}
			break;
		case GT:
			if(err_val <= expected_val)
			{
				goto EXIT_LABEL;
			}
			break;
		case LT:
			if(err_val >= expected_val)
			{
				goto EXIT_LABEL;
			}
			break;
		case GTE:
			if(err_val < expected_val)
			{
				goto EXIT_LABEL;
			}
			break;	
		case LTE:
			if(err_val > expected_val)
			{
				goto EXIT_LABEL;
			}
			break;
	}
	/* If we reach here, everything went fine. */
	error_occurred = 0;

EXIT_LABEL:	
	if(error_occurred)
	{
		if(exit_prog)
		{
			if(msg)
			{
				ERROR("%s\n", msg);
			}
			ERROR("Received: %d\t Expected: %d\n", err_val, expected_val);
			if(has_perror)
			{
				perror("Diagnostic");
			}
			exit(0);
		}
		else
		{
			if(msg)
			{
				WARN("%s\n", msg);
			}
			WARN("Received: %d\t Expected: %d\n", err_val, expected_val);
			if(has_perror)
			{
				perror("Diagnostic");
			}
		}
	}

	return;
}/**< exit_on_error */

#define MAX_DIR_BUF_LEN				1024



int32_t read_file_to_buffer(char *name, char **msg_buf, int32_t *length);
int32_t write_file_to_disk(char *name, char *msg_buf, int32_t length);
int32_t send_data(int32_t, uint32_t, char *);
int32_t receive_data(int32_t, uint32_t, char *);

#endif