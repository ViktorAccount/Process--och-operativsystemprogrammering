#include <stdio.h>
#include <syscall-nr.h>
#include "userprog/syscall.h"
#include "threads/interrupt.h"
#include "threads/thread.h"

/* header files you probably need, they are not used yet */
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "devices/input.h"
#include "devices/timer.h"

#include "../lib/stdio.h" // Hard coded path for stdio.h used to not gett errors on STDIN_FILENO and STDOUT_FILENO
#include "flist.h"
static void syscall_handler (struct intr_frame *);



/* Verify all addresses from and including 'start' up to but excluding
 * (start+length). */
bool verify_fix_length(void* start, unsigned length)
{
  if (start == NULL || is_kernel_vaddr(start))
    return false;

  void* end = (void*)((char*)start + length-1); // Getting the end addressw
  
  // Check if the end address is above the physical base, because if its over it is in the space reserver for the kernel
  if(end >= (void*)PHYS_BASE) 
    return false;
  
  void* start_page = pg_round_down(start); // Start page
  void* end_page = pg_round_down(end); // End page

  // Loops trough all the pages from start to end and checks if they are mapped, because the addresses checked can be in different pages.
  for (char* page = (char*)start_page; page <= (char*)end_page; page += PGSIZE)
  {
    // Returns false if one of the pages is not valid, if the page is valid it means that all the addresses are valid
    if (pagedir_get_page(thread_current()->pagedir, (void*)page) == NULL) {  
      return false;
    }
  }
  return true;
}

/* Verify all addresses from and including 'start' up to and including
 * the address first containg a null-character ('\0'). (The way
 * C-strings are stored.)
 */
bool verify_variable_length(char* start)
{
  if (start == NULL || is_kernel_vaddr(start))
    return false;

  if(!pagedir_get_page(thread_current()->pagedir, start)) 
    return false;

  while (*start != '\0')
  {
    if(pg_no(start) != pg_no(start+1))
      if(is_kernel_vaddr(start+1))
        return false;

      if(pagedir_get_page(thread_current()->pagedir, start+1) == NULL)
        return false;
    start++;
  }
  return true;
}


void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


/* This array defined the number of arguments each syscall expects.
   For example, if you want to find out the number of arguments for
   the read system call you shall write:

   int sys_read_arg_count = argc[ SYS_READ ];

   All system calls have a name such as SYS_READ defined as an enum
   type, see `lib/syscall-nr.h'. Use them instead of numbers.
 */
const int argc[] = {
  /* basic calls */
  0, 1, 1, 1, 2, 1, 1, 1, 3, 3, 2, 1, 1,
  /* not implemented */
  2, 1,    1, 1, 2, 1, 1,
  /* extended, you may need to change the order of these two (plist, sleep) */
  0, 1
};

