#include <utils.h>

#include <cli_interface.h>

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
	
	char *msg, userinput[MAXCMDLENGTH]={0};

	int32_t length;

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

		memset(userinput, 0, sizeof(userinput));
		memset(cmdargs, 0, sizeof(cmdargs));
		if(alt_gets(userinput)!=0)
		{
			EXIT_ON_ERROR("Could not parse input.", 0, NEQ, 0, FALSE);
		}
		wordsfound=processthiscmd(userinput);
		if(wordsfound==0)
		{
			TRACE("No command entered.\n");
			continue;
		}
		result=99;
		TRACE("%d\n",cmdnum(cmdargs[0]));
		switch(cmdnum(cmdargs[0]))
		{	
			case 1:	
				if(wordsfound==1)
				{
					result=do_ls(&length, &msg);
					if(!result)
					{
						fprintf(stdout, "%s", msg);
					}
				}
				break;
			case 2:	
				if(wordsfound==2)
				{
					result=do_cd(cmdargs[1],&length, &msg);
				}
				break;
			case 3:	
				if(wordsfound==3)
				{	permission=atoi(cmdargs[1]);
					if(strlen(cmdargs[1])==3 && permission>=0 && permission<=777)
					{	perm[0]=permission/100;
						perm[2]=permission%10;
						perm[1]=(permission-100*perm[0]-perm[2])/10;
						result=do_chmod(perm, cmdargs[2], &length, &msg);
					}
				}
				break;
			case 4:	
				if(wordsfound==1)
				{
					result=do_lls(&length, &msg);
					fprintf(stdout, "%s", msg);
				}
				break;
			case 5:	
				if(wordsfound==2)
				{
					result=do_lcd(cmdargs[1], &length, &msg);
				}
				break;
			case 6:	
				if(wordsfound==3)
				{	permission=atoi(cmdargs[1]);
					if(strlen(cmdargs[1])==3 && permission>=0 && permission<=777)
					{	perm[0]=permission/100;
						perm[2]=permission%10;
						perm[1]=(permission-100*perm[0]-perm[2])/10;
						result=do_lchmod(perm,cmdargs[2], &length, &msg);
					}
				}
				break;
			case 7:	
				if(wordsfound==2)
				{
					result=do_put(cmdargs[1],&length, &msg);
				}
				break;
			case 8:	
				if(wordsfound==2)
				{
					result=do_get(cmdargs[1],&length, &msg);
				}
				break;
			case 9:	
				if(wordsfound==1)
				{
					do_close(sock_fd);
				}
				goto EXIT_LABEL;

			default:	
				{
					result=0;
					printf("Command not present\n");
				}
		}
		if(result==-1)
			printdiagnosticmsg(&msg);
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


void do_close(int32_t fd)
{
	if(fd > 0)
	{
		close(fd);
	}
	return;
}

