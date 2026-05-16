#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "spinlock.h"

#define MAX_TEAMS 16

static int scores[MAX_TEAMS];
static int nteams = 0;
static struct spinlock scores_lock;

void
score_init(int n)
{
  initlock(&scores_lock, "scores");
  nteams = n;
  for(int i = 0; i < n; i++)
    scores[i] = 0;
}

int
score_inc(int team_id)
{
  if(team_id < 0 || team_id >= nteams)
    return -1;
  acquire(&scores_lock);
  int s = ++scores[team_id];
  release(&scores_lock);
  return s;
}

int
score_get(int team_id)
{
  if(team_id < 0 || team_id >= nteams)
    return -1;
  acquire(&scores_lock);
  int s = scores[team_id];
  release(&scores_lock);
  return s;
}
