#include <utils.h>
#include <cli_interface.h>
int32_t init_transport_connections(uint32_t);

int32_t handle_incoming_data(int32_t );
int32_t handle_put(char *, char *, uint32_t *, char **);
int32_t handle_get(char *, uint32_t *, char **);

#define NUM_QUEUED_CONNECTIONS		5
#define MAX_CONNECTIONS				10


/**
 * A single connection instance
 */
typedef struct connection_cb
{
	/**
	 * Socket Descriptor.
	 */
	int32_t sock_fd;

	/**
	 * Incoming Address of source.
	 */
	struct sockaddr_in addr;

	/**
	 * Address length
	 */
	 socklen_t length;
}CONNECTION_CB;

/**
 * @brief      Program to open a UDP listen socket for FTP server.
 *
 * @param[in]  argc  The argc
 * @param      argv  The argv
 *
 * @return     Nothing Useful
 */
int32_t main(int32_t argc, char *argv[])
{
	int32_t ret_val;

	uint32_t bind_port = 5000; /**< Default value */

	int32_t sock_fd;	/**< Listen Socket FD */
	struct sockaddr_in server_address; /**< Server Listen Address */

	CONNECTION_CB connection_fds[MAX_CONNECTIONS]; /**< MAP of open incoming sessions */
	int32_t next_available_fd_slot = 0;
	int32_t ii;

	fd_set persistent_set, fds;
	struct timeval timeout_info;
	int32_t num_fd_ready;
	int32_t max_fd;

	if(argc == 2)
	{
		bind_port = atoi(argv[1]);
	}

	for(ii=0;ii<MAX_CONNECTIONS;ii++)
	{
		connection_fds[ii].sock_fd=-1;
	}

	sock_fd = init_transport_connections(bind_port);
	EXIT_ON_ERROR(NULL, sock_fd, GT, 0, FALSE);

	FD_ZERO(&persistent_set);
	FD_SET(sock_fd, &persistent_set);

	max_fd = sock_fd;

	/* Start endless loop to serve as FTP */
	while(1)
	{
		FD_ZERO(&fds);
		fds = persistent_set;


		/* 0  timeout leads to 100% CPU utilization all the time. */
		timeout_info.tv_sec = 0;
		timeout_info.tv_usec = 5000; /* Wait for 5 milliseconds before timeout */

		num_fd_ready = select(max_fd+1, &fds, NULL, NULL, &timeout_info);
		if(num_fd_ready<0)
		{
			perror("Exception event on select()");
		}
		else
		{
			if(FD_ISSET(sock_fd, &fds))
			{
				TRACE("Incoming connection request.\n");
				next_available_fd_slot = next_available_fd_slot % MAX_CONNECTIONS;
				connection_fds[next_available_fd_slot].sock_fd=accept(	sock_fd, 
																		(struct sockaddr *)&connection_fds[next_available_fd_slot].addr, 
																		&connection_fds[next_available_fd_slot].length);
				LOG_EXCEPTION("Error accepting connection.", connection_fds[next_available_fd_slot].sock_fd, GTE, 0, TRUE);
				FD_SET(connection_fds[next_available_fd_slot].sock_fd, &persistent_set);

				TRACE("New connection accepted on descriptor: %d\n",connection_fds[next_available_fd_slot].sock_fd);
				next_available_fd_slot++;
				if(max_fd<connection_fds[next_available_fd_slot].sock_fd)
				{
					max_fd = connection_fds[next_available_fd_slot].sock_fd;
				}
			}
			for(ii=0;ii<MAX_CONNECTIONS;ii++)
			{
				if(FD_ISSET(connection_fds[next_available_fd_slot].sock_fd, &fds))
				{
					/* TODO: Read data */
					TRACE("Data on connection socket.\n");
					ret_val = handle_incoming_data(connection_fds[next_available_fd_slot].sock_fd);
				}
			}
		}
	}

	return(0);
}


/**
 * @brief      Opens network connection for listening to incoming requests.
 *
 * @param[in]  bind_port  The bind port
 *
 * @return     Descriptor of listen socket if successful, else -1.
 */
