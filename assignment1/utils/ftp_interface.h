#ifndef ASSIGNMENT_1_FTP_STRS__
#define ASSIGNMENT_1_FTP_STRS__

typedef enum ftp_cmds 
{  
	LS=0,
	CD,
	CHMOD,
	GET,
	PUT,

	__MAX_CMD_MEMBER__
}FTP_CMD;


typedef struct msg_hdr
{
	/**
	 * Message Type 
	 */
	uint32_t cmd;
	/**
	 * Is a request or response. 
	 * Takes one of TRUE/FALSE
	 */
	uint32_t is_request;
	/**
	 * If message is a response, then was response OK or not. 
	 * 0 is false, 1 is true.
	 */
	uint32_t response;
	/**
	 * Length of the entire message (this header included)
	 */
	uint32_t length;
}MSG_HDR;

typedef struct msg_comn
{
	/**
	 * This field server many purposes. But primarily, it is the starting point
	 * reference of any variable data.
	 * 
	 * Deliberately kept as array of length 1 so that we can reference it 
	 * directly as a pointer when using it as start of string.
	 * 
	 * For CD request, it contains the path to cd to. Length is implicit (header->length - sizeof(MSG_HDR)).
	 * For CD response, if the response contains errors, it contains the error message.
	 * 
	 * For ls request, there will be nothing in it.
	 * For ls repsonse, the formatted string representation of ls will be contained.
	 * 
	 * For CHMOD request, it contains name of file to chmod.
	 * For CHMOD response, the structure of MSG_CHMOD is overridded and the perms[3] 
	 * fields are also treated as message buffer in case error was returned.
	 * 
	 * For GET request, it contains name of file to get.
	 * For GET response, if successful, contains contents of file, else error message.
	 * 
	 * For PUT request, first `file_name_len` bytes contain file name of the file being sent. Rest of the buffer is
	 * file contents.
	 * For PUT response, if operation failed, it contains the error message returned by the server.
	 */
	char data[1];
}MSG_COMN;

typedef struct msg_cd
{
	/**
	 * Message Header information.
	 */
	MSG_HDR hdr;

	MSG_COMN comn;
}MSG_CD;

typedef struct msg_ls
{
	/**
	 * Message Header information.
	 */
	MSG_HDR hdr;
	MSG_COMN comn;
}MSG_LS;

typedef struct msg_chmod
{
	/**
	 * Message Header information.
	 */
	MSG_HDR hdr;
	/**
	 * New CHMOD values
	 * For each (owner, group, other), we have a read-write-execute bit sequence.
	 * These are expressed as individual values in sequence:
	 * e.g. chmod 754 is: 
	 * Owner [1: Read 1: Write 1:Execute], Group[1: Read, 0: Write, 1:Execute] 
	 * Others [1: Read, 0: Write, 0:Execute]
	 */	
	uint8_t perms[3];
	MSG_COMN comn;
}MSG_CHMOD;

typedef struct msg_get
{
	/**
	 * Message Header information.
	 */	
	MSG_HDR hdr;
	MSG_COMN comn;
}MSG_GET;


typedef struct msg_put
{
	/**
	 * Message Header information.
	 */	
	MSG_HDR hdr;
	uint32_t file_name_len;
	MSG_COMN comn;
}MSG_PUT;

/**
 * A helper union to allow us to dynamically use the pointer as any of the message types based on what is 
 * the message type in hdr->cmd
 */
typedef union msg_gen
{
	MSG_HDR hdr;
	MSG_CD cd;
	MSG_LS ls;
	MSG_GET get;
	MSG_PUT put;
	MSG_CHMOD chmod;
}MSG_GEN;


#endif