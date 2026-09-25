#ifndef _FLIST_H_
#define _FLIST_H_

#include "threads/synch.h"
#include "lib/kernel/list.h"
#include "filesys/file.h"
#include <stdbool.h>

/* File descriptor entry structure */
struct file_desc 
{
  int fd;                     /* File descriptor number */
  struct file *file;          /* Pointer to the file object */
  struct list_elem elem;      /* List element for thread's file list */
};

/* Save a file in the current thread's file list */
int save_fd (struct file *file);

/* Find a file given a file descriptor */
struct file *find_fd (int fd);

/* Remove a file descriptor from the list and return the file */
struct file *remove_fd (int fd);

/* Get the file size for a given file descriptor */
int filesize_fd (int fd);

/* Seek to a position in a file */
void seek_fd (int fd, unsigned position);

/* Get the current position in a file */
unsigned tell_fd (int fd);

// // DEFINITION OF MAP
// typedef struct file* value_t;
// typedef int key_t;

// struct flist {
//     struct list file_descriptor_list;
// };

// struct file_descriptor {
//     int fd;
//     struct file *file;
//     struct list_elem elem;
// };

// struct flist* get_container(void);

// /* Initializes the flist */
// void flist_init(struct flist *f);

// /* Creates a new file descriptor and adds it to the flist */
// int flist_insert(struct flist *f, struct file *file);

// /* Finds a file descriptor by its FD */
// struct file_descriptor* flist_find(struct flist *f, int fd);

// /* Removes a file descriptor by its FD */
// struct file *  flist_remove(struct flist *f, int fd);

// /* Closes all files in the flist */
// void flist_close_all(struct flist *f);

// /* Destroys the flist, freeing memory */
// void flist_destroy(struct flist *f);


// // DEFINITION OF FD FUNCTIONS
// key_t save_fd(struct file* file);

// value_t find_fd(key_t fd);

// struct file* remove_fd(key_t fd);

// int filesize_fd(key_t fd);

// void seek_fd(key_t fd, unsigned position);

// unsigned tell_fd(key_t fd);



/* Place code to keep track of your per-process open file table here.
 *
 * (The system-wide open file table exist as part of filesys/inode.c )
 *
 * User-mode code use a file by first opening it to retrieve a file
 * descriptor (integer) that uniquely identifies the open file for the
 * operation system. This file descriptor is then passed to read or
 * write to use the file, and finally to close to led the operating
 * system release any resources associated with the file.
 *
 * The kernel use a file in the same way, but use pointer to a file
 * structure to uniquely identify a file instead of an integer. If we
 * do not care for security we could pass this pointer directly to
 * user-mode code when a file is opened and expect the same pointer
 * back when the file is used in read, write or close.
 *
 * But we do care for security, we want to:
 *
 * - Hide kernel addresses and data from (untrusted) user-mode code
 *
 * - Perform validity checks that a file descriptor was indeed
 *   obtained from a call to open by the same process
 *
 * - Verify that a file descriptor was not closed
 *
 * - Make sure the kernel can close all files associated to a process
 *   as soon as it terminates
 *
 * This is best done by shielding kernel data from user code. Now the
 * kernel must keep track of which file descriptors a certain process
 * have open, and which kernel file pointer that are associated to
 * each file descriptor. This mapping is for you to solve, and the
 * data structure you need may be placed in this file.
 *
 *
 * User-mode sequence                 Kernel sequence
 * ------------------                 ---------------
 *
 * char buffer[5];                    struct file* fp;
 *
 * int   fd = open("example.txt");    fp = filesys_open(...)
 *       |                            \_________
 *       |                                      \
 *       V                                       V
 * read( fd, buffer, 5);              file_read( fp, ...)
 *       |                                       |
 *       V                                       V
 * write(fd, buffer, 5);              file_write(fp, ...)
 *       |                                       |
 *       V                                       V
 * close(fd);                         file_close(fp);
 *
 *
 * A (very) simple implementation data structure equivalent to a C++
 * std::map is recommended.
 *
 * This structure can be placed either globally or locally for each
 * process. If you go for a global map, consider how to remember which
 * process that opened each file. If you go for a local map, consider
 * where to declare and initialize it correctly. In both cases, consider
 * what size limit that may be appropriate.
 */

#endif
