#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main()
{
	pid_t pid;
  int result;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execl("./test.sh", "test.sh", (char *)NULL) == -1) 
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
