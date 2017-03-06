#include <utils.h>

int32_t connect_to_server(uint32_t, char *);

int32_t sock_fd;

/**
 * @brief      Program to open a Connection socket for FTP server.
 *
 * @param[in]  argc  The argc
 * @param      argv  The argv
 *
 * @return     Nothing Useful
 */
int32_t main(int32_t argc, char *argv[])
{
	int32_t ret_val;

	uint32_t dest_port = 5000; /**< Default value */

	extern int32_t sock_fd;	/**< Connect Socket FD */
	char *server_address=NULL; /**< Server Listen Address */

	int wordsfound,result,perm[3],permission;
	char **msg,userinput[MAXCMDLENGTH];

	puts("+---------------------+");
	puts("|Welcome to FTP client|");
	puts("+---------------------+");

	if(argc >= 2)
	{
		dest_port = atoi(argv[1]);
	}
	if(argc ==3)
	{
		server_address = argv[2];
	}
	else
	{
		server_address = "127.0.0.1";
	}

	sock_fd = connect_to_server(dest_port, server_address);
	EXIT_ON_ERROR(NULL, sock_fd, GT, 0, FALSE);

	/* Start CLI session */
	while(TRUE)
	{	
		printf("ftp>");
		gets(userinput);
		wordsfound=processthiscmd(userinput);
		if(wordsfound==0)
			continue;
		result=99;
		switch(cmdnum(cmdargs[0]))
		{	
			case 1:	
				if(wordsfound==1)
				{
					result=do_ls(msg);
				}
				break;
			case 2:	
				if(wordsfound==2)
				{
					result=do_cd(cmdargs[1],msg);
				}
				break;
			case 3:	
				if(wordsfound==3)
				{	permission=atoi(cmdargs[1]);
					if(strlen(cmdargs[1])==3 && permission>=0 && permission<=777)
					{	perm[0]=permission/100;
						perm[2]=permission%10;
						perm[1]=(permission-100*perm[0]-perm[2])/10;
						result=do_chmod(perm,cmdargs[2],msg);
					}
				}
				break;
			case 4:	
				if(wordsfound==1)
				{
					result=do_lls(msg);
				}
				break;
			case 5:	
				if(wordsfound==2)
				{
					result=do_lcd(cmdargs[1],msg);
				}
				break;
			case 6:	
				if(wordsfound==3)
				{	permission=atoi(cmdargs[1]);
					if(strlen(cmdargs[1])==3 && permission>=0 && permission<=777)
					{	perm[0]=permission/100;
						perm[2]=permission%10;
						perm[1]=(permission-100*perm[0]-perm[2])/10;
						result=do_lchmod(perm,cmdargs[2],msg);
					}
				}
				break;
			case 7:	
				if(wordsfound==2)
				{
					result=do_put(cmdargs[1],msg);
				}
				break;
			case 8:	
				if(wordsfound==2)
				{
					result=do_get(cmdargs[1],msg);
				}
				break;
			case 9:	
				if(wordsfound==1)
				{
					result=do_close(msg, sock_fd);
				}
				goto EXIT_LABEL;

			default:	
				{
					result=0;
					printf("Command not present\n");
				}
		}
		if(result==-1)
			printdiagnosticmsg(msg);
		else if(result==99)
			printf("Command with inadequate/surplus/wrong arguments\n");
	}

EXIT_LABEL:	
	return(0);
}

/**
 * @brief      Opens network connection to the server.
 *
 * @param[in]  connect_port  The port on which server is listening
 * @param[in]  dest_address  String representation of destination server. NULL if local-host
 *
 * @return     Descriptor of connection socket if successful, else -1.
 */
int32_t connect_to_server(uint32_t connect_port, char *dest_address)
{
	int32_t ret_val;

	int32_t sock_fd;
	struct sockaddr_in server_address; /**< Server Listen Address */

	TRACE("Opening UDP Socket\n");
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	EXIT_ON_ERROR("Could not open stream socket.", sock_fd, GTE, 0, TRUE);


	/* Setup address to bind to */
	memset(&server_address, 0, sizeof(struct sockaddr));
	inet_pton(AF_INET, (const char *)dest_address, (void *) &server_address.sin_addr.s_addr);
	server_address.sin_port = htons(connect_port);
	server_address.sin_family = AF_INET;

	TRACE("Connecting to server at %s:%d\n", dest_address, connect_port);
	ret_val = connect(sock_fd, (struct sockaddr *)&server_address, sizeof(struct sockaddr));
	EXIT_ON_ERROR("Failed to connect to server address.", ret_val, EQ, 0, TRUE);
	TRACE("Connected to server...\n");

	return(sock_fd);
}

