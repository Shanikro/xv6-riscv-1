#define ISRAELI_QUEUE_SIZE 16

struct israelilock {
  int active;                      // is the lock already created?
  int locked;                      // is the lock currently held?
  int favoritism;                  // favoritism coefficient (0-100)
  int holder_gid;                  // gid of the process currently holding it
  int holder_pid;                  // pid of the process currently holding it
  int queue[ISRAELI_QUEUE_SIZE];   // PIDs of waiting processes (FIFO)
  int queue_head;                  // index of front of queue
  int queue_tail;                  // index of back of queue
  int queue_size;                  // number of processes waiting
  struct spinlock lk;              // protects this struct
};