#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>
#include <errno.h>

#define BUF_LEN 1024
#define MAX_BUF_LEN  (BUF_LEN * 1024 * 10)		//10MB Max buffer size

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

/****************************************************************************************/
/* Globals for this program: It's not the best way, but I am just being lazy			*/

char *file_name = NULL;
char *dir_path = NULL;

char *src_file = NULL;
char *src_path = NULL;
/****************************************************************************************/

static void print_usage(const char *bin_name)
{
	fprintf(stderr, "\n%s <file_to_copy> <destination>\n", bin_name);
}

static int32_t check_file_exists(const char *file_name, int32_t return_fd)
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
	if(return_fd)
	{	
		return(fd);
	}
	else
	{
		close(fd);
		if(fd>=0)
		{
			return(1);
		}
		else
		{
			return(-1);
		}
	}
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

	extern char *dir_path;

	extern char *file_name;

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
			if(check_file_exists(path, FALSE)>=0)
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
		dir_path = (char *)path;
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

static int32_t do_copy(int32_t source, int32_t destination)
{
	int32_t ret_val = 0;
	TRACE("Copy!");

	char *buf;
	int32_t bytes_read = 0;
	int32_t bytes_written = 0;

	int32_t buf_len = BUF_LEN;

	static float print_progress = 0;

	buf = (char *)malloc(sizeof(char) * buf_len);
	if(buf==NULL)
	{
		ERROR("Error allocating memory for copy operation.");
		ret_val = -1;
		goto EXIT_LABEL;
	}

	fprintf(stdout, "\n");
	while((bytes_read = read(source, buf, buf_len))>0)
	{
		do
		{
			bytes_written = write(destination, buf, bytes_read);

			print_progress += bytes_written;
			fprintf(stdout, "\rCopied %fkB", print_progress/1024);

		}while(bytes_written >= 0 && bytes_written < bytes_read);

		if(bytes_written != bytes_read)
		{
			ERROR("Error occured while copying!");
			perror("Reason");
			ret_val = -1;
			break;
		}
		/* Double buffer (prospectively) for faster copy */
		buf_len += buf_len;

		buf_len = buf_len >= MAX_BUF_LEN? MAX_BUF_LEN: buf_len;

		buf = (char *)realloc(buf, sizeof(char) * buf_len);
		if(buf==NULL)
		{
			ERROR("Error allocating memory for copy operation.");
			ret_val = -1;
			break;
		}		
	}
EXIT_LABEL:	
	return(ret_val);
}

int32_t main(int32_t argc, char *argv[])
{
	int32_t ret_val= 0;

	int32_t fd = -1;

	int32_t dest_fd = -1;

	char *dest_path = NULL;

	unsigned int file_perms = 0;
	struct stat file_stats;
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
	 fd = check_file_exists(argv[1], TRUE);
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
	 	goto EXIT_LABEL;
	 }

	ret_val = strip_file_name(argv[1], &src_path, &src_file);
	if(ret_val==-1)
	{
		ERROR("Error occured while stripping file from path.");
		ret_val = -1;
	 	goto EXIT_LABEL;
	}

	/* Get soruce file permissions */
	/*
		S_IRWXU
		00700 user (file owner) has read, write and execute permission
		S_IRUSR
		00400 user has read permission
		S_IWUSR
		00200 user has write permission
		S_IXUSR
		00100 user has execute permission
		S_IRWXG
		00070 group has read, write and execute permission
		S_IRGRP
		00040 group has read permission
		S_IWGRP
		00020 group has write permission
		S_IXGRP
		00010 group has execute permission
		S_IRWXO
		00007 others have read, write and execute permission
		S_IROTH
		00004 others have read permission
		S_IWOTH
		00002 others have write permission
		S_IXOTH
	*/
	ret_val = stat(argv[1], &file_stats);
	if(ret_val == -1)
	{
		ERROR("Stat failed.");
		perror("Reason");
		goto EXIT_LABEL;
	}

	if(file_stats.st_mode & S_IRUSR)
	{
		file_perms |= S_IRUSR;
	}
	if(file_stats.st_mode & S_IWUSR)
	{
		file_perms |= S_IWUSR;
	}
	if(file_stats.st_mode & S_IXUSR)
	{
		file_perms |= S_IXUSR;
	}
	if(file_stats.st_mode & S_IRGRP)
	{
		file_perms |= S_IRGRP;
	}
	if(file_stats.st_mode & S_IWGRP)
	{
		file_perms |= S_IWGRP;
	}
	if(file_stats.st_mode & S_IXGRP)
	{
		file_perms |= S_IXGRP;
	}
	if(file_stats.st_mode & S_IROTH)
	{
		file_perms |= S_IROTH;
	}
	if(file_stats.st_mode & S_IWOTH)
	{
		file_perms |= S_IWOTH;
	}
	if(file_stats.st_mode & S_IXOTH)
	{
		file_perms |= S_IXOTH;
	}

	 /* OK. Now, open file in destination */
	 if(file_name==NULL)
 	 { 
 		file_name = src_file;	
 	 }

	 if(dir_path!=NULL)
	 {
	 	dest_path = (char *)malloc(sizeof(char)* (strlen(dir_path) + strlen(file_name)+1));
	 	snprintf(dest_path, (strlen(dir_path) + strlen(file_name)+2), "%s/%s", dir_path, file_name);	
	 }
	 else
	 {
	 	dest_path = (char *)malloc(sizeof(char)* (strlen(file_name)+1));
	 	snprintf(dest_path, (strlen(file_name)+1), "%s",file_name);	
	 }
	 

	 TRACE("Source File Path: %s", (src_path==NULL?"NONE": src_path));
	 TRACE("Source File Name: %s", (src_file==NULL?"NONE": src_file));
	 TRACE("Destination File Path: %s", (dir_path==NULL?"NONE": dir_path));
	 TRACE("Destination File Name: %s", (file_name==NULL?"NONE": file_name));

	 dest_fd = open(dest_path, O_WRONLY|O_CREAT| file_perms);
	 if(dest_fd<0)
	 {
	 	ERROR("Error opening descriptor for new file!");
	 	ret_val = -1;
	 	perror("Reason");
	 	goto EXIT_LABEL;
	 }

	 TRACE("Full path: %s", dest_path);
	 ret_val = do_copy(fd, dest_fd);
	 if(ret_val == -1)
	 {
	 	ERROR("Copy failed!");
	 	goto ERROR_COND;
	 }

	 /* Delete old file */
	 unlink(argv[1]);
ERROR_COND:	 
	 close(fd);
 	 close(dest_fd);
	 dest_fd = -1;


EXIT_LABEL:
	return(ret_val);
}