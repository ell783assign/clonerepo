/*
 * program2.c
 *
 *  Created on: 09-Apr-2017
 *      Author: craft
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>

/****************************************************************************************/
/* Custom defines section.															    */
/****************************************************************************************/
#ifdef BUILD_DEBUG
#define TRACE(...) 	fprintf(stderr, "\nTRACE  \t%10s\t%3d\t", __func__, __LINE__);fprintf(stderr, __VA_ARGS__)
#define WARN(...) 	fprintf(stderr, "\nWARN  \t%10s\t%3d\t", __func__, __LINE__);fprintf(stderr, __VA_ARGS__)
#define ERROR(...)  fprintf(stderr, "\nERROR  \t%10s\t%3d\t", __func__, __LINE__);fprintf(stderr, __VA_ARGS__)
#define ENTRY()		fprintf(stderr, "\nTRACE \t%10s\t%3d Enter {",__func__, __LINE__);
#define EXIT()		fprintf(stderr, "\nTRACE \t%10s\t%3d Exit }",__func__, __LINE__);
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
/****************************************************************************************/

typedef struct circular_linked_list
{
	void *self;
	struct circular_linked_list *next;
	struct circular_linked_list *prev;
}CLL;

#define INIT_CLL_ROOT(LIST)					\
	(LIST).self = NULL;						\
	(LIST).prev = &(LIST);						\
	(LIST).next = &(LIST);

#define INIT_CLL_NODE(NODE,SELF)			\
	(NODE).self = (SELF);					\
	(NODE).next = NULL;					\
	(NODE).prev = NULL;

//Read as Insert A after B
#define INSERT_AFTER(A,B)					\
	(A).next = (B).next;						\
	(A).next->prev = &(A);						\
	(B).next = &(A);							\
	(A).prev = &(B);

//Read as Insert A before B
#define INSERT_BEFORE(A,B)					\
	(A).next = &(B);							\
	(B).prev->next = &(A);						\
	(A).prev = (B).prev;						\
	(B).prev = &(A);

#define REMOVE_FROM_LIST(A)					\
	(A).next->prev = (A).prev;					\
	(A).prev->next = (A).next;					\
	(A).prev = NULL;							\
	(A).next = NULL;

#define NEXT_IN_LIST(NODE)						\
	(NODE).next->self

#define PREV_IN_LIST(NODE)						\
	(NODE).prev->self

typedef struct path_list
{
	CLL node;
	char *path;
}PATH_LIST;

/****************************************************************************************/

CLL path;

/**
 * Execute command
 */
int32_t execute(char *cmd, char *params)
{
	int32_t ret_val = 0;

	return(ret_val);
}


/**
 * @brief Try to execute command
 *
 * When executing a command, the shell should first look for the command in the current directory,
 * and if not found, search the directories defined in a special variable, path.
 */
int32_t try_execute(char *cmd, char *params)
{
	int32_t ret_val = 0;

	PATH_LIST *path_node = NULL;

	char path_var[1024] = {0};

	ret_val = access(cmd, F_OK);
	if(ret_val == -1)
	{
		if(errno != ENOENT)
		{
			ERROR("Some error occurred!");
			ret_val = -1;
			goto EXIT_LABEL;
		}
		TRACE("Command does not exist in CWD");

		/* Look in path variables. */
		for(path_node = (PATH_LIST *)NEXT_IN_LIST(path);
			path_node != NULL;
			path_node = (PATH_LIST *)NEXT_IN_LIST(path_node->node))
		{
			snprintf(path_var, sizeof(path_var),"%s/%s", path_node->path, cmd);
			ret_val = access(cmd, F_OK);
			if(ret_val == -1)
			{
				TRACE("Command does not exist in %s", path_node->path);
			}
			else
			{
				ret_val = execute(path_var, params);
			}
		}
	}

EXIT_LABEL:
	return(ret_val);
}

int32_t main(void)
{

	INIT_CLL_ROOT(path);

	return(0);
}
