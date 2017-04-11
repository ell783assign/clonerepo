#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
{
	return 0;
}

/*
 * @brief Builtin command: popd
 */
int simsh_popd(char **args)
{
	return 0;
}

/*
 * @brief Builtin command: path
 */
int simsh_path(char **args)
{
	return 0;
}

/*
 * @brief Builtin command: dirs
 */
int simsh_dirs(char **args)
{
	return 0;
}
/**
  @brief execute a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */
int simsh_runsys(char **args)
{
  pid_t pid;
  int result;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
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
		fclose(fp);
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

#define simsh_RL_BUFSIZE 1024
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

#define simsh_TOK_BUFSIZE 64
#define simsh_TOK_DELIM " \t\r\n\a"
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

/**
   @brief Loop getting input and executing it.
 */
void simsh_loop(void)
{
  char *input_string;
  char **args;
  int result;

  do {
    printf("\nWelcome to simsh, enter `help` to gain information on system.\n System ready for executing commands\n> ");
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
int main(int argc, char **argv)
{
  // Load config files, if any.

  // Run command loop.
  simsh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}

