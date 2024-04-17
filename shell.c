#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

void exit_command(char **arguments, int arguments_count)
{
  if (arguments_count > 0)
  {
    printf("Error: No arguments should be provided to exit\n");
    return;
  }

  exit(0);
}

void cd_command(char **arguments, int arguments_count)
{
  if (arguments_count != 1)
  {
    printf("Error: cd command takes exactly one argument\n");
    return;
  }

  if (chdir(arguments[0]) != 0)
  {
    perror("chdir() error");
    return;
  }
}

void exec_command(char **arguments, int arguments_count)
{
  if (arguments_count < 1)
  {
    printf("Error: exec command requires at least one argument\n");
    return;
  }

  pid_t pid = fork();
  if (pid < 0)
  {
    perror("fork() error");
    return;
  }
  else if (pid == 0)
  {
    // Child process
    char **arguments_to_new_program = malloc(sizeof(arguments[0]) * (arguments_count - 1));

    for (int i = 1; i < arguments_count; i++)
    {
      arguments_to_new_program[i - 1] = arguments[i];
    }

    if (execv(arguments[0], arguments_to_new_program) == -1)
    {
      perror("execv() error");
      exit(errno);
    }

    free(arguments_to_new_program);
  }
  else
  {
    // Parent process
    int status;
    waitpid(pid, &status, 0);
  }

  // The shell does not return after executing the command
  exit(0);
}

void print_cwd()
{
  char cwd[1024];

  if (getcwd(cwd, sizeof(cwd)) == 0)
  {
    perror("getcwd() error");
    return;
  }

  printf("%s$ ", cwd);
}

int main()
{
  char input[1024];
  char *arguments[100];
  int arguments_count = 0;

  print_cwd();
  while (fgets(input, sizeof(input), stdin) != NULL)
  {
    // Remove trailing newline
    if (input[strlen(input) - 1] == '\n')
    {
      input[strlen(input) - 1] = '\0';
    }

    if (strlen(input) > 0)
    {
      char *saveptr;
      char *program = strtok_r(input, " ", &saveptr);

      char *token = strtok_r(NULL, " ", &saveptr);
      arguments_count = 0;
      while (token != NULL)
      {
        arguments[arguments_count++] = token;
        token = strtok_r(NULL, " ", &saveptr);
      }

      if (strcmp(program, "exit") == 0)
      {
        exit_command(arguments, arguments_count);
      }
      else if (strcmp(program, "cd") == 0)
      {
        cd_command(arguments, arguments_count);
      }
      else if (strcmp(program, "exec") == 0)
      {
        exec_command(arguments, arguments_count);
      }
      else
      {
        printf("Unrecognized command: %s\n", program);
      }
    }

    print_cwd();
  }

  return 0;
}