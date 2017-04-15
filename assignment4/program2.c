/*
 * program2.c
 *
 *  Created on: 09-Apr-2017
 *      Author: craft
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <errno.h>
#include <string.h>

/****************************************************************************************/
/* Custom defines section.															    */
/****************************************************************************************/
#ifdef BUILD_DEBUG
#define TRACE(...) 	fprintf(stderr, "\nTRACE  \t%10s\t%3d\t", __func__, __LINE__);fprintf(stderr, __VA_ARGS__)
#define WARN(...) 	fprintf(stderr, "\nWARN  \t%10s\t%3d\t", __func__, __LINE__);fprintf(stderr, __VA_ARGS__)
#define ERROR(...)  fprintf(stderr, "\nERROR  \t%10s\t%3d\t", __func__, __LINE__);fprintf(stderr, __VA_ARGS__)
#define ENTRY()		fprintf(stderr, "\nTRACE \t%10s\t%3d Enter {",__func__, __LINE__);
#define EXIT()		fprintf(stderr, "\nTRACE \t%10s\t%3d Exit }",__func__, __LINE__);
#else
#define TRACE(...)
#define WARN(...)
#define ERROR(...)
#define ENTRY()
#define EXIT()
#endif

#define CONSOLE(...) 	fprintf(stderr,__VA_ARGS__)
#define TRUE  		(uint32_t)1
#define FALSE 		(uint32_t)0


#define simsh_TOK_BUFSIZE 64
#define simsh_TOK_DELIM " \t\r\n\a"
#define simsh_MAX_PATH_LEN 1024
#define simsh_RL_BUFSIZE 1024
#define MAX_CWD_LENGTH 1000 
/****************************************************************************************/

typedef struct circular_linked_list
{
	void *self;
	struct circular_linked_list *next;
	struct circular_linked_list *prev;
}CLL;

struct path_string
{	char *my_path;
	struct path_string *next;
};

struct my_stack
{	struct path_string *head;
};

struct my_stack path_stack;

#define INIT_CLL_ROOT(LIST)					\
	(LIST).self = NULL;						\
	(LIST).prev = &(LIST);						\
	(LIST).next = &(LIST);

#define INIT_CLL_NODE(NODE,SELF)			\
	(NODE).self = (SELF);					\
	(NODE).next = NULL;					\
	(NODE).prev = NULL;

//Read as Insert A after B
#define INSERT_AFTER(A,B)					\
	(A).next = (B).next;						\
	(A).next->prev = &(A);						\
	(B).next = &(A);							\
	(A).prev = &(B);

//Read as Insert A before B
#define INSERT_BEFORE(A,B)					\
	(A).next = &(B);							\
	(B).prev->next = &(A);						\
	(A).prev = (B).prev;						\
	(B).prev = &(A);

#define REMOVE_FROM_LIST(A)					\
	(A).next->prev = (A).prev;					\
	(A).prev->next = (A).next;					\
	(A).prev = NULL;							\
	(A).next = NULL;

#define NEXT_IN_LIST(NODE)						\
	(NODE).next->self

#define PREV_IN_LIST(NODE)						\
	(NODE).prev->self

typedef struct path_list
{
	CLL node;
	char *path;
}PATH_LIST;

/****************************************************************************************/

char *simsh_linereader(void);
char **simsh_extract(char *);
int simsh_execute(char **);
int simsh_runsys(char **);
int simsh_islocal(char *);
int simsh_runlocal(char **);



/*
  Function Declarations for builtin shell commands:
 */
int simsh_cd(char **args);
int simsh_help(char **args);
int simsh_exit(char **args);
int simsh_pushd(char **args);
int simsh_popd(char **args);
int simsh_path(char **args);
int simsh_dirs(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "pushd",
  "popd",
  "path",
  "dirs"
};