int32_t init_transport_connections(uint32_t bind_port)
{
	int32_t ret_val;

	int32_t sock_fd;	/**< Listen Socket FD */
	struct sockaddr_in server_address; /**< Server Listen Address */

	TRACE("Opening UDP Socket\n");
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	EXIT_ON_ERROR("Could not open stream socket.", sock_fd, GTE, 0, TRUE);


	/* Setup address to bind to */
	memset(&server_address, 0, sizeof(struct sockaddr));
	inet_pton(AF_INET, (const char *)"0.0.0.0", (void *) &server_address.sin_addr.s_addr);
	server_address.sin_port = htons(bind_port);
	server_address.sin_family = AF_INET;

	TRACE("Binding to Port %d\n", bind_port);
	ret_val = bind(sock_fd, (struct sockaddr *)&server_address, sizeof(struct sockaddr));
	EXIT_ON_ERROR("Failed to bind to server address.", ret_val, EQ, 0, TRUE);

	ret_val = listen(sock_fd, NUM_QUEUED_CONNECTIONS);
	EXIT_ON_ERROR("Failed to listen on interface.", ret_val, EQ, 0, TRUE);
	TRACE("Listening...\n");

	return(sock_fd);
}

int32_t handle_incoming_data(int32_t sock_fd)
{
	int32_t ret_val = 0;

	int32_t num_reads = 0;

	char *msg;
	char *request; 
	char *filename;
	uint32_t length;
	uint32_t response_length = sizeof(MSG_HDR);

	int32_t perm[3];

	MSG_GEN *gen=NULL;


	MSG_HDR *hdr = (MSG_HDR *)malloc(sizeof(MSG_HDR));
	if(!hdr)
	{
		LOG_EXCEPTION("Error allocating buffer for message header.", 0, NEQ, 0, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;
	}

	ret_val = receive_data(sock_fd, sizeof(MSG_HDR), (char *)hdr);
	if(ret_val != 0)
	{
		LOG_EXCEPTION("Error allocating buffer for message header.", ret_val, EQ, 0, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;	
	}

	/* receive the rest of data too. */
	gen = (MSG_GEN *)malloc(sizeof(char)*hdr->length);
	if(!gen)
	{
		LOG_EXCEPTION("Error allocating buffer for message body.", 0, NEQ, 0, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;
	}	

	/* Align pointer to receive rest of data */
	request = (char *)((char *)gen + sizeof(MSG_HDR));
	ret_val = receive_data(sock_fd, hdr->length - sizeof(MSG_HDR), (char *)request);
	if(ret_val != 0)
	{
		LOG_EXCEPTION("Error allocating buffer for message header.", ret_val, EQ, 0, FALSE);
		ret_val = -1;
		goto EXIT_LABEL;	
	}

	switch(hdr->cmd)
	{
		case LS:
			ret_val = do_lls(&length, &msg);
			if(ret_val != 0)
			{
				response_length += length;
			}
			break;
		case CD:
			ret_val = do_lcd(request, &length, &msg);
			if(ret_val != 0)
			{
				response_length += length;
			}
			break;
		case CHMOD:

			perm[0] = 0 | gen->chmod.perms[0];
			perm[1] = 0 | gen->chmod.perms[1];
			perm[2] = 0 | gen->chmod.perms[2];

			request = (char *)(gen->chmod.comn.data);
			ret_val = do_lchmod(perm, request, &length, &msg);
			if(ret_val != 0)
			{
				response_length += length+(sizeof(uint8_t)*3);
			}
			break;

		case GET:
			ret_val = handle_get(request, &length, &msg);
			response_length += length;
			break;

		case PUT:
			filename = (char *)malloc(sizeof(char) * gen->put.file_name_len);
			if(!filename)
			{
				LOG_EXCEPTION("Error allocating buffer for file name.", 0, NEQ, 0, FALSE);
				ret_val = -1;
				goto EXIT_LABEL;
			}
			strncpy(filename, gen->put.comn.data, gen->put.file_name_len);
			request = 	(char *)(gen->put.comn.data + gen->put.file_name_len);			
			ret_val = handle_put(filename, request, &length, &msg);
			if(ret_val != 0)
			{
				response_length += length;
			}
			break;
		default:
			LOG_EXCEPTION("Unknown command!", 0, NEQ, 0, FALSE);
	}
	free(msg);

	

EXIT_LABEL:
	return(ret_val);
}

int32_t handle_get(char *filename, uint32_t *length, char **msg)
{
	int32_t ret_val = 0;


EXIT_LABEL:
	return(ret_val);	
}

int32_t handle_put(char *filename, char *contents, uint32_t *length, char **msg)
{
	int32_t ret_val = 0;


EXIT_LABEL:
	return(ret_val);	
}