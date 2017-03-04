#include <utils.h>

int32_t connect_to_server(uint32_t, char *);

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

	int32_t sock_fd;	/**< Connect Socket FD */
	char *server_address=NULL; /**< Server Listen Address */

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

	while(1)
	{
		/* TODO: Add code here.*/
	}
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