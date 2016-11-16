#ifndef __KERNEL_THREADS_H
#define __KERNEL_THREADS_H

#include "tinyos.h"
#include "util.h"
#include "bios.h"
#include "kernel_proc.h"


typedef struct pthread_control_block
{
  TCB* tcb;
  rlnode ptcb_node;
  Task task;
  int argl;
  void* args;
  uint refcount;
  CondVar tjoin;
  
}PTCB;


void* aquire_ptcb();

void release_ptcb(void* ptr, size_t size);

void start_thread();

PTCB* create_thread(Task task, int argl, void* args);


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