#include <stdio.h>
#include <unistd.h>

int main()
{
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) == 0)
  {
    perror("getcwd() error");
    return 1;
  }

  printf("%s$ ", cwd);
  return 0;
}