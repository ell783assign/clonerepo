#include <utils.h>

/* For now, it is just a placeholder. */

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


int32_t do_lls(char **msg)
{
	int32_t ret_val = 0;

	DIR *current_dir = NULL;
	struct dirent *file_iterator;

	unsigned char dir_name[MAX_DIR_BUF_LEN];

	if(getcwd(dir_name, sizeof(dir_name)) == NULL)
	{
		LOG_EXCEPTION("Failed to get directory structure.", 0, NEQ, 0, TRUE);
		ret_val = -1;
		goto EXIT_LABEL;
	}

	current_dir = opendir(dir_name);
	while((file_iterator = readdir(current_dir)) !=NULL)
	{
		fprintf(stdout, "%s\n", file_iterator->d_name);
	}
	closedir(current_dir);

	*msg = NULL;

EXIT_LABEL:
	return 	(ret_val);
}

int32_t do_lcd(char *path, char **msg)
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

int32_t do_lchmod(int32_t perm[3], char * f_name, char **msg)
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