int (*builtin_func[]) (char **) = {
  &simsh_cd,
  &simsh_help,
  &simsh_exit,
  &simsh_pushd,
  &simsh_popd,
  &simsh_path,
  &simsh_dirs
};

int simsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}


CLL path;


static char *print_path()
{
	static uint32_t multiplier = 1;
	char *pp_path = (char *)malloc(sizeof(char) * simsh_MAX_PATH_LEN*multiplier);
	PATH_LIST *path_elem = NULL;
	uint32_t path_len = 1; /* Always have space for a NULL char */

	if(!pp_path)
	{
		ERROR("Error allocating memory!");
	}
	else
	{
		memset(pp_path, 0, simsh_MAX_PATH_LEN);
		path_elem = (PATH_LIST *)NEXT_IN_LIST(path);
		if(path_elem!=NULL)
		{
			if(path_len+strlen(path_elem->path)> simsh_MAX_PATH_LEN)
			{
				multiplier++;
				pp_path = (char *)realloc(pp_path, sizeof(char) * simsh_MAX_PATH_LEN*multiplier);
				if(!pp_path)
				{
					ERROR("Error growing buffer");
					goto EXIT_LABEL;
				}
				path_len += strlen(path_elem->path);
			}
			strncat(pp_path, path_elem->path, strlen(path_elem->path));
		}
		while( (path_elem != NULL) && 
				(path_elem = (PATH_LIST *)NEXT_IN_LIST(path_elem->node)) != NULL)
		{
			if(path_len+strlen(path_elem->path)+1> simsh_MAX_PATH_LEN) /* +1 for colon */
			{
				multiplier++;
				pp_path = (char *)realloc(pp_path, sizeof(char) * simsh_MAX_PATH_LEN*multiplier);
				if(!pp_path)
				{
					ERROR("Error growing buffer");
					goto EXIT_LABEL;
				}
				path_len += strlen(path_elem->path);
			}

			strncat(pp_path, ":", strlen(":"));
			strncat(pp_path, path_elem->path, strlen(path_elem->path));
		}
	}

EXIT_LABEL:
	return(pp_path);
}

/**
 * Execute command
 */
static int32_t execute(char *cmd_path, char *cmd, char *params[])
{
	int32_t ret_val = 0;
	pid_t pid;
	int result;

	char *pp_path = print_path();
	if(pp_path==NULL)
	{
		ERROR("Error fetching PATH variable.");
		goto EXIT_LABEL;
	}

	char *path_var = (char *)malloc(sizeof(char) * (strlen(pp_path)+6));
	if(!path_var)
	{
		ERROR("Error allocating memory.");
		goto EXIT_LABEL;
	}
	memset(path_var, 0, sizeof(char) * (strlen(pp_path)+6));

	snprintf(path_var, strlen(pp_path)+6, "PATH=%s", pp_path);

	char *child_env[] = {
		path_var,
		0
	};

	pid = fork();
	if (pid == 0) 
	{
		// Child process
		if (execve(cmd_path, &params[0], child_env) == -1) 
		{
			perror("simsh");
		}

		exit(EXIT_FAILURE);
	} 
	else if (pid < 0) 
	{
		// Error forking
		perror("simsh");
	} 
	else 
	{
		// Parent process
		do 
		{
			waitpid(pid, &result, WUNTRACED);
		} while (!WIFEXITED(result) && !WIFSIGNALED(result));
	}

EXIT_LABEL:
	return(ret_val);
}


/**
 * @brief Try to execute command
 *
 * When executing a command, the shell should first look for the command in the current directory,
 * and if not found, search the directories defined in a special variable, path.
 */
