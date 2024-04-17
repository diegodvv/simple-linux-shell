#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

void exit_command(char **arguments, int arguments_count)
{
  // Disables the unused parameter warning for arguments
  (void)arguments;

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

void exec_command(char **arguments, int arguments_count, bool should_exit)
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
    arguments[arguments_count] = NULL;
    if (execv(arguments[0], arguments) == -1)
    {
      perror("execv() error");
      exit(errno);
    }
  }
  else
  {
    // Parent process
    int status;
    waitpid(pid, &status, 0);
  }

  if (should_exit)
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

bool try_to_exec_command_in_path(char *program, char **arguments, int arguments_count)
{
  char *path = getenv("PATH");

  if (path == NULL)
  {
    printf("Error: PATH environment variable not set\n");
    return false;
  }

  char path_copy[10000];
  strncpy(path_copy, path, strlen(path));

  char *saveptr;
  char *dir = strtok_r(path_copy, ":", &saveptr);
  while (dir != NULL)
  {
    char command_path[1024];
    snprintf(command_path, sizeof(command_path), "%s/%s", dir, program);

    if (access(command_path, X_OK) == 0)
    {
      char *arguments_with_program[100];
      arguments_with_program[0] = command_path;
      for (int i = 0; i < arguments_count; i++)
      {
        arguments_with_program[i + 1] = arguments[i];
      }

      exec_command(arguments_with_program, arguments_count + 1, false);
      return true;
    }

    dir = strtok_r(NULL, ":", &saveptr);
  }

  return false;
}

char *replace_home_directory(const char *path)
{
  if (path == NULL)
    return NULL;

  const char *home = getenv("HOME");
  if (home == NULL)
    return strdup(path);

  size_t home_len = strlen(home);
  size_t path_len = strlen(path);

  char *result = malloc(100000);
  if (result == NULL)
    return NULL;

  size_t result_index = 0;
  for (size_t i = 0; i < path_len; i++)
  {
    if (path[i] == '~')
    {
      strcpy(result + result_index, home);
      result_index += home_len;
    }
    else
    {
      result[result_index++] = path[i];
    }
  }
  result[result_index] = '\0';

  return result;
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
      char *program = replace_home_directory(strtok_r(input, " ", &saveptr));

      char *token = strtok_r(NULL, " ", &saveptr);
      arguments_count = 0;
      while (token != NULL)
      {
        arguments[arguments_count++] = replace_home_directory(token);
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
        exec_command(arguments, arguments_count, true);
      }
      else if (program[0] == '.' || program[0] == '/')
      {
        char *arguments_with_program[100];
        arguments_with_program[0] = program;
        for (int i = 0; i < arguments_count; i++)
        {
          arguments_with_program[i + 1] = arguments[i];
        }

        exec_command(arguments_with_program, arguments_count + 1, false);
      }
      else
      {
        bool executed_program = try_to_exec_command_in_path(program, arguments, arguments_count);

        if (!executed_program)
          printf("Unrecognized command: %s\n", program);
      }

      free(program);
      for (int i = 0; i < arguments_count; i++)
      {
        free(arguments[i]);
      }
    }

    print_cwd();
  }

  return 0;
}