int32_t do_ls(uint32_t *length, char **cmd_msg)
{
	int32_t ret_val = 0;
	char *err_msg = NULL;

	MSG_LS *msg = NULL;

	char *ls_result = NULL;
	MSG_COMN *comn_msg = NULL;

	msg = (MSG_LS *)malloc(sizeof(MSG_LS));
	if(!msg)
	{
		LOG_EXCEPTION("Error allocating buffers.",0, NEQ, 0, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;
	}
	memset(msg, 0, sizeof(MSG_LS));

	msg->hdr.cmd = LS;
	msg->hdr.is_request = TRUE;
	msg->hdr.response = 0; /* Not applicable. */
	msg->hdr.length = sizeof(MSG_LS);

	ret_val = send_data(sock_fd, sizeof(MSG_LS), (char *)msg);
	if(ret_val!=sizeof(MSG_LS))
	{
		LOG_EXCEPTION("Error sending data to server.", ret_val, EQ, sizeof(MSG_LS), FALSE);
		goto EXIT_LABEL;
	}

	memset(msg, 0, sizeof(MSG_LS));
	ret_val = receive_data(sock_fd, sizeof(MSG_HDR), (char *)msg);
	if(ret_val!=sizeof(MSG_HDR))
	{
		LOG_EXCEPTION("Error receiving response",ret_val, EQ, 0, FALSE);	
		goto EXIT_LABEL;
	}

	/* Some response was received. */
	ls_result = (char *)malloc(msg->hdr.length - sizeof(MSG_HDR));
	if(!ls_result)
	{
		LOG_EXCEPTION("Error allocating buffers.",0, NEQ, 0, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;	
	}

	/* Receive all data from socket anyway, even if we have to discard it. */
	ret_val = receive_data(sock_fd, msg->hdr.length - sizeof(MSG_HDR), (char *)ls_result);
	if(ret_val!=msg->hdr.length - sizeof(MSG_HDR))
	{
		LOG_EXCEPTION("Error receiving response",ret_val, EQ, msg->hdr.length - sizeof(MSG_HDR), FALSE);	
		goto EXIT_LABEL;
	}
	comn_msg = (MSG_COMN *)ls_result;

	if(!msg->hdr.is_request && !(msg->hdr.response == 1))
	{
		if(!msg->hdr.is_request)
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
		*cmd_msg = NULL;
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

int32_t do_cd(char *path, uint32_t *length, char **cmd_msg)
{
	TRACE("%s\n", path);
	int32_t ret_val = 0;
	char *err_msg = NULL;

	MSG_CD *msg = NULL;

	char *ls_result = NULL;
	MSG_COMN *comn_msg = NULL;

	msg = (MSG_CD *)malloc(sizeof(MSG_CD) + strlen(path));
	if(!msg)
	{
		LOG_EXCEPTION("Error allocating buffers.",0, NEQ, 0, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;
	}
	memset(msg, 0, sizeof(MSG_LS)+ strlen(path));

	msg->hdr.cmd = CD;
	msg->hdr.is_request = TRUE;
	msg->hdr.response = 0; /* Not applicable. */
	msg->hdr.length = sizeof(MSG_CD) + strlen(path);

	strncpy(msg->comn.data, path, strlen(path));

	ret_val = send_data(sock_fd, msg->hdr.length, (char *)msg);
	if(ret_val!=msg->hdr.length)
	{
		LOG_EXCEPTION("Error sending data to server.", ret_val, EQ, msg->hdr.length , FALSE);
		goto EXIT_LABEL;
	}

	memset(msg, 0, sizeof(MSG_CD));
	ret_val = receive_data(sock_fd, sizeof(MSG_HDR), (char *)msg);
	if(ret_val!=sizeof(MSG_HDR))
	{
		LOG_EXCEPTION("Error receiving response",ret_val, EQ, sizeof(MSG_HDR), FALSE);	
		goto EXIT_LABEL;
	}

	/* Some response was received. */
	if(msg->hdr.length - sizeof(MSG_HDR) >0)
	{
		ls_result = (char *)malloc(msg->hdr.length - sizeof(MSG_HDR));
		if(!ls_result)
		{
			LOG_EXCEPTION("Error allocating buffers.",0, NEQ, 0, FALSE);
			ret_val = -1;
			goto EXIT_LABEL;	
		}

		/* Receive all data from socket anyway, even if we have to discard it. */
		ret_val = receive_data(sock_fd, msg->hdr.length - sizeof(MSG_HDR), (char *)ls_result);
		if(ret_val!=msg->hdr.length - sizeof(MSG_HDR))
		{
			LOG_EXCEPTION("Error receiving response",ret_val, EQ, msg->hdr.length - sizeof(MSG_HDR), FALSE);	
			goto EXIT_LABEL;
		}
		comn_msg = (MSG_COMN*)ls_result;
	}
	if(!msg->hdr.is_request && !(msg->hdr.response == 1))
	{
		if(!msg->hdr.is_request)
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

int32_t do_chmod(int32_t perm[3], char *path, uint32_t *length, char **cmd_msg)
{
	int32_t ret_val = 0;
	char *err_msg = NULL;

	MSG_CHMOD *msg = NULL;

	char *ls_result = NULL;
	MSG_COMN *comn_msg = NULL;

	msg = (MSG_CHMOD *)malloc(sizeof(MSG_CHMOD) + strlen(path));
	if(!msg)
	{
		LOG_EXCEPTION("Error allocating buffers.",0, NEQ, 0, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;
	}
	memset(msg, 0, sizeof(MSG_CHMOD) + strlen(path));

	msg->hdr.cmd = CHMOD;
	msg->hdr.is_request = TRUE;
	msg->hdr.response = 0; /* Not applicable. */
	msg->hdr.length = sizeof(MSG_CHMOD) + strlen(path);

	msg->perms[0] = (uint8_t)perm[0];
	msg->perms[1] = (uint8_t)perm[1];
	msg->perms[2] = (uint8_t)perm[2];

	strncpy(msg->comn.data, path, strlen(path));

	ret_val = send_data(sock_fd, msg->hdr.length, (char *)msg);
	if(ret_val!=msg->hdr.length)
	{
		LOG_EXCEPTION("Error sending data to server.", ret_val, EQ, msg->hdr.length, FALSE);
		goto EXIT_LABEL;
	}

	memset(msg, 0, sizeof(MSG_CD));
	ret_val = receive_data(sock_fd, sizeof(MSG_HDR),(char *) msg);
	if(ret_val!=sizeof(MSG_HDR))
	{
		LOG_EXCEPTION("Error receiving response",ret_val, EQ, sizeof(MSG_HDR), FALSE);	
		goto EXIT_LABEL;
	}

	/* Some response was received. */
	ls_result = (char *)malloc(msg->hdr.length - sizeof(MSG_HDR));
	if(!ls_result)
	{
		LOG_EXCEPTION("Error allocating buffers.",0, NEQ, 0, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;	
	}

	/* Receive all data from socket anyway, even if we have to discard it. */
	ret_val = receive_data(sock_fd, msg->hdr.length - sizeof(MSG_HDR), (char *)ls_result);
	if(ret_val!=msg->hdr.length - sizeof(MSG_HDR))
	{
		LOG_EXCEPTION("Error receiving response",ret_val, EQ, msg->hdr.length - sizeof(MSG_HDR), FALSE);	
		goto EXIT_LABEL;
	}
	/* Align pointer to read error message if any */
	comn_msg = (MSG_COMN*)((unsigned char *)ls_result + (sizeof(uint8_t)*3));

	if(!(msg->hdr.is_request) && !(msg->hdr.response == 1))
	{
		if(!msg->hdr.is_request)
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

int32_t do_put(char *file_name, uint32_t *length, char **err_msg_ptr)
{
	int32_t ret_val = 0;

	char *buf=NULL;
	int32_t len;

	MSG_PUT *msg = NULL;

	char *ls_result = NULL;
	MSG_COMN *comn_msg = NULL;

	ret_val = read_file_to_buffer(file_name, &buf, &len);
	if(ret_val!=0)
	{
		LOG_EXCEPTION("Could not read file.", ret_val, EQ, 0, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;
	}

	msg = (MSG_PUT *)malloc(sizeof(MSG_PUT)+ strlen(file_name) + len);
	if(!msg)
	{
		LOG_EXCEPTION("Error allocating transport buffer.", 0, NEQ, 0, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;
	}

	msg->hdr.cmd = PUT;
	msg->hdr.is_request = TRUE;
	msg->hdr.response = 0;
	msg->hdr.length = sizeof(MSG_PUT)+ strlen(file_name) + len;

	msg->file_name_len = strlen(file_name);
	snprintf(msg->comn.data, strlen(file_name)+1, "%s", file_name);

	memcpy((char *)msg->comn.data + strlen(file_name)+1, msg, len);

	ret_val = send_data(sock_fd, msg->hdr.length, (char *)msg);
	if(ret_val!=msg->hdr.length)
	{
		LOG_EXCEPTION("Error sending data to server.", ret_val, EQ, sizeof(MSG_LS), FALSE);
		goto EXIT_LABEL;
	}

	memset(msg, 0, sizeof(MSG_PUT));
	ret_val = receive_data(sock_fd, sizeof(MSG_HDR), (char *)msg);
	if(ret_val!=sizeof(MSG_HDR))
	{
		LOG_EXCEPTION("Error receiving response",ret_val, EQ, sizeof(MSG_HDR), FALSE);	
		goto EXIT_LABEL;
	}

	/* Some response was received. */
	ls_result = (char *)malloc(msg->hdr.length - sizeof(MSG_HDR));
	if(!ls_result)
	{
		LOG_EXCEPTION("Error allocating buffers.",0, NEQ, 0, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;	
	}

	/* Receive all data from socket anyway, even if we have to discard it. */
	ret_val = receive_data(sock_fd, msg->hdr.length - sizeof(MSG_HDR), (char *)ls_result);
	if(ret_val!=msg->hdr.length - sizeof(MSG_HDR))
	{
		LOG_EXCEPTION("Error receiving response",ret_val, EQ, msg->hdr.length - sizeof(MSG_HDR), FALSE);	
		goto EXIT_LABEL;
	}
	/* Align pointer to read error message if any */
	comn_msg = (MSG_COMN*)((unsigned char *)ls_result + (sizeof(uint8_t)*3));

	if(!(msg->hdr.is_request) && !(msg->hdr.response == 1))
	{
		if(!msg->hdr.is_request)
		{
			LOG_EXCEPTION("Malformed reply!", 0, EQ, 1, FALSE);
			ret_val = -1;
			goto EXIT_LABEL;	
		}

		/* Else, error at server. Read error message. */
		fprintf(stdout, "%s\n", (char *)comn_msg);
	}
	/* else put command was successful */
	


EXIT_LABEL:	
	return ret_val;
}

int32_t do_get(char *file_name, uint32_t *length, char **err_msg_ptr)
{
	int32_t ret_val = 0;

	char *buf=NULL;
	int32_t len;

	MSG_GET *msg = NULL;

	char *ls_result = NULL;
	MSG_COMN *comn_msg = NULL;

	msg = (MSG_GET *)malloc(sizeof(MSG_GET)+ strlen(file_name)+1);
	if(!msg)
	{
		LOG_EXCEPTION("Error allocating transport buffer.", 0, NEQ, 0, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;
	}

	msg->hdr.cmd = GET;
	msg->hdr.is_request = TRUE;
	msg->hdr.response = 0;
	msg->hdr.length = sizeof(MSG_GET)+ strlen(file_name);

	snprintf(msg->comn.data, strlen(file_name)+1, "%s", file_name);

	ret_val = send_data(sock_fd, msg->hdr.length, (char *)msg);
	if(ret_val != msg->hdr.length)
	{
		LOG_EXCEPTION("Error sending data to server.", ret_val, EQ, msg->hdr.length, FALSE);
		goto EXIT_LABEL;
	}

	memset(msg, 0, sizeof(MSG_GET));
	ret_val = receive_data(sock_fd, sizeof(MSG_HDR), (char *)msg);
	if(ret_val != sizeof(MSG_HDR))
	{
		LOG_EXCEPTION("Error receiving response",ret_val, EQ, sizeof(MSG_HDR), FALSE);	
		goto EXIT_LABEL;
	}

	/* Some response was received. */
	ls_result = (char *)malloc(msg->hdr.length - sizeof(MSG_HDR));
	if(!ls_result)
	{
		LOG_EXCEPTION("Error allocating buffers.",0, NEQ, 0, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;	
	}

	/* Receive all data from socket anyway, even if we have to discard it. */
	ret_val = receive_data(sock_fd, msg->hdr.length - sizeof(MSG_HDR), (char *)ls_result);
	if(ret_val!=msg->hdr.length - sizeof(MSG_HDR))
	{
		LOG_EXCEPTION("Error receiving response",ret_val, EQ, msg->hdr.length - sizeof(MSG_HDR), FALSE);	
		goto EXIT_LABEL;
	}
	/* Align pointer to read error message if any */
	comn_msg = (MSG_COMN*)((unsigned char *)ls_result);

	if(!(msg->hdr.is_request) && !(msg->hdr.response == 1))
	{
		if(!msg->hdr.is_request)
		{
			LOG_EXCEPTION("Malformed reply!", 0, EQ, 1, FALSE);
			ret_val = -1;
			goto EXIT_LABEL;	
		}

		/* Else, error at server. Read error message. */
		fprintf(stdout, "%s\n", (char *)comn_msg);
	}
	/* else get command was successful. Save into file */
	/* Don't need to send back the file name */
	ret_val = write_file_to_disk(file_name, (char *)comn_msg, msg->hdr.length - sizeof(MSG_HDR));
	if(ret_val!=0)
	{
		LOG_EXCEPTION("Error writing to disk",ret_val, EQ, 0, FALSE);	
		goto EXIT_LABEL;
	}
EXIT_LABEL:
	return(ret_val);	
}