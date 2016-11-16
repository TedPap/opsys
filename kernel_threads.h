#ifndef __KERNEL_THREADS_H
#define __KERNEL_THREADS_H

#include "tinyos.h"
#include "kernel_proc.h"


typedef struct process_thread_control_block
{
  PCB* owner_pcb;
  TCB* tcb;
  rlnode ptcb_node;
  //Task task;
  //int argl;
  //void* args;
  uint refcount;
  
}PTCB;


void start_thread();


Tid_t CreateThread(Task task, int argl, void* args);

/**
  @brief Return the Tid of the current thread.
 */
Tid_t ThreadSelf();

/**
  @brief Join the given thread.
  */
int ThreadJoin(Tid_t tid, int* exitval);

/**
  @brief Detach the given thread.
  */
int ThreadDetach(Tid_t tid);

/**
  @brief Terminate the current thread.
  */
void ThreadExit(int exitval);


/**
  @brief Awaken the thread, if it is sleeping.

  This call will set the interrupt flag of the
  thread.

  */
int ThreadInterrupt(Tid_t tid);


/**
  @brief Return the interrupt flag of the 
  current thread.
  */
int ThreadIsInterrupted();

/**
  @brief Clear the interrupt flag of the
  current thread.
  */
void ThreadClearInterrupt();



#endif