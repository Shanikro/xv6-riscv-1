#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NTEAMS       3
#define RUNNERS_TEAM 5
#define TARGET       30
#define FAVORITISM   0

int
main(void)
{
  int lock_id = israeli_create(FAVORITISM);
  if(lock_id < 0){
    printf("israeli_create failed\n");
    exit(1);
  }

  score_init(NTEAMS);

  // fork interleaved: one runner per team at a time for a fair start
  for(int r = 0; r < RUNNERS_TEAM; r++){
    for(int team = 0; team < NTEAMS; team++){
      int pid = fork();
      if(pid < 0){
        printf("fork failed\n");
        exit(1);
      }
      if(pid == 0){
        // child: assign team
        setgid(team);

        while(1){
          israeli_acquire(lock_id);

          // check if race is already over - some team has reached the target score
          int over = 0;
          for(int t = 0; t < NTEAMS; t++){
            if(score_get(t) >= TARGET){
              over = 1;
              break;
            }
          }
          if(over){
            israeli_release(lock_id);
            break;
          }

          // increment this team's score - this team is now holding the baton
          int s = score_inc(team);
          printf("Runner %d (Team %d) acquired the baton\nTeam %d score = %d\n\n",
                 getpid(), team, team, s);

          int done = (s >= TARGET);
          israeli_release(lock_id);

          if(done)
            break; // race is over, break the while loop

          sleep(1); // sleep for 1 second
        }
        exit(0); // exit the child process
      }
    }
  }

  // parent waits for all children
  for(int i = 0; i < NTEAMS * RUNNERS_TEAM; i++)
    wait(0);

  // print final scores and winner
  printf("=== Race over ===\n");
  int winner = 0;
  for(int t = 0; t < NTEAMS; t++){
    int s = score_get(t);
    printf("Team %d final score: %d\n", t, s);
    if(s > score_get(winner)) // update the winner if this team has a higher score
      winner = t;
  }
  printf("Winner: Team %d\n", winner);

  israeli_destroy(lock_id); // destroy the israeli lock
  exit(0);
}
