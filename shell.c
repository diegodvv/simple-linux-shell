#include <stdio.h>
#include <unistd.h>
#include <string.h>

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
      while (token != NULL)
      {
        arguments[arguments_count++] = token;
        token = strtok_r(NULL, " ", &saveptr);
      }

      printf("Unrecognized command: %s\n", program);
    }

    print_cwd();
  }

  return 0;
}