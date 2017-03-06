#ifndef __CLI_INTERFACE__
#define __CLI_INTERFACE__

/**
 * @brief      				Invoked by the Command Line Interface when user types `ls`.
 *
 * @param[out]      msg   	This contains a pointer to the string that must be output to the user
 * 							based on whether the operation was successful or not. Developer must 
 * 							first check the value returned by the function if the operation was 
 * 							successful.
 *
 * @return     				Returns 0 if ls command was successful, else -1 is returned.
 * 							Diagnostic string is returned in msg
 */
int32_t do_ls(char **);

/**
 * @brief      				Invoked by the Command Line Interface when user types `cd`.
 *
 * @param[in]		path    Path to cd to.
 * @param[out]      msg   	This contains a pointer to the string that must be output to the user
 * 							based on whether the operation was successful or not. Developer must 
 * 							first check the value returned by the function if the operation was 
 * 							successful.
 *
 * @return     				Returns 0 if `cd` command was successful, else -1 is returned.
 * 							Diagnostic string is returned in msg
 */
int32_t do_cd(char *, char **);

/**
 * @brief      				Invoked by the Command Line Interface when user types `chmod`.
 *
 * @param[in]		perm	This contains the permissions to be set on the file.
 * 							The developer must pass the permissions as an array of 3 integers in
 * 							the following manner:
 * 							perm[0] - Range of value [0-7] - Permissions of owner of file.
 * 							perm[1] - Range of value [0-7] - Permissions of the group of users.
 * 							perm[2] - Range of value [0-7] - Permissions for any other user.
 * @param[in]		f_name	This must contain the name of the file for which permissions are to be
 * 							changed.
 * @param[out]      msg   	This contains a pointer to the string that must be output to the user
 * 							based on whether the operation was successful or not. Developer must 
 * 							first check the value returned by the function if the operation was 
 * 							successful.
 *
 * @return     				Returns 0 if `chmod` command was successful, else -1 is returned.
 * 							Diagnostic string is returned in msg
 */
int32_t do_chmod(int32_t *, char *, char **);

/**
 * @brief      				Invoked by the Command Line Interface when user types `lcd`.
 *
 * @param[in]		path	Path to CD to.
 * @param[out]      msg   	This contains a pointer to the string that must be output to the user
 * 							based on whether the operation was successful or not. Developer must 
 * 							first check the value returned by the function if the operation was 
 * 							successful.
 *
 * @return     				Returns 0 if `lcd` command was successful, else -1 is returned.
 * 							Diagnostic string is returned in msg
 */
int32_t do_lcd(char *, char **);

/**
 * @brief      				Invoked by the Command Line Interface when user types `lls`.
 *
 * @param[out]      msg   	This contains a pointer to the string that must be output to the user
 * 							based on whether the operation was successful or not. Developer must 
 * 							first check the value returned by the function if the operation was 
 * 							successful.
 *
 * @return     				Returns 0 if `lls` command was successful, else -1 is returned.
 * 							Diagnostic string is returned in msg
 */
int32_t do_lls(char **);

/**
 * @brief      				Invoked by the Command Line Interface when user types `lchmod`.
 *
 * @param[in]		perm	This contains the permissions to be set on the file.
 * 							The developer must pass the permissions as an array of 3 integers in
 * 							the following manner:
 * 							perm[0] - Range of value [0-7] - Permissions of owner of file.
 * 							perm[1] - Range of value [0-7] - Permissions of the group of users.
 * 							perm[2] - Range of value [0-7] - Permissions for any other user.
 * @param[in]		f_name	This must contain the name of the file for which permissions are to be
 * 							changed.
 * @param[out]      msg   	This contains a pointer to the string that must be output to the user
 * 							based on whether the operation was successful or not. Developer must 
 * 							first check the value returned by the function if the operation was 
 * 							successful.
 *
 * @return     				Returns 0 if `lchmod` command was successful, else -1 is returned.
 * 							Diagnostic string is returned in msg
 */
int32_t do_lchmod(int32_t * , char *, char **);

/**
 * @brief      				Invoked by Command Line Interface when user types `get`.
 *
 * @param      f_name  		The file name that must be fetched from the server.
 * @param      msg     		Diagnostic message for the user.
 *
 * @return     				Returns 0 if `get` was successful, else, -1 is returned.
 * 							Diagnostic string is returned in msg. 				
 */
int32_t do_get(char *, char **);

/**
 * @brief      				Invoked by Command Line Interface when user types `put`.
 *
 * @param      f_name  		The file name that must be sent to the server.
 * @param      msg     		Diagnostic message for the user.
 *
 * @return     				Returns 0 if `put` was successful, else, -1 is returned.
 * 							Diagnostic string is returned in msg. 				
 */
int32_t do_put(char *, char **);

/**
 * @brief      				Invoked by Command Line Interface to close the daemon.
 *
 * @param      sock_fd  	Socket Descriptor that is open.
 *
 * @return     				Nothing.
 */
void do_close(int32_t);

#define MAXCMDLENGTH 512

extern char cmdargs[3][MAXCMDLENGTH];

int processthiscmd(char *);
int cmdnum(char *);
void printdiagnosticmsg(char **);
#endif