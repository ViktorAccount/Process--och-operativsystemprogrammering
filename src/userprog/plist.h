#ifndef _PLIST_H_
#define _PLIST_H_

#include "../filesys/file.h"
#include <stdbool.h>
#include "threads/synch.h"

/* The global process list container */
struct plist {
  struct list plist_elem_list;  /* List of processes */
  struct lock plist_lock;       /* Lock for the entire list */
};

struct plist_elem {
  int pid;                /* Process ID */
  int parent_id;          /* Parent process ID */
  int exit_status;        /* Exit status code */
  bool alive;             /* True if process is still running */
  bool parent_alive;      /* True if parent process is still running */
  bool waited;            /* True if this process has been waited on */
  struct semaphore sema;  /* Semaphore for wait synchronization */

  struct list_elem elem;  /* List element for inclusion in plist */
  struct lock lock;       /* Lock for this specific process element */
};


/* Accessor for the global plist container */
struct plist* get_plist_container(void);

/* 
   Initializes the process list
   Must be called before any other plist functions
*/
void plist_init(void);

/*
   Completely destroys the process list
   Frees all memory and resets the list to uninitialized state
*/
void plist_destroy(void);

/* 
   Inserts a new process into the process list
   
   @param pid Process ID of the new process
   @param parent_id Process ID of the parent process
   @return The process ID if successful, -1 on failure
*/
int plist_insert(int pid, int parent_id);

/* 
   Finds a process in the list by its process ID
   
   @param pid Process ID to search for
   @return Pointer to the process element if found, NULL otherwise
*/
struct plist_elem* plist_find(int pid);

/* 
   Removes a process from the list
   
   @param pid Process ID to remove
*/
void plist_remove(int pid);

/* 
   Executes a function for each process in the list
   
   @param exec Function to execute for each process element
*/
void plist_for_each(void (*exec)(struct plist_elem* p));

/* 
   Marks all child processes of a parent as having dead parent
   
   @param parent_tid Process ID of the parent
*/
void plist_set_parent_dead(int parent_tid);

/* 
   Removes all child processes of a parent
   
   @param parent_pid Process ID of the parent
*/
void plist_remove_all_children(int parent_pid);

/* 
   Cleans up processes that can be safely removed
   Removes processes that are both dead and have dead parents
*/
void plist_clean_everything(void);

#endif