int32_t do_lls(char **msg)
{
	int32_t ret_val = 0;

	DIR *current_dir = NULL;
	struct dirent *file_iterator;

	unsigned char dir_name[MAX_DIR_BUF_LEN];

	if(getcwd(dir_name, sizeof(dir_name) == NULL)
	{
		LOG_EXCEPTION("Failed to get directory structure.", NULL, NEQ, NULL, TRUE);
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
	{
		LOG_EXCEPTION("Error occurred while changing directory.", ret_val, EQ, 0, TRUE);
		err_msg = (char *)malloc(sizeof(char) * 512);
		EXIT_ON_ERROR("Error allocating memory.", err_msg, NEQ, NULL, FALSE);
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
	{
		LOG_EXCEPTION("Error while changing permissions.", ret_val, EQ, 0, TRUE);
		err_msg = (char *)malloc(sizeof(char)*512);
		sprintf(err_msg, "Error while changing permissions.\n");
		*msg = err_msg;
	}

	return (ret_val);
}

void do_close(int32_t fd)
{
	if(fd > 0)
	{
		close(fd);
	}
	return;
}

int32_t do_ls(char **cmd_msg)
{
	int32_t ret_val = 0;
	char *err_msg = NULL;

	MSG_LS *msg = NULL;

	char *ls_result = NULL;
	MSG_COMN *comn_msg = NULL;

	msg = (MSG_LS *)malloc(sizeof(MSG_LS));
	{
		LOG_EXCEPTION("Error allocating buffers.",msg, NEQ, NULL, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;
	}
	memset(msg, 0, sizeof(MSG_LS));

	msg->cmd = LS;
	msg->is_request = TRUE;
	msg->response = 0; /* Not applicable. */
	msg->length = sizeof(MSG_LS);

	ret_val = send_data(sock_fd, sizeof(MSG_LS), (char *)msg);
	{
		LOG_EXCEPTION("Error sending data to server.", ret_val, EQ, sizeof(MSG_LS), FALSE);
		goto EXIT_LABEL;
	}

	memset(msg, 0, sizeof(MSG_LS));
	ret_val = receive_data(sock_fd, sizeof(MSG_HDR), msg);
	{
		LOG_EXCEPTION("Error receiving response",ret_val, EQ, 0, FALSE);	
		goto EXIT_LABEL;
	}

	/* Some response was received. */
	ls_result = (char *)malloc((MSG_HDR *)msg->length - sizeof(MSG_HDR));
	{
		LOG_EXCEPTION("Error allocating buffers.",ls_result, NEQ, NULL, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;	
	}

	/* Receive all data from socket anyway, even if we have to discard it. */
	ret_val = receive_data(sock_fd, (MSG_HDR *)msg->length - sizeof(MSG_HDR), ls_result);
	{
		LOG_EXCEPTION("Error receiving response",ret_val, EQ, 0, FALSE);	
		goto EXIT_LABEL;
	}
	comn_msg = ls_result;

	if(!((MSG_HDR *)msg->is_request) && !((MSG_HDR *)msg->response == 1))
	{
		if(!((MSG_HDR *)msg->is_request))
		{
			LOG_EXCEPTION("Malformed reply!", 0, EQ, 1, FALSE);
			ret_val = -1;
			goto EXIT_LABEL;	
		}

		/* Else, error at server. Read error message. */
		*cmd_msg = ls_result;
	}
	else
	{
		/* Else, ls output was sent. Print it out. */
		fprintf(stdout, "%s\n", (char *)comn_msg);	
		*msg = NULL;
	}
	
EXIT_LABEL:
	if(msg != NULL)
	{
		free(msg);
		msg = NULL;
	}
	if(ls_result != NULL && ret_val != -1)
	{
		free(ls_result);
		ls_result = NULL;
	}

	return(ret_val);	
}

int32_t do_cd(char *path, char **cmd_msg)
{
	int32_t ret_val = 0;
	char *err_msg = NULL;

	MSG_CD *msg = NULL;

	char *ls_result = NULL;
	MSG_COMN *comn_msg = NULL;

	msg = (MSG_LS *)malloc(sizeof(MSG_CD) + strlen(path));
	{
		LOG_EXCEPTION("Error allocating buffers.",msg, NEQ, NULL, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;
	}
	memset(msg, 0, sizeof(MSG_LS));

	msg->cmd = CD;
	msg->is_request = TRUE;
	msg->response = 0; /* Not applicable. */
	msg->length = sizeof(MSG_CD) + strlen(path);

	strncpy(msg->comn.data, path, strlen(path));

	ret_val = send_data(sock_fd, msg->length, (char *)msg);
	{
		LOG_EXCEPTION("Error sending data to server.", ret_val, EQ, sizeof(MSG_LS), FALSE);
		goto EXIT_LABEL;
	}
	free(msg);

	memset(msg, 0, sizeof(MSG_CD));
	ret_val = receive_data(sock_fd, sizeof(MSG_HDR), msg);
	{
		LOG_EXCEPTION("Error receiving response",ret_val, EQ, 0, FALSE);	
		goto EXIT_LABEL;
	}

	/* Some response was received. */
	ls_result = (char *)malloc((MSG_HDR *)msg->length - sizeof(MSG_HDR));
	{
		LOG_EXCEPTION("Error allocating buffers.",ls_result, NEQ, NULL, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;	
	}

	/* Receive all data from socket anyway, even if we have to discard it. */
	ret_val = receive_data(sock_fd, (MSG_HDR *)msg->length - sizeof(MSG_HDR), ls_result);
	{
		LOG_EXCEPTION("Error receiving response",ret_val, EQ, 0, FALSE);	
		goto EXIT_LABEL;
	}
	comn_msg = ls_result;

	if(!((MSG_HDR *)msg->is_request) && !((MSG_HDR *)msg->response == 1))
	{
		if(!((MSG_HDR *)msg->is_request))
		{
			LOG_EXCEPTION("Malformed reply!", 0, EQ, 1, FALSE);
			ret_val = -1;
			goto EXIT_LABEL;	
		}

		/* Else, error at server. Read error message. */
		fprintf(stdout, "%s\n", (char *)comn_msg);
	}
	/* else cd command was successful */
	
EXIT_LABEL:
	if(msg != NULL)
	{
		free(msg);
		msg = NULL;
	}
	if(ls_result != NULL)
	{
		free(ls_result);
		ls_result = NULL;
	}

	return(ret_val);	
}

int32_t do_chmod(int32_t perm[3], char *path, char **cmd_msg)
{
	int32_t ret_val = 0;
	char *err_msg = NULL;

	MSG_CHMOD *msg = NULL;

	char *ls_result = NULL;
	MSG_COMN *comn_msg = NULL;

	msg = (MSG_LS *)malloc(sizeof(MSG_CHMOD) + strlen(path));
	{
		LOG_EXCEPTION("Error allocating buffers.",msg, NEQ, NULL, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;
	}
	memset(msg, 0, sizeof(MSG_CHMOD) + strlen(path));

	msg->cmd = CHMOD;
	msg->is_request = TRUE;
	msg->response = 0; /* Not applicable. */
	msg->length = sizeof(MSG_CHMOD) + strlen(path);

	msg->perms[0] = perm[0];
	msg->perms[1] = perm[1];
	msg->perms[2] = perm[2];

	strncpy(msg->comn.data, path, strlen(path));

	ret_val = send_data(sock_fd, msg->length, (char *)msg);
	{
		LOG_EXCEPTION("Error sending data to server.", ret_val, EQ, sizeof(MSG_LS), FALSE);
		goto EXIT_LABEL;
	}
	free(msg);

	memset(msg, 0, sizeof(MSG_CD));
	ret_val = receive_data(sock_fd, sizeof(MSG_HDR), msg);
	{
		LOG_EXCEPTION("Error receiving response",ret_val, EQ, 0, FALSE);	
		goto EXIT_LABEL;
	}

	/* Some response was received. */
	ls_result = (char *)malloc((MSG_HDR *)msg->length - sizeof(MSG_HDR));
	{
		LOG_EXCEPTION("Error allocating buffers.",ls_result, NEQ, NULL, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;	
	}

	/* Receive all data from socket anyway, even if we have to discard it. */
	ret_val = receive_data(sock_fd, (MSG_HDR *)msg->length - sizeof(MSG_HDR), ls_result);
	{
		LOG_EXCEPTION("Error receiving response",ret_val, EQ, 0, FALSE);	
		goto EXIT_LABEL;
	}
	/* Align pointer to read error message if any */
	comn_msg = (unsigned char *)ls_result + (sizeof(uint8_t)*3);

	if(!((MSG_HDR *)msg->is_request) && !((MSG_HDR *)msg->response == 1))
	{
		if(!((MSG_HDR *)msg->is_request))
		{
			LOG_EXCEPTION("Malformed reply!", 0, EQ, 1, FALSE);
			ret_val = -1;
			goto EXIT_LABEL;	
		}

		/* Else, error at server. Read error message. */
		fprintf(stdout, "%s\n", (char *)comn_msg);
	}
	/* else chmod command was successful */
	
EXIT_LABEL:
	if(msg != NULL)
	{
		free(msg);
		msg = NULL;
	}
	if(ls_result != NULL)
	{
		free(ls_result);
		ls_result = NULL;
	}

	return(ret_val);	
}