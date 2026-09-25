#include "wrap/thread.h"
#include "wrap/synch.h"
#include <stdlib.h>
#include <stdio.h>

/**
 * This file contains a basic skeleton which you can use to test your
 * readers-writers lock outside of Pintos. It is possible to use it in two ways:
 *
 * - You can run it in the visualization tool available on the course webpage.
 * - You can compile it as usual by typing "make" in the terminal, and
 *   then run "./rwlock" in your terminal.
 *
 * This example contains the "inode" struct from Pintos. As this example does
 * not do any IO, we represent the disk data as a single integer instead. This
 * is enough to trigger the appropriate warnings in the visualization tool at
 * least.
 *
 * You can use the synchronization primitives that are available in Pintos in
 * this file. They are defined in the file "wrap/synch.h" and implemented in
 * "wrap/os.c" if you are interested in how the Pintos versions are implemented
 * in terms of what is available in pthreads (which is typically used in C on
 * UNIX-like systems) and Win32 (which is what is available on Windows).
 *
 * Note: You can use NO_STEP if you have a function you don't want the
 * visualization tool to step through. See what is done to the "inode_open"
 * function below, for example.
 */

struct rw_lock {
  // Add any members you need for your readers-writers lock here.
  struct lock rw_lock;
  struct condition read_cond;
  struct condition write_cond;
  int read_count;
  int write_count;
  bool writer;
};

// Initialize the readers-writers lock.
void rw_lock_init(struct rw_lock *lock)
{
  lock_init(&lock->rw_lock);
  cond_init(&lock->read_cond);
  cond_init(&lock->write_cond);
  lock->read_count = 0;
  lock->write_count = 0;
  lock->writer = false;
}

// Acquire the readers-writers lock for reading.
void rw_lock_acquire_read(struct rw_lock *lock)
{
  lock_acquire(&lock->rw_lock);
  while (lock->write_count > 0 || lock->writer) {
    cond_wait(&lock->read_cond, &lock->rw_lock);
  }
  lock->read_count++;
  lock_release(&lock->rw_lock);
}

// Release the readers-writers lock after reading.
void rw_lock_release_read(struct rw_lock *lock)
{
  lock_acquire(&lock->rw_lock);
  lock->read_count--;
  if (lock->read_count == 0) {
    cond_broadcast(&lock->write_cond, &lock->rw_lock);
  }
  lock_release(&lock->rw_lock);
}

// Acquire the readers-writers lock for writing.
void rw_lock_acquire_write(struct rw_lock *lock)
{
  lock_acquire(&lock->rw_lock);
  lock->write_count++;
  while (lock->read_count > 0 || lock->writer) {
    cond_wait(&lock->write_cond, &lock->rw_lock);
  }
  lock->write_count--;
  lock->writer = true;
  lock_release(&lock->rw_lock);
}


// Release the readers-writers lock after writing.
void rw_lock_release_write(struct rw_lock *lock)
{
  lock_acquire(&lock->rw_lock);
  lock->writer = false;
  cond_broadcast(&lock->read_cond, &lock->rw_lock);
  cond_broadcast(&lock->write_cond, &lock->rw_lock);
  lock_release(&lock->rw_lock);
}



/**
 * This struct corresponds to the inode struct inside Pintos.
 */
struct inode
{
  // Our representation of the data on disk - a single integer.
  int data;
  struct rw_lock lock; // The readers-writers lock for this inode.
  // Add any other members you need for your readers-writers lock here.
};

// Create an inode. We don't need a sector in this implementation.
struct inode *inode_open(void) NO_STEP
{

  struct inode *node = malloc(sizeof(struct inode));

  node->data = 0;
  rw_lock_init(&node->lock);
  // Any other initialization...

  return node;
}

// Close (free) an inode.
void inode_close(struct inode *inode)
{
  // We don't have an "open count" in this implementation,
  // so we can just free it.
  free(inode);
}

// Read data from the inode. Since we don't do actual disk IO, we don't need to
// take an offset into consideration.
void inode_read_at(struct inode *inode, int *output)
{

  // This is the simplified version of the code that reads data in the
  // implementation in Pintos. This is enough to trigger appropriate messages in
  // the visualization tool. Add your rw-lock before and after.
  rw_lock_acquire_read(&inode->lock);
  *output = inode->data;
  rw_lock_release_read(&inode->lock);
}

// Write data to the inode. Since we don't do actual disk IO, we don't need to
// take an offset into consideration.
void inode_write_at(struct inode *inode, int *input)
{

  // This is the simplified version of the code that writes data in the
  // implementation in Pintos. This is enough to trigger appropriate messages in
  // the visualization tool. Add your rw-lock before and after.
  rw_lock_acquire_write(&inode->lock);
  inode->data = *input;
  rw_lock_release_write(&inode->lock);

}


/**
 * The code below here is a simple program that spawns a number of reader and
 * writer threads so that you can test your implementation. Feel free to modify
 * this part so that you can examine the cases you want to explore.
 */

void reader_thread(struct inode *inode)
{
  int data;
  for (int i = 1; i <= 2; i++)
    inode_read_at(inode, &data);
}

void writer_thread(struct inode *inode)
{
  for (int i = 1; i <= 2; i++)
    inode_write_at(inode, &i);
}

int main(void) {
  struct inode *inode = inode_open();

  // Spawn threads. Note: "thread_new" works more or less like "thread_create",
  // but you don't need to specify name and priority. "thread_create" does not
  // work in the visualization tool, but "thread_new" does.

  thread_new(&writer_thread, inode);
  // thread_new(&writer_thread, inode);
  thread_new(&reader_thread, inode);
  thread_new(&reader_thread, inode);

  return 0;
}
