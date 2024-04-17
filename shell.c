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
    // Process the user input here
    // ...

    print_cwd();
  }

  return 0;
}