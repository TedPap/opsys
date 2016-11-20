#ifndef __KERNEL_THREADS_H
#define __KERNEL_THREADS_H

#include "tinyos.h"
#include "util.h"
#include "bios.h"
#include "kernel_proc.h"


/** 
  @brief The ptcb state

  The state of the ptcb. It can either be alive(LIVING) or aa zombie(DEAD). 
  */

typedef enum pthread_state_e {
  LIVING,  /**< The PTCB is in use, its thread has not EXITED */
  DEAD  /**< The PTCB is a zombie, its thread has EXITED */
} ptcb_state;


/**
  @brief Process Thread Control Block

  This structure holds all information pertaining to a ptcb.
 */

typedef struct pthread_control_block
{
  TCB* tcb;               /**< The thread of this PTCB */
  int exitval;            /**< The exit value */
  uint refCount;          /**< This PTCB's refCount */

  Task task;              /**< The thread's task */
  int argl;               /**< The thread's argument length */
  void* args;             /**< The thread's argument string */

  CondVar join_exit;      /**< Condition variable for ThreadJoin */
  bool isDetached;        /**< Flag for ThreadDetach */

  ptcb_state state;       /**< The state of this PTCB */

  rlnode ptcb_node;       /**< Intrusive node for ptcb_list of each process */
  
}PTCB;


/**
  @brief Allocate memory for the ptcb instant.

  This function is called during ptcb initialization( create_thread ).
  */

void* aquire_ptcb();


/**
  @brief Free the memory allocated to the ptcb instant.

  This function is called during ptcb cleanup( bring_out_your_dead ).
  */

void release_ptcb(void* ptr);


/**
  @brief The function that runs the thread's task.

  This function is called during ptcb creation( CreateThread ).
  */

void start_thread();


/**
  @brief Initialization of the ptcb struct.

  This function is called during ptcb creation( CreateThread, Exec ).
  */

PTCB* create_thread(Task task, int argl, void* args);

/**
  @brief Cleanup of the exited thread's ptcb.

  This function is called after a thread has exited in ThreadJoin.
  */

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