#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  lcg_srand(42);

  for(int i = 0; i < 10; i++){
    printf("%d\n", lcg_rand());
  }

  exit(0);
}
