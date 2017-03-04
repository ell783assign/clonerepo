#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>

#include <arpa/inet.h>

int32_t init_transport_connections(uint32_t);

#define NUM_QUEUED_CONNECTIONS		5
#define MAX_CONNECTIONS				10

#define TRACE(...) 	fprintf(stderr, "TRACE  \t"__VA_ARGS__)
#define WARN(...) 	fprintf(stderr, "WARNING\t"__VA_ARGS__)
#define ERROR(...)  fprintf(stderr, "ERROR  \t"__VA_ARGS__)

#define TRUE  		(uint32_t)1
#define FALSE 		(uint32_t)0

#define EXIT_ON_ERROR(...)				exit_on_error(__VA_ARGS__, TRUE)
#define LOG_EXCEPTION(...)				exit_on_error(__VA_ARGS__, FALSE)

typedef enum
{
	EQ = 0,
	GT,
	LT,
	GTE,
	LTE,

	__MAX_MEMBER__
} COMPARE_RULE;

/**
 * @brief      Helper function (a macro) to handle error on function returns
 *
 * @param      msg           The message to be printed as diagnostic. NULL if nothing.
 * @param[in]  err_val       The error value that was returned.
 * @param[in]  expected_val  The expected value that should have been returned.
 * @param[in]  has_perror    Indicates if perror message is present for this condition
 * @param[in]  exit_prog	 Indicates if program must terminate on error or just log exception.
 */
static inline void exit_on_error(char *msg, int32_t err_val, COMPARE_RULE rule, int32_t expected_val, uint32_t has_perror, uint32_t exit_prog)
{
	int32_t error_occurred = 1;
	switch(rule)
	{
		case EQ:
			if(err_val != expected_val)
			{
				goto EXIT_LABEL;
			}
			break;
		case GT:
			if(err_val <= expected_val)
			{
				goto EXIT_LABEL;
			}
			break;
		case LT:
			if(err_val >= expected_val)
			{
				goto EXIT_LABEL;
			}
			break;
		case GTE:
			if(err_val < expected_val)
			{
				goto EXIT_LABEL;
			}
			break;	
		case LTE:
			if(err_val > expected_val)
			{
				goto EXIT_LABEL;
			}
			break;
	}
	/* If we reach here, everything went fine. */
	error_occurred = 0;

EXIT_LABEL:	
	if(error_occurred)
	{
		if(exit_prog)
		{
			if(msg)
			{
				ERROR("%s\n", msg);
			}
			ERROR("Received: %d\t Expected: %d\n", err_val, expected_val);
			if(has_perror)
			{
				perror("Diagnostic");
			}
			exit(0);
		}
		else
		{
			if(msg)
			{
				WARN("%s\n", msg);
			}
			WARN("Received: %d\t Expected: %d\n", err_val, expected_val);
			if(has_perror)
			{
				perror("Diagnostic");
			}
			exit(0);
		}
	}

	return;
}/**< exit_on_error */

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
	FD_SET(sock_fd, &fds);

	/* Start endless loop to serve as FTP */
	while(1)
	{
		FD_ZERO(&fds);
		fds = persistent_set;


		/* 0  timeout leads to 100% CPU utilization all the time. */
		timeout_info.tv_sec = 0;
		timeout_info.tv_usec = 5000; /* Wait for 5 milliseconds before timeout */

		num_fd_ready = select(sock_fd+1, &fds, NULL, NULL, &timeout_info);
		if(num_fd_ready<0)
		{
			perror("Exception event on select()");
		}
		else
		{
			if(FD_ISSET(sock_fd, &fds))
			{
				TRACE("Incoming Data on listen socket.");
				next_available_fd_slot = next_available_fd_slot % MAX_CONNECTIONS;
				connection_fds[next_available_fd_slot].sock_fd=accept(	sock_fd, 
																		(struct sockaddr *)&connection_fds[next_available_fd_slot].addr, 
																		&connection_fds[next_available_fd_slot].length);
				LOG_EXCEPTION("Error accepting connection.", connection_fds[next_available_fd_slot].sock_fd, GTE, 0, TRUE);
				FD_SET(connection_fds[next_available_fd_slot].sock_fd, &persistent_set);
				
				next_available_fd_slot++;
			}
			for(ii=0;ii<MAX_CONNECTIONS;ii++)
			{
				if(FD_ISSET(connection_fds[next_available_fd_slot].sock_fd, &fds))
				{
					/* TODO: Read data */
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