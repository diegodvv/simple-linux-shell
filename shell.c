#include <stdio.h>
#include <unistd.h>

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
      printf("Unrecognized command\n");
    }

    print_cwd();
  }

  return 0;
}