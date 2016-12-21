
#include "tinyos.h"
#include "kernel_cc.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_threads.h"

void* aquire_ptcb()
{
  void* ptr = xmalloc(sizeof(PTCB));
  return ptr;
}

void release_ptcb(void* ptr)
{
  free(ptr);
}


void start_thread()
{
  Task task = CURTHREAD->owner_ptcb->task;
  int argl = CURTHREAD->owner_ptcb->argl;
  void* args = CURTHREAD->owner_ptcb->args;
  int exitval = task(argl, args); //what about null temp_task etc?
  ThreadExit(exitval);
}


/** 
  @brief Initialize a new thread.
  */
PTCB* create_thread(Task task, int argl, void* args)
{
  PTCB* ptcb = (PTCB*) aquire_ptcb();
  
  ptcb->refCount = 0;
  ptcb->isDetached = false;
  ptcb->state = LIVING;

  ptcb->task = task;
  ptcb->argl = argl;
  ptcb->args = args;

  rlnode_init(& ptcb->ptcb_node, ptcb);
  ptcb->join_exit = COND_INIT;

  return ptcb;
}


/** 
  @brief Create a new thread in the current process.
  */
Tid_t CreateThread(Task task, int argl, void* args)
{
	PTCB* ptcb;

  Mutex_Lock(&kernel_mutex);

  ptcb = create_thread(task, argl, args);
  rlist_push_back(& CURPROC->ptcb_list, & ptcb->ptcb_node);
  ptcb->tcb = spawn_thread(CURPROC, ptcb, start_thread);
  wakeup(ptcb->tcb);

  Mutex_Unlock(&kernel_mutex);

  return (Tid_t) ptcb->tcb;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t ThreadSelf()
{
	return (Tid_t) CURTHREAD;
}


/** 
  @brief Cleanup of an exited ptcb.
  */
void bring_out_your_dead(PTCB* ptcb, int* exitval)
{
  if(exitval != NULL)
    *exitval = ptcb->exitval;

  rlist_remove(& ptcb->ptcb_node);

  //release_TCB(ptcb->tcb);
  release_ptcb(ptcb);
}

/**
  @brief Join the given thread.
  */
int ThreadJoin(Tid_t tid, int* exitval)
{
  bool err = false;
  PTCB* ptcb;
  Mutex_Lock(&kernel_mutex);

  TCB* tcb = tid;
  ptcb = tcb->owner_ptcb;
  
  PTCB* curr = CURTHREAD->owner_ptcb;

  /** Error checks */
  if (ptcb->isDetached == true) {
    err = true;
    goto finish;
  }
  if (tcb->owner_pcb != CURTHREAD->owner_pcb) {
    err = true;
    goto finish;
  }
  if (tcb == CURTHREAD) {
    err = true;
    goto finish;
  }

  curr->refCount++;
  /**Ok ptcb is legal. Wait for it to exit. */
  while(ptcb->state == LIVING)
    Cond_Wait(& kernel_mutex, & ptcb->join_exit);

  /** Clean the exited ptcb. */
  bring_out_your_dead(ptcb, exitval);

  curr->refCount--;
finish:

  Mutex_Unlock(&kernel_mutex);

  switch(err){
    case true:
      return -1;
    case false:
      return 0;
  }
	return -1;
}

/**
  @brief Detach the given thread.
  */
int ThreadDetach(Tid_t tid)
{
  bool err = false;
  Mutex_Lock(&kernel_mutex);
  TCB* tcb = tid;
  PTCB* ptcb = tcb->owner_ptcb;

  if (tcb->state == EXITED || ptcb->state == DEAD)
  {
    err = true;
  }

  /** Detach the thread and broadcast all threads that had
  joined it. */
  ptcb->isDetached = true;
  Cond_Broadcast(& ptcb->join_exit);

  Mutex_Unlock(&kernel_mutex);
	
  switch(err){
    case true:
      return -1;
    case false:
      return 0;
  }
  return -1;
}

/**
  @brief Terminate the current thread.
  */
void ThreadExit(int exitval)
{
  PTCB* ptcb = CURTHREAD->owner_ptcb;

  Mutex_Lock(&kernel_mutex);

  /** Check for a valid refCount. */
  assert (ptcb->refCount<=0);

    if (exitval != NULL)
      ptcb->exitval = exitval;

    ptcb->state = DEAD;

    /** Remove from process's list of ptcbs. */
    rlist_remove(& ptcb->ptcb_node);

    /** Broadcast all threads that had
    joined this one. */
    Cond_Broadcast(& ptcb->join_exit);

    /** Bye bye cruel world :P */
    sleep_releasing(EXITED, & kernel_mutex);

    //bring_out_your_dead(ptcb, NULL);

  Mutex_Unlock(&kernel_mutex);
}


/**
  @brief Awaken the thread, if it is sleeping.

  This call will set the interrupt flag of the
  thread.

  */
int ThreadInterrupt(Tid_t tid)
{
	return -1;
}


/**
  @brief Return the interrupt flag of the 
  current thread.
  */
int ThreadIsInterrupted()
{
	return 0;
}

/**
  @brief Clear the interrupt flag of the
  current thread.
  */
void ThreadClearInterrupt()
{

}