static int32_t try_execute(char *cmd, char *params[])
{
	int32_t ret_val = 0;

	PATH_LIST *path_node = NULL;

	char path_var[1024] = {0};

	ret_val = access(cmd, F_OK);
	if(ret_val == -1)
	{
		if(errno != ENOENT)
		{
			ERROR("Some error occurred!");
			ret_val = -1;
			goto EXIT_LABEL;
		}
		TRACE("Command does not exist in CWD");

		/* Look in path variables. */
		for(path_node = (PATH_LIST *)NEXT_IN_LIST(path);
			path_node != NULL;
			path_node = (PATH_LIST *)NEXT_IN_LIST(path_node->node))
		{
			snprintf(path_var, sizeof(path_var),"%s/%s", path_node->path, cmd);
			ret_val = access(path_var, F_OK);
			if(ret_val == -1)
			{
				TRACE("Command does not exist in %s", path_node->path);
			}
			else
			{
				ret_val = execute(path_var, cmd, params);
				break;
			}
		}
	}
	else
	{
		/* Either it is an absolute path or found in local directory */
		/* This must have been taken care of in a function already!*/
		WARN("We probably shouldn't land up here!");
	}

EXIT_LABEL:
	return(ret_val);
}

/**
   @brief Loop getting input and executing it.
 */
static void simsh_loop(void)
{
  char *input_string;
  char **args;
  int result;

  printf("\nWelcome to simsh, enter `help` to gain information on system.\nSystem ready for executing commands.");
  do {
    printf("\n> ");
    input_string = simsh_linereader();
    args = simsh_extract(input_string);
    result = simsh_execute(args);

    free(input_string);
    free(args);
  } while (result);
}

/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return result code
 */
int32_t main(void)
{
  INIT_CLL_ROOT(path);

  // Run command loop.
  simsh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}

/*
  Builtin function implementations.
*/

/**
   @brief Bultin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int simsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "simsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("simsh");
    }
  }
  return 1;
}

/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int simsh_help(char **args)
{
  int i;
  printf("Simple shell simulation\n");
  printf("Usage similar to shell utility, except for built-in commands you can run any simple shell command as well.\n");
  printf("The following are built in:\n");

  for (i = 0; i < simsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int simsh_exit(char **args)
{
  return 0;
}

/**
 * @brief Builtin command: pushd.
 */
int simsh_pushd(char **args)
{	int chdir_retval;
	struct path_string *stack_element;
	char *getcwd_retval;
	char path[MAX_CWD_LENGTH];
	
	getcwd_retval=getcwd(path,MAX_CWD_LENGTH);
	if(getcwd_retval==NULL)
	{	printf("\nCould not get current working directory");
		return -1;
	}
	printf("\nTrying cd to %s",*args);
	chdir_retval=chdir(*args);
	if(chdir_retval==0)
	{	
		stack_element=(struct path_string *)malloc(sizeof(struct path_string));
		stack_element->my_path=(char *)malloc(strlen(path)*sizeof(char));
		strcpy(stack_element->my_path,path);
		stack_element->next=path_stack.head;
		path_stack.head=stack_element;
	}
	return chdir_retval;
}

/*
 * @brief Builtin command: popd
 */
int simsh_popd(void)
{	struct path_string *stack_element;
	if(path_stack.head==NULL)
		return -1;
	else
	{	stack_element=path_stack.head;
		path_stack.head=stack_element->next;
		chdir(stack_element->my_path);
		free(stack_element->my_path);
		free(stack_element);
		return 0;
	}
}

/*
 * @brief Builtin command: path
 */
