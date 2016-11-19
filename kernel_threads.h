#ifndef __KERNEL_THREADS_H
#define __KERNEL_THREADS_H

#include "tinyos.h"
#include "util.h"
#include "bios.h"
#include "kernel_proc.h"

typedef enum pthread_state_e {
  LIVING,  /**< The PID is given to a process */
  DEAD  /**< The PID is held by a zombie */
} ptcb_state;

typedef struct pthread_control_block
{
  TCB* tcb;
  int exitval;
  uint refCount;

  Task task;
  int argl;
  void* args;

  CondVar join_exit;
  bool isDetached;

  ptcb_state state;

  rlnode ptcb_node;
  
}PTCB;


void* aquire_ptcb();

void release_ptcb(void* ptr);

void start_thread();

PTCB* create_thread(Task task, int argl, void* args);

void bring_out_your_dead(PTCB* ptcb, int* exitval);


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