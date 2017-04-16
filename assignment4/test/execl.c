#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	pid_t pid;
  int result;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execl(argv[1], argv[1], *(argv + 2), (char *)NULL) == -1) 
    {
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
}
