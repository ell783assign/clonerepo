#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>
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

/****************************************************************************************/

static void print_usage(const char *bin_name)
{
	fprintf(stderr, "\n%s <file_to_copy> <destination>\n", bin_name);
}

static int32_t check_file_exists(const char *file_name)
{
	int32_t fd = -1;

	fd = open(file_name, O_RDONLY);
	if(fd==-1)
	{
		ERROR("Could not open file %s\n", file_name);
		perror("Could not open file");
	}
	else
	{
		TRACE("File exists!");
	}
	return(fd);
}


static int32_t strip_file_name(const char *path, char **dir_path, char **filename)
{
	char *dir = NULL;
	char *fname = NULL;

	int32_t ii;
	int32_t len = strlen(path);

	TRACE("Length: %d", len);
	int32_t ret_val = 0;

	/* Find the first '/' from the end and split at that point into 2 strings */
	for(ii = len; ii>=0; ii--)
	{
		if(path[ii] == '/')
		{
			break;
		}
	}

	if(ii<0)
	{
		/* Nothing to keep in the directory, everything is a file name*/
		*dir_path = NULL;
		fname = (char *)malloc(sizeof(char) * (len+1));
		if(fname==NULL)
		{
			ERROR("Error allocating memory for directory path!");
			ret_val = -1;
			goto EXIT_LABEL;
		}
		snprintf(fname, len+1, "%s", path);
		*filename = fname;
	}
	else
	{
		dir = (char *)malloc(sizeof(char) * (ii+2));
		if(dir==NULL)
		{
			ERROR("Error allocating memory for directory path!");
			ret_val = -1;
			goto EXIT_LABEL;
		}
		snprintf(dir, ii+2, "%s", path);/* snprintf must be given an additional length to accomodate a null character */
		if((len - ii)>0)
		{
			fname = (char *)malloc(sizeof(char) * ((len - ii)+1));
			if(dir==NULL)
			{
				ERROR("Error allocating memory for directory path!");
				ret_val = -1;
				goto EXIT_LABEL;
			}
			snprintf(fname, (len - ii), "%s", path + ii+1);
		}

		*dir_path = dir;
		*filename = fname==NULL? NULL: fname;
	}

EXIT_LABEL:
	return(ret_val);	
}

static int32_t check_path_valid(const char *path, int32_t recurse)
{
	int32_t ret_val = 0;

	int32_t size_reqd = 1024;
	char *cwd = NULL;

	char *dir_path=NULL;

	char *file_name=NULL;

retry:
	cwd = (char *)malloc(sizeof(char) * size_reqd);
	if(cwd==NULL)
	{
		ERROR("Could not allocate sufficient memory!");
		ret_val = -1;
		goto EXIT_LABEL;
	}

	if(getcwd(cwd, sizeof(char)*size_reqd)==NULL)
	{
		if(errno==ERANGE)
		{
			WARN("Need a bigger buffer size to save current directory!");
			free(cwd);
			cwd = NULL;
			size_reqd += size_reqd;
			goto retry;
		}
		else
		{
			ERROR("Could not perform operation.");
			perror("getcwd() failed");
			goto EXIT_LABEL;
		}
	}

	/* Check if it is a valid directory path. We should be able to cd to it */
	ret_val = chdir(path);
	if(ret_val != 0)
	{
		if(errno==ENOENT || errno ==ENOTDIR)
		{
			/* Could not cd to it. That implies it is not a valid path. Could be a file, check if file exists */
			if(check_file_exists(path)>=0)
			{
				ERROR("File exists! Cannot copy!");
				ret_val = -1;
				goto EXIT_LABEL;
			}
			/* Else, everything but last segment of the path must be a valid directory */
			ret_val = strip_file_name(path, &dir_path, &file_name);
			if(ret_val==-1)
			{
				ERROR("Error occured while stripping file from path.");
				ret_val = -1;
			}
			else
			{
				TRACE("Directory: %s", (dir_path==NULL? "NONE": dir_path));
				TRACE("Filename: %s", (file_name==NULL? "NONE": file_name));
			}

			/* Check if this directory is valid (just to be sure) */
			if(recurse==TRUE &&dir_path != NULL)
			{
				ret_val = check_path_valid(dir_path, FALSE); /* Do not recurse */
				if(ret_val == -1)
				{
					ERROR("Not a valid path!");
				}
			}
		}
		else
		{
			/* Something else*/
			ERROR("Could not cd into %s", path);
			perror("chdir failed:");
			goto EXIT_LABEL;
		}
	}
	else
	{
		/* Is a valid path. Just switch back to original one */
		TRACE("%s is a valid path!", path);
		ret_val = chdir(cwd);
		if(ret_val!=0)
		{
			ERROR("Something went wrong while switching back!");
			ret_val = -1;
		}
	}

EXIT_LABEL:
	return(ret_val);	
}

int32_t main(int32_t argc, char *argv[])
{
	int32_t ret_val= 0;

	int32_t fd = -1;

	/* 
	 * Input sanity check: There must be exactly 2 extra parameters provided
	 * 
	 * First parameter is the file to copy.
	 * Second parameter is a qualified name of the file to copy (move) into
	 * 
	 * There are two cases:
	 * The destination string may contain the new name of the file, or it may not.
	 * So, we need to check if it is a valid folder or file
	 */
	 if(argc<3)
	 {
	 	fprintf(stderr, "\nInsufficient parameters provided.");
	 	print_usage(argv[0]);
	 	goto EXIT_LABEL;
	 }

	 /* Check if file to copy is valid or not. */
	 fd = check_file_exists(argv[1]);
	 if(fd==-1)
	 {
	 	/* We've already printed the error message */
	 	goto EXIT_LABEL;
	 }

	 /* Check if destination is valid */
	 ret_val = check_path_valid(argv[2], TRUE);
	 if(ret_val == -1)
	 {
	 	ERROR("Not a valid path!");
	 }

EXIT_LABEL:
	return(ret_val);
}