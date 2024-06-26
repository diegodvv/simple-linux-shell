#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <pwd.h>

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

void exec_command(char **arguments, int arguments_count, bool should_exit, FILE *input_file, FILE *output_file)
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

    if (input_file != NULL)
    {
      // Redirect stdin to input file
      if (dup2(fileno(input_file), STDIN_FILENO) == -1)
      {
        perror("dup2() error");
        exit(errno);
      }
    }
    if (output_file != NULL)
    {
      // Redirect stdout to output file
      if (dup2(fileno(output_file), STDOUT_FILENO) == -1)
      {
        perror("dup2() error");
        exit(errno);
      }
    }

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

bool try_to_exec_command_in_path(char *program, char **arguments, int arguments_count, FILE *input_file, FILE *output_file)
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

      exec_command(arguments_with_program, arguments_count + 1, false, input_file, output_file);
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

  size_t path_len = strlen(path);

  char *result = malloc(100000);
  if (result == NULL)
    return NULL;

  size_t result_index = 0;
  for (size_t i = 0; i < path_len; i++)
  {
    if (path[i] == '~')
    {
      size_t username_len = 0;
      if (path[i + 1] != '\0' && path[i + 1] != '/')
      {
        const char *end = strchr(path + i + 1, '/');
        if (end != NULL)
        {
          username_len = end - (path + i + 1);
        }
        else
        {
          username_len = strlen(path + i + 1);
        }
      }

      if (username_len == 0)
      {
        // Replace '~' as '$HOME'
        strcpy(result + result_index, home);
        result_index += strlen(home);
      }
      else
      {
        char *username = strndup(path + i + 1, username_len);
        struct passwd *pw = getpwnam(username);
        if (pw != NULL)
        {
          // Replace '~username' as 'username_home_path'
          strcpy(result + result_index, pw->pw_dir);
          result_index += strlen(pw->pw_dir);
        }
        else
        {
          // Maintain '~username' as '~username'
          strncpy(result + result_index, path + i, username_len + 1);
          result_index += username_len + 1;
        }
        free(username);
      }

      i += username_len;
    }
    else
    {
      result[result_index++] = path[i];
    }
  }
  result[result_index] = '\0';

  return result;
}

struct Command
{
  char *arguments[100];
  int arguments_count;

  char *program;
  char *input_file_name;
  char *output_file_name;
};

int main()
{
  char input[1024];
  struct Command *commands[100];
  int commands_count = 0;

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
      struct Command *command = malloc(sizeof(struct Command));
      if (command == NULL)
      {
        perror("malloc() error");
        return 1;
      }
      char **arguments = command->arguments;

      char *saveptr;
      char *program = replace_home_directory(strtok_r(input, " \t\n\r", &saveptr));

      char *token = strtok_r(NULL, " \t\n\r", &saveptr);
      int arguments_count = 0;
      char *input_file_name = NULL;
      char *output_file_name = NULL;
      while (token != NULL)
      {
        if (strcmp(token, "<") == 0 || strcmp(token, ">") == 0)
        {
          if (strcmp(token, "<") == 0)
          {
            input_file_name = replace_home_directory(strtok_r(NULL, " \t\n\r", &saveptr));
          }
          else
          {
            output_file_name = replace_home_directory(strtok_r(NULL, " \t\n\r", &saveptr));
          }
        }
        // Don't store to program arguments the redirection operators
        else
        {
          arguments[arguments_count++] = replace_home_directory(token);
        }

        token = strtok_r(NULL, " \t\n\r", &saveptr);
      }

      command->arguments_count = arguments_count;
      command->input_file_name = input_file_name;
      command->output_file_name = output_file_name;
      command->program = program;
      commands[commands_count++] = command;
    }

    for (int i = 0; i < commands_count; i++)
    {
      struct Command *command = commands[i];
      char **arguments = command->arguments;
      int arguments_count = command->arguments_count;
      char *program = command->program;
      char *input_file_name = command->input_file_name;
      char *output_file_name = command->output_file_name;

      FILE *input_file;
      FILE *output_file;

      // Open input file if present
      if (input_file_name != NULL)
      {
        input_file = fopen(input_file_name, "r");
        if (input_file == NULL)
        {
          printf("Error: File '%s' does not exist\n", input_file_name);
          return 1;
        }
      }

      // Open output file if present
      if (output_file_name != NULL)
      {
        output_file = fopen(output_file_name, "w");
        if (output_file == NULL)
        {
          printf("Error: Unable to open file '%s'\n", output_file_name);
          return 1;
        }
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
        exec_command(arguments, arguments_count, true, input_file, output_file);
      }
      else if (program[0] == '.' || program[0] == '/')
      {
        char *arguments_with_program[100];
        arguments_with_program[0] = program;
        for (int i = 0; i < arguments_count; i++)
        {
          arguments_with_program[i + 1] = arguments[i];
        }

        exec_command(arguments_with_program, arguments_count + 1, false, input_file, output_file);
      }
      else
      {
        bool executed_program = try_to_exec_command_in_path(program, arguments, arguments_count, input_file, output_file);

        if (!executed_program)
          printf("Unrecognized command: %s\n", program);
      }

      free(program);
      for (int i = 0; i < arguments_count; i++)
      {
        free(arguments[i]);
      }

      if (input_file != NULL)
      {
        fclose(input_file);
        free(input_file_name);
      }
      if (output_file != NULL)
      {
        fclose(output_file);
        free(output_file_name);
      }
    }

    print_cwd();
  }

  return 0;
}