#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "israelilock.h"


#define NISR 15
struct israelilock isrlocks[NISR];

extern struct proc proc[];

// initialize the israeli locks
void
israeli_init(void)
{
  for(int i = 0; i < NISR; i++){
    isrlocks[i].active = 0;
    isrlocks[i].locked = 0;
    isrlocks[i].favoritism = -1; // not favoritism yet
    isrlocks[i].holder_gid = -1; // no holder yet
    isrlocks[i].holder_pid = -1; // no holder yet
    isrlocks[i].queue_head = 0;
    isrlocks[i].queue_tail = 0;
    isrlocks[i].queue_size = 0;
    initlock(&isrlocks[i].lk, "israelilock"); // initialize the spinlock
  }
}

int
israeli_create(int favoritism)
{
    // find the first empty slot for new lock
    for(int i = 0; i < NISR; i++){
        acquire(&isrlocks[i].lk);
        if(isrlocks[i].active == 0){ // empty slot found
            isrlocks[i].active = 1; // set the slot to active
            isrlocks[i].favoritism = favoritism;
            release(&isrlocks[i].lk);
            return i; // return the lock id, success
        }
        release(&isrlocks[i].lk);
    }
    return -1; // no empty slot found, return -1
}


int
israeli_acquire(int lock_id)
{
    if(lock_id < 0 || lock_id >= NISR){
        return -1; // invalid lock id
    }

    struct israelilock *lk = &isrlocks[lock_id];
    acquire(&lk->lk);       // acquire the lock

    if(!lk->active){        // slot not created
      release(&lk->lk);
      return -1; // return -1, failed
    }

    // check queue not full
    if(lk->queue_size >= ISRAELI_QUEUE_SIZE){
        release(&lk->lk);
        return -1;
    }

    // add ourselves to the back of the FIFO queue
    struct proc *p = myproc();
    lk->queue[lk->queue_tail] = p->pid;
    lk->queue_tail = (lk->queue_tail + 1) % ISRAELI_QUEUE_SIZE;
    lk->queue_size++;

    // wait until we are at the front AND lock is free
    while(lk->locked || lk->queue[lk->queue_head] != p->pid){
        sleep(lk, &lk->lk);  // releases lk->lk while sleeping, reacquires on wakeup
    }

    // take the lock
    lk->queue_head = (lk->queue_head + 1) % ISRAELI_QUEUE_SIZE;
    lk->queue_size--;
    lk->locked = 1;
    lk->holder_pid = p->pid;
    lk->holder_gid = p->gid;

    // release the lock
    release(&lk->lk);
    return 0;
}

int
israeli_release(int lock_id)
{
    // check invalid lock id
    if(lock_id < 0 || lock_id >= NISR)
        return -1;

    struct israelilock *lk = &isrlocks[lock_id];
    acquire(&lk->lk);

    // check slot not created or not locked
    if(!lk->active || !lk->locked){
        release(&lk->lk);
        return -1;
    }

    // release the lock
    lk->locked = 0;
    lk->holder_pid = -1;

    // favoritism: with probability favoritism/100, promote earliest same-gid process
    {
        int prev_gid = lk->holder_gid; // the current holder's gid
        lk->holder_gid = -1;

        if(lk->queue_size > 1 && lk->favoritism > 0 && // queue not empty and favoritism is set
           (int)(lcg_rand() % 100) < lk->favoritism){ // random number less than favoritism

            // find gid of the front process
            int front_gid = -1;
            for(struct proc *pp = proc; pp < &proc[NPROC]; pp++){
                if(pp->pid == lk->queue[lk->queue_head]){
                    front_gid = pp->gid;
                    break;
                }
            }
            // only swap if front is NOT already same-gid as previous holder
            if(front_gid != prev_gid){
                for(int i = 1; i < lk->queue_size; i++){ // iterate through the queue to find the process to swap
                    int idx = (lk->queue_head + i) % ISRAELI_QUEUE_SIZE;
                    // find the process to swap
                    for(struct proc *pp = proc; pp < &proc[NPROC]; pp++){
                        if(pp->pid == lk->queue[idx] && pp->gid == prev_gid){ 
                            // swap the process to the front
                            int tmp = lk->queue[lk->queue_head];
                            lk->queue[lk->queue_head] = lk->queue[idx];
                            lk->queue[idx] = tmp;
                            goto done_favoritism;
                        }
                    }
                }
            }
        }
    }
done_favoritism:
    wakeup(lk); // wake up the next process in the queue
    release(&lk->lk);
    return 0;
}

int
israeli_destroy(int lock_id)
{
    if(lock_id < 0 || lock_id >= NISR){
        return -1; // invalid lock id
    }

    struct israelilock *lk = &isrlocks[lock_id];
    acquire(&lk->lk);       // acquire the lock

    if(!lk->active){        // slot not created
      release(&lk->lk);
      return -1; // return -1, failed
    }

    lk->active = 0;         // mark as free
    //reset the lock
    lk->locked = 0;
    lk->favoritism = -1;
    lk->holder_gid = -1;
    lk->holder_pid = -1;
    //reset the queue
    memset(lk->queue, 0, sizeof(lk->queue));
    lk->queue_head = 0;
    lk->queue_tail = 0;
    lk->queue_size = 0;
    //release the lock
    release(&lk->lk);
    return 0; // success
}