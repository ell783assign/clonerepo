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