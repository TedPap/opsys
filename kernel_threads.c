
#include "tinyos.h"
#include "kernel_cc.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_threads.h"

Task TEMP_TASK;
int TEMP_ARGL;
void* TEMP_ARGS;

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
  int exitval = TEMP_TASK(TEMP_ARGL, TEMP_ARGS); //what about null temp_task etc
  ThreadExit(exitval);
}

PTCB* create_thread(Task task, int argl, void* args)
{
  PTCB* ptcb = (PTCB*) aquire_ptcb();
  
  ptcb->refCount = 1;
  ptcb->isDetached = false;

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
  TEMP_TASK = task;
  TEMP_ARGL = argl;
  TEMP_ARGS = args;
  ptcb->tcb = spawn_thread(CURPROC, ptcb, start_thread);
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

void bring_out_your_dead(PTCB* ptcb, int* exitval)
{
  if(exitval != NULL)
    *exitval = ptcb->exitval;

  rlist_remove(& ptcb->ptcb_node);

  release_ptcb(ptcb);
}

/**
  @brief Join the given thread.
  */
int ThreadJoin(Tid_t tid, int* exitval)
{
  bool err = false;
  Mutex_Lock(&kernel_mutex);
  TCB* tcb = tid;
  PTCB* ptcb = tcb->owner_ptcb;
  PTCB* curr = CURTHREAD->owner_ptcb;
  if (ptcb->isDetached == true)
    err = true;
  if (tcb->owner_pcb != CURTHREAD->owner_pcb)
    err = true;
  if (tcb == CURTHREAD)
    err = true;

  curr->refCount++;

  while(tcb->state != EXITED)
    Cond_Wait(& kernel_mutex, & ptcb->join_exit);

  bring_out_your_dead(ptcb, exitval);

  curr->refCount--;

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

  if (tcb->state == EXITED)
  {
    err = true;
  }

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

  if(ptcb->args) {
    free(ptcb->args);
    ptcb->args = NULL;
  }

  

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