static void
syscall_handler (struct intr_frame *f)
{
  int32_t* esp = (int32_t*)f->esp;
  
  /*
  Validate the user stack pointer (must point to a readable user address)
  Ensures we're not reading from kernel memory or NULL. 
  */
  if (!verify_fix_length(f->esp, 4)) {
    process_exit(-1);
    thread_exit();
  }
  
  /*
  Check if the syscall number is within valid bounds
  Prevents undefined syscall numbers from being used (which could crash kernel) 
  */
 if (*esp > SYS_NUMBER_OF_CALLS){
   process_exit(-1);
   thread_exit();
  }
  
  /*
  Chekc if the stack contains enoguh space for all arguments
  We verify that the stack contains at least that many 4-byte arguments
  */
 if (!verify_fix_length(esp, (argc[*esp] + 1) * 4)){
   process_exit(-1);
   thread_exit();
  }
  
  int syscall_number = esp[0];

  switch ( syscall_number )
  {
    case SYS_HALT:
    {
      power_off();
      break;
    }

    case SYS_EXIT:
    {
      int status = esp[1];
      process_exit(status);
      thread_exit();
      break;
    }

    case SYS_EXEC:
    {
      const char* cmd_line = (const char*)esp[1];

      // Verify cmd_line is a valid string
      if (!verify_variable_length((char*)cmd_line)) {
        process_exit(-1);
        thread_exit();
      }

      f->eax = process_execute(cmd_line);
      break;
    }

    case SYS_READ:
    {
      int fd = esp[1];
      void *buffer = (void *)esp[2]; 
      unsigned size = (unsigned)esp[3];
      
      // Verify buffer is valid and writable
      if (!verify_fix_length(buffer, size)) {
        process_exit(-1);
        thread_exit();
      }


      if (fd == STDIN_FILENO){
        char *buf = (char *)buffer; 
        unsigned i = 0;
        while(i<size){
          char c = input_getc();
          if (c == '\r'){
            c = '\n';
          } 
          buf[i++] = c; // Store the character
          
          putchar(c);
          if (c == '\n') {  // Stop reading on Enter key
            break;
          }
          
        }
        f->eax = i;
      }
      else if(fd > 1){
        struct file* file = find_fd(fd);
        if(file == NULL){
          f->eax = -1;
        }
        else {
          off_t bytes = file_read(file, buffer, size);
          f->eax = bytes;
        }
      }
      else{
        f->eax = -1;
      }
      
      break;
    }

    case SYS_WRITE:
    {
      int fd = esp[1];
      void *buffer = (void *)esp[2];
      unsigned size = (unsigned)esp[3];

      // Verify buffer is valid and readable
      if (!verify_fix_length(buffer, size)) {
        process_exit(-1);
        thread_exit();
      }

      if(fd == STDOUT_FILENO) {
        putbuf(buffer, size);
        f->eax = size;
      } else if(fd > 1) {
        struct file *file = find_fd(fd);
        if(file == NULL) {
          f->eax = -1;
        } else {
          int bytes_written = file_write(file, buffer, size);
          f->eax = bytes_written;
        }
      }
      else{
        f->eax = -1;
      }
      break;
    }

    case SYS_CREATE:
    {
      const char* file_name = (char *)esp[1];
      unsigned initial_size = (unsigned)esp[2];

      // Verify file_name is valid
      if (!verify_variable_length((char*)file_name)) {
        process_exit(-1);
        thread_exit();
      }

      f->eax = filesys_create(file_name, initial_size);
      break;
    }

    case SYS_OPEN:
    {
      const char* file_name = (char *)esp[1];

      // Verify file_name is valid
      if (!verify_variable_length((char*)file_name)) {
        process_exit(-1);
        thread_exit();
      }

      struct file* file = filesys_open(file_name);
      if(file == NULL){
        f->eax = -1;
        break;
      }

      int result = save_fd(file);
      if(result == -1){
        filesys_close(file);
      }
      f->eax = result;
      break;
    }

    case SYS_REMOVE:
    {
      const char* file_name = (char *)esp[1];

      // Verify file_name is valid
      if (!verify_variable_length((char*)file_name)) {
        process_exit(-1);
        thread_exit();
      }

      f->eax = filesys_remove(file_name);
      break;
    }

    case SYS_CLOSE:
    {
      int fd = esp[1];
      struct file* file = remove_fd(fd);
      if(file != NULL){
        filesys_close(file);
      }
      break;
    }
    
    case SYS_FILESIZE:
    {
      int fd = esp[1];
      f->eax = filesize_fd(fd);
      break;
    }
    
    case SYS_SEEK:
    {
      int fd = esp[1];
      unsigned position = (unsigned)esp[2];
      seek_fd(fd, position);
      break;
    }
    
    case SYS_TELL:
    {
      int fd = esp[1];
      f->eax = tell_fd(fd);
      break;
    }

    case SYS_PLIST:
    {
      process_print_list();
      break;
    }

    case SYS_SLEEP:
    {
      int millis = esp[1];
      timer_msleep(millis);
      break;
    }

    case SYS_WAIT:
    {
      int pid = esp[1];
      f->eax = process_wait(pid);
      break;
    }
    
    default:
    {
      process_exit(-1);
      thread_exit();
    }
  }
}
