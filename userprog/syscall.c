#include <stdio.h>
#include <stdlib.h>
#include "userprog/syscall.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "lib/user/syscall.h"
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"

#define INPUT 0
#define OUTPUT 1

static void syscall_handler (struct intr_frame *);
void sys_halt(void);
pid_t exec(const char *cmd_line);
int wait(pid_t pid);
int open(const char *file);
int write(int fd, const void *buffer, unsigned size);
void close(int fd);
void close_file(int fd);

int set_file(struct file *fn);
struct file* get_file (int fd);

/*  IMPORTANT
	When performing a syscall
	when a function returns a value
	you need to modify the register on the stack called uint32_t eax;
	you need to push values ? onto the stack
*/

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

//Implement a get arguments from stack function
static void
syscall_handler (struct intr_frame *f UNUSED) 
{	
	int *p = f->esp;
	switch(*p){
		case SYS_OPEN:{
			char *filename = f->esp + 1;
			printf("FILE: %c", *filename);
			f->eax = open(filename);
			}
		case SYS_WRITE:{
			int *filename = f->esp + 1;
			char *buffer = f->esp + 2;
			unsigned *size = f->esp + 3;
			f->eax = write(filename, (const void *)buffer,(unsigned)size);
		}
	
		
	}
	// wait((pid_t)p);
  	thread_exit ();
}

void halt(void){
	shutdown_power_off();
}

/*open file*/
//CREATED 18 DEC 2020
int 
open(const char *file){
	//Goes at fetches the file
	struct file *file_open = filesys_open(file);
	//File cound not be found
	if(file_open == NULL){
		return -1;
	}else{	
		//Returns the file
		return (int)file_open;
	}
	return -1;
}

//CREATED 20 DEC 2020
int write(int fd, const void *buffer, unsigned size){

	//Check if there is anything in the bugger first
	if(size <= 0){
		return size;
	}
	///Then check the FD to see what state its in
	if(fd == OUTPUT){
		putbuf(buffer, size);
		return size;
	}
	
	//Get file from file struct
	struct file *file_to_process = get_file(fd);
	//call get_file which will fetch the file from a structure 
	//call filesys_write passing in the buffer,size and filename ? 
	int written_bytes = file_write(file_to_process, buffer, size);
	return written_bytes;
}
//CREATED  05/01/2021
void close(int fd){
	close_file(fd);
}

//CREATED 1st JAN
/* add file to file list and return file descriptor of added file*/
int set_file(struct file *fn)
{
  struct process_file *file_to_process = malloc(sizeof(struct process_file));
  if (file_to_process == NULL)
  {
    return -1;
  }
  file_to_process->file = fn;
  file_to_process->fd = thread_current()->fd;
  thread_current()->fd++;
  list_push_back(&thread_current()->file_list, &file_to_process->elem);
  return file_to_process->fd;
  
}
//CREATED JAN 1st
/* get file that matches file descriptor */
struct file* get_file (int fd)
{
  struct thread *t = thread_current();
  struct list_elem* next;
  struct list_elem* e = list_begin(&t->file_list);
  
  for (; e != list_end(&t->file_list); e = next)
  {
    next = list_next(e);
    struct process_file *file_to_process = list_entry(e, struct process_file, elem);
    if (fd == file_to_process->fd)
    {
      return file_to_process->file;
    }
  }
  return NULL; // nothing found
}

void close_file(int fd){
	struct thread *t = thread_current();
	struct list_elem* next;
	struct list_elem* e = list_begin(&t->file_list);

	for (; e != list_end(&t->file_list); e = next)
	{
		next = list_next(e);
		struct process_file *file_to_process = list_entry(e, struct process_file, elem);
		if (fd == file_to_process->fd)
		{
			file_close(file_to_process->file);
		}
	}
}

