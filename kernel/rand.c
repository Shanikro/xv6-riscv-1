#include "types.h"
#include "spinlock.h"
#include "defs.h"

static uint current_x;
static struct spinlock rand_lock;

uint a = 1664525;
uint b = 1013904223;

void
lcg_srand(uint seed)
{
  acquire(&rand_lock);
  current_x = seed;
  release(&rand_lock);
}

uint
lcg_rand(void)
{
  acquire(&rand_lock);
  current_x = a * current_x + b; // unsigned int includes overflow checking by 32 bits
  uint result = current_x;
  release(&rand_lock);
  return result;
}