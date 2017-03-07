#include <utils.h>
#include <cli_interface.h>

int32_t alt_gets(char *buffer)
{
	char buf[MAXCMDLENGTH];

	memset(buf, '\0', sizeof(buf));

	if(fgets(buf, sizeof(buf), stdin))
	{
		/* remove trailing \n */
		if(strlen(buf)>0 && buf[strlen(buf)-1]=='\n')
		{
			snprintf(buffer, strlen(buf), "%s", buf);
		}
		else
		{
			snprintf(buffer, strlen(buf), "%s", buf);	
		}
		return(0);
	}
	return(-1);
	
}
int32_t read_file_to_buffer(char *name, char **msg_buf, int32_t *length)
{
	int32_t ret_val = 0;

	uint32_t size_of_file = 0;
	char *buffer = NULL;

	FILE *fp = fopen(name, "rb");
	if(fp==NULL)
	{
		LOG_EXCEPTION("Could not open file. Does it exist?", 0, NEQ, 0, TRUE);
		ret_val = -1;
		goto EXIT_LABEL;
	}

	/* Determine file size */
	fseek(fp, 0L, SEEK_END);
	size_of_file = ftell(fp);
	/* Go back */
	fseek(fp, 0L, SEEK_SET);

	buffer = (char *)malloc(sizeof(char)*size_of_file);
	if(buffer==NULL)
	{
		LOG_EXCEPTION("Could not allocate sufficient buffer.", 0, NEQ, 0, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;
	}
	*length = fread(buffer, sizeof(unsigned char), size_of_file, fp);
	{
		LOG_EXCEPTION("Could not completely write into buffer.", *length, EQ, size_of_file, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;
	}
	/* Otherwise we have successfully written data. */
	*msg_buf = buffer;

EXIT_LABEL:
	if(fp)
	{
		fclose(fp);
		fp = NULL;
	}
	return(ret_val);	
}

int32_t write_file_to_disk(char *name, char *msg_buf, int32_t length)
{
	int32_t ret_val = 0;
	int32_t size_written = 0;

	FILE *fp = fopen(name, "wb+");
	if(fp==NULL)
	{
		LOG_EXCEPTION("Could not create file.", 0, NEQ, 0, TRUE);
		ret_val = -1;
		goto EXIT_LABEL;
	}

	size_written = fwrite(msg_buf, sizeof(char), length, fp);
	{
		LOG_EXCEPTION("Could not completely write into file.", size_written, EQ, length, FALSE);
		ret_val = -1;
	}

EXIT_LABEL:
	if(fp)
	{
		fclose(fp);
		fp = NULL;
	}
	return(ret_val);
}

int32_t send_data(int32_t sock_fd, uint32_t length, char *buf)
{
	int32_t num_writes = 0;

	num_writes = send(sock_fd, buf, length, 0);
	{
		LOG_EXCEPTION("Could not send all data.", num_writes, EQ, length, TRUE);
		num_writes = -1;
	}
	return(num_writes);
}

int32_t receive_data(int32_t sock_fd, uint32_t length, char *buf)
{
	int32_t num_reads = 0;

	num_reads = recv(sock_fd, buf, length, 0);
	{
		LOG_EXCEPTION("Could not read all data.", num_reads, EQ, length, TRUE);
		num_reads = -1;
	}
	return(num_reads);	
}


int32_t do_lls(uint32_t *length, char **msg)
{
	TRACE("enter do_lls\n");
	int32_t ret_val = 0;

	char *return_buffer = (char *)malloc(sizeof(char) * 1024);
	if(!return_buffer)
	{
		EXIT_ON_ERROR("Could not allocate sufficient memory to save output.", 0, NEQ, 0, FALSE);
	}
	char *current_ptr = return_buffer;
	int32_t run_length = 0;
	int32_t max_len = 1024;

	DIR *current_dir = NULL;
	struct dirent *file_iterator;

	unsigned char dir_name[MAX_DIR_BUF_LEN];

	if(getcwd(dir_name, sizeof(dir_name)) == NULL)
	{
		LOG_EXCEPTION("Failed to get directory structure.", 0, NEQ, 0, TRUE);
		ret_val = -1;
		goto EXIT_LABEL;
	}

	TRACE("Inside %s\n", dir_name);
	current_dir = opendir(dir_name);
	if(!current_dir)
	{
		EXIT_ON_ERROR("Could not open directory for parsing files.", 0, NEQ, 0, TRUE);
	}
	while((file_iterator = readdir(current_dir)) !=NULL)
	{
		/* If memory is not sufficient, grow buffer */
		while(run_length+strlen(file_iterator->d_name)+2 > max_len)/* +2 for \n and NULL character */
		{
			max_len += 1024;
			return_buffer = (char *)realloc(return_buffer, max_len);
			if(!return_buffer)
			{
				EXIT_ON_ERROR("Could not allocate sufficient memory to save output.", 0, NEQ, 0, FALSE);
			}
			/* Realign current_ptr*/
			current_ptr = (char *)return_buffer + run_length;
		}
		/* now we have sufficiently large buffer. Copy results */
		snprintf(current_ptr, strlen(file_iterator->d_name)+1, "\n%s",file_iterator->d_name);
		TRACE("%s\n", current_ptr);
		/* increment runlength*/
		run_length +=strlen(file_iterator->d_name)+1;/* +1 for \n */
		/* advance current_ptr*/
		current_ptr += strlen(file_iterator->d_name)+1;
		//fprintf(stdout, "%s\n", file_iterator->d_name);
	}

	closedir(current_dir);

	*msg = return_buffer;
	*length = run_length+1;/*1 for NULL character */

EXIT_LABEL:
	TRACE("exit do_lls\n");
	return 	(ret_val);
}

int32_t do_lcd(char *path, uint32_t *length, char **msg)
{
	int32_t ret_val = 0;
	char *err_msg = NULL;

	ret_val = chdir((const char *)path);
	if(ret_val!=0)
	{
		LOG_EXCEPTION("Error occurred while changing directory.", ret_val, EQ, 0, TRUE);
		err_msg = (char *)malloc(sizeof(char) * 512);
		if(!err_msg)
		{
			EXIT_ON_ERROR("Error allocating memory.", 0, NEQ, 0, FALSE);
		}
		sprintf(err_msg, "Error occurred while changing directory.");
		*msg = err_msg;
	}

	return(ret_val);
}

int32_t do_lchmod(int32_t perm[3], char * f_name, uint32_t *length, char **msg)
{
	/** https://www.gnu.org/software/libc/manual/html_node/Permission-Bits.html */
	int32_t ret_val = 0;
	char *err_msg = NULL;

	struct stat file_stats;
	uint32_t mask;
	mode_t mode;

	stat(f_name, &file_stats);

	
	mode = file_stats.st_mode & 0x7777;

	mask = 0x0000 | perm[0]<<16 | perm[1]<<8 | perm[2];
	mode = mode | mask;
	
	ret_val = chmod(f_name, mode);
	if(ret_val!=0)
	{
		LOG_EXCEPTION("Error while changing permissions.", ret_val, EQ, 0, TRUE);
		err_msg = (char *)malloc(sizeof(char)*512);
		sprintf(err_msg, "Error while changing permissions.\n");
		*msg = err_msg;
	}

	return (ret_val);
}