int simsh_path(char **args)
{
	PATH_LIST *iterator = NULL;
	PATH_LIST *insert = NULL;

	if(args[1]==NULL)
	{
		fprintf(stderr, "\nPATH=%s\n",print_path());
	}
	else if(strncmp(args[1], "+", 1)==0)
	{
		/* Add args[2] to path */
		TRACE("Add %s to path", args[2]);
		insert = (PATH_LIST *)malloc(sizeof(PATH_LIST));
		if(!insert)
		{
			ERROR("Error allocating memory");
			fprintf(stderr, "PATH Addition failed.\n");
		}
		else
		{
			INIT_CLL_NODE(insert->node, insert);
			insert->path = (char *)malloc(sizeof(char) * simsh_MAX_PATH_LEN);
			if(!insert->path)
			{
				ERROR("Error allocating memory");
				fprintf(stderr, "PATH Addition failed.\n");
				free(insert);
				insert = NULL;
			}
			else
			{
				memset(insert->path, 0, sizeof(char) * simsh_MAX_PATH_LEN);
				snprintf(insert->path, strlen(args[2])+1, "%s", args[2]);
				INSERT_BEFORE(insert->node, path);
			}	
		}
	}
	else if(strncmp(args[1], "-", 1)==0)
	{
		TRACE("Remove %s from path", args[2]);
		for(iterator = (PATH_LIST *)NEXT_IN_LIST(path);
			iterator != NULL;
			iterator = (PATH_LIST *)NEXT_IN_LIST(iterator->node))
		{
			if(strncmp(iterator->path, args[2], ((strlen(args[2])> strlen(iterator->path) ? strlen(iterator->path): strlen(args[2]))))==0)
			{
				TRACE("Found entry!");
				break;
			}
		}
		if(iterator != NULL)
		{
			REMOVE_FROM_LIST(iterator->node);
			free(iterator->path);
			free(iterator);
			iterator = NULL;
		}
		else
		{
			fprintf(stderr, "%s not found in PATH\n", args[2]);
		}
	}
	else
	{
		ERROR("Unknown option.");
	}
  return 1;
}

/*
 * @brief Builtin command: dirs
 */
int simsh_dirs(void)
{	struct path_string *stack_element;
	stack_element=path_stack.head;
	if(stack_element==NULL)
		return -1;
	printf("\nStack Contents");
	while(stack_element!=NULL)
	{	printf("\n%s",stack_element->my_path);
		stack_element=stack_element->next;
	}
	return 0;
}
/**
  @brief execute a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */
int simsh_runsys(char **args)
{

  try_execute(args[0], args);

  return 1;
}

/*
 * @brief check whether a command exists in current directory or not
 * @param arg The name of the command to search
 * @return returns 1 if the command is present locally else 0
 */
int simsh_islocal(char *arg)
{
  FILE *fp;
  char cmd[1024];
  sprintf(cmd, "%s", arg);
  fp = fopen(cmd, "r");
  if(fp!=NULL)
  {
    fclose(fp);
    return 1;
  }
  else
  {
    return 0;
  }
}

/*
 * @brief execute a local program and wait for it to terminate
 * @param args Null terminated list of arguments
 * @return Always returns 1, to continue execution
 */
int simsh_runlocal(char **args)
{
  pid_t pid;
  int result;
  char path[1024];
  memset(path, 0, sizeof(path));

  pid = fork();
  if (pid == 0) {
    // Child process
    sprintf(path, "./%s", args[0]);
    if (execl((const char *)path, (const char *)args[0], args, (char *)NULL) == -1) {
      perror("simsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("simsh");
  } else {
    // Parent process
    do {
      waitpid(pid, &result, WUNTRACED);
    } while (!WIFEXITED(result) && !WIFSIGNALED(result));
  }

  return 1;
}

/**
   @brief Execute shell built-in or runsys program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int simsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < simsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }
  
  if(simsh_islocal(args[0])!=0)
  {
    return simsh_runlocal(args);
  }
  return simsh_runsys(args);
}

/**
   @brief Read a input_string of input from stdin.
   @return The input_string from stdin.
 */
char *simsh_linereader(void)
{
  int bufsize = simsh_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "simsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += simsh_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "simsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

/**
   @brief Split a input_string into tokens (very naively).
   @param input_string The line.
   @return Null-terminated array of tokens.
 */
char **simsh_extract(char *line)
{
  int bufsize = simsh_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "simsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, simsh_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += simsh_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		free(tokens_backup);
        fprintf(stderr, "simsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, simsh_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

