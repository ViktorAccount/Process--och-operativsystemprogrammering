#include "flist.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../threads/thread.h"
#include <stdlib.h>
#include "filesys/file.h"


/* Save a file in the current thread's file list */
int 
save_fd (struct file *file) 
{
  if (file == NULL)
    return -1;

  struct thread *t = thread_current();
  struct file_desc *fd_entry = malloc(sizeof(struct file_desc));
  
  if (fd_entry == NULL)
    return -1;
  
  /* Assign a file descriptor number 
     Start from 2 since 0 and 1 are reserved for stdin and stdout */
  fd_entry->fd = t->next_fd++;
  fd_entry->file = file;
  
  /* Add to the thread's file list */
  list_push_back(&t->file_list, &fd_entry->elem);
  
  return fd_entry->fd;
}



/* Find a file given a file descriptor */
struct file *
find_fd (int fd) 
{
  struct thread *t = thread_current();
  struct list_elem *e;
  
  /* Search the file list for the file descriptor */
  for (e = list_begin(&t->file_list); e != list_end(&t->file_list); e = list_next(e)) 
  {
    struct file_desc *fd_entry = list_entry(e, struct file_desc, elem);
    if (fd_entry->fd == fd)
      return fd_entry->file;
  }
  
  return NULL;  /* File descriptor not found */
}

/* Remove a file descriptor from the list and return the file */
struct file *
remove_fd (int fd) 
{
  struct thread *t = thread_current();
  struct list_elem *e;
  
  /* Search the file list for the file descriptor */
  for (e = list_begin(&t->file_list); e != list_end(&t->file_list); e = list_next(e)) 
  {
    struct file_desc *fd_entry = list_entry(e, struct file_desc, elem);
    if (fd_entry->fd == fd) 
    {
      struct file *file = fd_entry->file;
      
      /* Remove from the list */
      list_remove(&fd_entry->elem);
      
      /* Free the file descriptor entry but NOT the file itself */
      free(fd_entry);
      
      return file;  /* Return the file so caller can close it if needed */
    }
  }
  
  return NULL;  /* File descriptor not found */
}


/* Get the file size for a given file descriptor */
int
filesize_fd (int fd) 
{
  struct file *file = find_fd(fd);
  if (file == NULL)
    return -1;
  
  return file_length(file);
}


/* Seek to a position in a file */
void
seek_fd (int fd, unsigned position) 
{
  struct file *file = find_fd(fd);
  if (file != NULL)
    if(position <= file_length(file))
        file_seek(file, position);
    else
        file_seek(file, file_length(file)); // Seek to the end of the file if position is greater than the file size
}

/* Get the current position in a file */
unsigned
tell_fd (int fd) 
{
  struct file *file = find_fd(fd);
  if (file == NULL)
    return -1;
  
  return file_tell(file);
}

