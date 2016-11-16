
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

void release_ptcb(void* ptr, size_t size)
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
  
  ptcb->refcount = 1;
  ptcb->tjoin = COND_INIT;
  ptcb->task = task;
  ptcb->argl = argl;
  ptcb->args = args;
  rlnode_init(& ptcb->ptcb_node, ptcb);

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
    ptcb->tcb = spawn_thread(CURPROC, start_thread);
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
  @brief Join the given thread.
  */
int ThreadJoin(Tid_t tid, int* exitval)
{
	return -1;
}

/**
  @brief Detach the given thread.
  */
int ThreadDetach(Tid_t tid)
{
	return -1;
}

/**
  @brief Terminate the current thread.
  */
void ThreadExit(int exitval)
{

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

