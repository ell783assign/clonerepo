#include "transport_interface.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

int32_t send_data(int32_t dest_fd, int32_t length, unsigned char *buffer)
{
    int32_t n;
    unsigned char *buff = (char *)malloc(length);
    /* fill the buffer with message*/
    buff = buffer;
    /* Send message to the server */
    n = write(sockfd, buffer, strlen(buffer));
    /* return code for whether the data was sent successfully or not */
    return n;
}


int32_t receive_response(int32_t sock_fd, int32_t response[])
{
    int32_t n;
    char *ptr = (char *)malloc(64);
    unsigned char* buff = (char *)malloc(64);
    n = read(sock_fd, buff, 64);
    /* The received buffer contains two integers separated by whitespace, so we separate them */
    ptr = strtok(buff, " ");
    response[0] = (int32_t)atoi((const char *)ptr);
    ptr = strtok(NULL, " ");
    response[1] = (int32_t)atoi((const char *)ptr);
    free(ptr);
    /* return code for whether response was read successfully or not */
    return n;
}

int32_t receive_data(int32_t sock_fd, int32_t length, char **data)
{
    int32_t n;
    n = read(sock_fd, *data, length);
    return n;
}

int32_t process_get_or_put(int32_t sock_fd, uint32_t is_put, unsigned char *file_name)
{
    FILE *fp;
    int32_t size, n;
    unsigned char *buffer = (unsigned char *)malloc(32);
    int32_t resp[2];
    /* determine whether get or put*/
    if(is_put)
    {
        /* search for the file in the working directory */
        fp = fopen(file_name, "rb");
        if(fp==NULL)
        { 
            /* file doesn't exist or is in use somewhere */
            return -1;
        }

        else
        {
            /* get file size */
            fseek(fp, 0, 2);
            size = ftell(fp);
            
            /* reset fp at beginning of file */
            rewind(fp);

            /* tell server that we're performing `put` operation */
            memset(buffer, 0, 32);
            sprintf(buffer, "put %s", filename);
            n = send_data(sock_fd, sizeof(buffer), buffer);
            if(n==0)
            {
                /* check if operation was successful or not */
                n = receive_response(sock_fd, resp);
                if(resp[0])
                {
                    /* write the contents of the file and send it to the socket file descriptor */
                    buffer = (unsigned char*)realloc(buffer, size);
                    memset(buffer, 0, size);
                    fread(buffer, size, 1, fp);
                    n = send_data(sock_fd, size, buffer);
                    /* return information of whether contents were successfully written or not */
                    return n;
                }
                else
                    /* `put` action failed */
                    return -1;
            }
            else
                /* could not send action information to server */
                return n;
        }
    }

    else
    {
        /* send "get filename" to server */
        memset(buffer, 0, 32);
        sprintf(buffer, "get %s", filename);
        n = send_data(sock_fd, sizeof(buffer), buffer);
        if(n==0)
        {
            n = receive_response(sock_fd, resp);
            if(resp[0])
            {
                buffer = (unsigned char *)realloc(buffer, resp[1]);
                memset(buffer, 0, resp[1]);
                /* receive the data sent by the server */
                n = receive_data(sock_fd, resp[1], buffer);
            }

            else
                /* get action was a failure */
                return -1;
        }

        else
            /* couldn't inform server of "get filename" action */
            return n;
            
    }

}

int32_t process_exec(unsigned char *command_string);
 
