#ifndef __TRANSPORT_IF__
#define __TRANSPORT_IF__

/**
 * @brief      Send request (command) to server or Send response (response/files) to and fro.
 * 
 * This function is invoked when data needs to be sent over the communication channel (socket)
 * to the other end. 
 * 
 * The `Data` can be an exec command request like `ls`, `cd`, `get <file>`, 
 * `put <file>`, `chmod <permissions> <file>` .
 *
 * Or, it can be the actual file contents follwing a `put <file>` request. Thus, when implementing
 * `put <file>`, this method will be called twice as follows:
 * 
 * send_data(dest_fd, strlen("put some_file.txt"), "put some_file.txt");
 * send_data(dest_fd, buffer_length, pointer_to_buffer_where_file_contents_are_written);
 * 
 * On receiving the first command on the other side, the server must receive the file completely
 * and call process_get_or_put(...) so that the file can be saved properly.
 * 
 * The process_get_or_put() will call this function again to send a response (whether the file was
 * saved or not) to the client.
 * 
 * @param[in]  dest_fd  		The destination socket descriptor to which the data must be sent.
 * 								This is relevant for the server only, since it can be handling
 * 								multiple clients and needs to specify whose request is being
 * 								serviced.
 * @param[in]  length   		The length of buffer that  is to be sent.
 * @param[in]  buffer   		The buffer that must be sent on socket.
 *
 * @return     Returns 0 if all data was sent. Else -1 is returned.
 */
int32_t send_data(int32_t dest_fd, int32_t length, unsigned char *buffer);

/**
 * @brief      Receive the response of the command that was sent.
 * 
 * This method is always preceded by at least one call to send_data(). 
 * 
 * So, when the client calls send_data(fd, strlen("ls"), "ls"); it expects to
 * know whether the operation was successful or not. So, the server first
 * sends an explicit response of `0` or `1` for failure or success respectively.
 * Then, based on that result, the client will determine if it must call
 * receive_result() or not (where actual content results of `ls` command would be contained).
 * 
 * @param[in]  response  The response code of whether operation was successful or not.
 *
 * @return     Returns 0 if a response message was successfully received from socket, else -1.
 */
int32_t receive_response(int32_t response);


/**
 * @brief      Receives the output of the command that was executed remotely or file contents.
 *
 * @param[out] length   Address of a length field where transport must write the number of
 * 						bytes that it has read from the socket in the `data` field.
 * @param[out] data  	Pointer to the address where the transport as written the data buffer 
 * 						which contains either the output of command, or contents of the file 
 * 						that is being transferred. 
 *
 * @return    Returns 0 if data was `length` bytes were successfully written into `data`. Else -1.
 */
int32_t receive_data(uint32_t *length, char **data);

/**
 * @brief      Called by transport to allow the code to either save the received file or send the requested file.
 * 
 * On receiving a 'put' request from the client, the transport code would have to receive the entire file in a 
 * buffer which must now be saved to disk. So, it calls:
 * 
 * process_get_or_put(TRUE, file_name)
 * 
 * The code will determine internally if the request is of get or put. If the request is of `get`, the code will
 * first check if the file exists. If it exists, it will call send_data() with a value `1` so that the client
 * can know that the file exists and is coming in the next message.
 * 
 * Then, the code will read the file and call send_data() again with the entire file contents.
 * 
 * Similarly, if the request was a `put`, call process_get_or_put(FALSE, file_name).
 * The code will call into receive_data() expecting that the client
 * will be sending the file in the next message. The transport will receive the file completely and pass
 * its length and buffer pointers in the length and data fields of the function call when it returns.
 * 
 * Then, this function can save the file on server and return successfully.
 *
 * @param[in]  is_put     Indicates if the command requested is `put` or not. If not, then it is `get`.
 * @param      file_name  The file name that is to be `put` or `get`.
 *
 * @return     0 if put/get was successful, else -1.
 */
int32_t process_get_or_put(uint32_t is_put, unsigned char *file_name);

#endif