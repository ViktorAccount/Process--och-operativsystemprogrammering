#include "plist.h"
#include <stddef.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesys/file.h"

/* Global process list */
struct plist plist;

struct plist* get_plist_container(void) {
    return &plist;
}

void plist_init()
{
    list_init(&plist.plist_elem_list);
    lock_init(&plist.plist_lock);
}


void plist_destroy(void)
{
    /* Acquire lock to prevent concurrent access */
    lock_acquire(&plist.plist_lock);
    
    /* Free all elements in the list */
    struct list_elem *e = list_begin(&plist.plist_elem_list);
    while (e != list_end(&plist.plist_elem_list)) {
        struct list_elem *next = list_next(e);
        struct plist_elem *plist_entry = list_entry(e, struct plist_elem, elem);
        free(plist_entry);
        e = next;
    }
    
    /* Reset the list */
    list_init(&plist.plist_elem_list);
    lock_release(&plist.plist_lock);
}

int plist_insert(int pid, int parent_id)
{ 

    struct plist_elem *plist_entry = malloc(sizeof(struct plist_elem));
    if(plist_entry == NULL) return -1; /* Failed to allocate memory */

    plist_entry->pid = pid;
    plist_entry->parent_id = parent_id;
    plist_entry->alive = true;
    plist_entry->parent_alive = true;
    plist_entry->waited = false;
    plist_entry->exit_status = -1; /* Default exit status */
    lock_init(&plist_entry->lock);
    sema_init(&plist_entry->sema, 0);

    /* Add to process list with proper synchronization */
    lock_acquire(&plist.plist_lock);
    list_push_back(&plist.plist_elem_list, &plist_entry->elem);
    lock_release(&plist.plist_lock);

    return pid;
}


struct plist_elem* plist_find(int pid)
{ 

    struct plist_elem *found = NULL;

    lock_acquire(&plist.plist_lock); /* Lock for safe traversal */

    struct list_elem *e;
    for(e = list_begin(&plist.plist_elem_list); 
        e != list_end(&plist.plist_elem_list); 
        e = list_next(e)) {

        struct plist_elem *plist_entry = list_entry(e, struct plist_elem, elem);
        if(plist_entry->pid == pid) {
            found = plist_entry;
            break;
        }
    }

    lock_release(&plist.plist_lock);
    return found;
}


void plist_remove(int pid)
{
    lock_acquire(&plist.plist_lock);

    struct list_elem *e;
    for(e = list_begin(&plist.plist_elem_list); 
        e != list_end(&plist.plist_elem_list); 
        e = list_next(e)) {

        struct plist_elem *plist_entry = list_entry(e, struct plist_elem, elem);
        if(plist_entry->pid == pid) {
            list_remove(&plist_entry->elem);
            free(plist_entry);
            break;
        }
    }

    lock_release(&plist.plist_lock);
}

void plist_for_each(void (*exec)(struct plist_elem*))
{
    lock_acquire(&plist.plist_lock);
    struct list_elem *e = list_begin(&plist.plist_elem_list);
    while (e != list_end(&plist.plist_elem_list)) {
        struct list_elem *next = list_next(e);
        struct plist_elem *plist_entry = list_entry(e, struct plist_elem, elem);

        lock_acquire(&plist_entry->lock);
        exec(plist_entry);
        lock_release(&plist_entry->lock);

        e = next;
    }

    lock_release(&plist.plist_lock);
}


void plist_set_parent_dead(int parent_tid) {
    lock_acquire(&plist.plist_lock);

    struct list_elem *e;
    for(e = list_begin(&plist.plist_elem_list); 
        e != list_end(&plist.plist_elem_list); 
        e = list_next(e)) {

        struct plist_elem *entry = list_entry(e, struct plist_elem, elem);

        lock_acquire(&entry->lock);
        if (entry->parent_id == parent_tid) {
            entry->parent_alive = false;
        }
        lock_release(&entry->lock);
    }

    lock_release(&plist.plist_lock);
}

void plist_remove_all_children(int parent_pid) {
    
    lock_acquire(&plist.plist_lock);
    
    struct list_elem *e = list_begin(&plist.plist_elem_list);
    while (e != list_end(&plist.plist_elem_list)) {
        struct list_elem *next = list_next(e);
        struct plist_elem *entry = list_entry(e, struct plist_elem, elem);
        
        lock_acquire(&entry->lock);
        bool is_child = (entry->parent_id == parent_pid);
        lock_release(&entry->lock);
        
        if (is_child) {
            /* Remove this child process */
            list_remove(&entry->elem);
            free(entry);
        }
        
        e = next;
    }
    
    lock_release(&plist.plist_lock);
}


void plist_clean_everything(void) {
    
    lock_acquire(&plist.plist_lock);
    
    struct list_elem *e = list_begin(&plist.plist_elem_list);
    while (e != list_end(&plist.plist_elem_list)) {
        struct plist_elem *entry = list_entry(e, struct plist_elem, elem);
        lock_acquire(&entry->lock);
        
        bool can_remove = !entry->alive && !entry->parent_alive;
        
        if (can_remove) {
            struct list_elem *next = list_next(e); /* Save next element before removing */
            list_remove(&entry->elem);
            lock_release(&entry->lock);
            free(entry);
            e = next;
        } else {
            lock_release(&entry->lock);
            e = list_next(e);
        }
    }
    
    lock_release(&plist.plist_lock);
}