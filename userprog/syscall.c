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

static void syscall_handler (struct intr_frame *);
void halt(void);
pid_t exec(const char *cmd_line);
int open(const char *file);
bool create(const char * file,unsigned initial_size);
int write(int fd, const void *buffer, unsigned size);
void exit(int status);
off_t fileSize(int fd);
void seek(int fd, unsigned position);

int set_file(struct file *fn);
struct file* get_file (int fd);

#define INPUT 0
#define OUTPUT 1

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}
 
/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  uint32_t *p=f->esp; 
 
  switch(*p) {
  	case SYS_HALT: // completed 21/12/2020
  		halt();// calls power off function from devices/shutdown.h
  		break;
  	case SYS_OPEN:{
			char *filename = f->esp + 1;
			printf("FILE: %c", *filename);
			f->eax = open(filename);
			}
			break;
	case SYS_WRITE:{
			int *filename = f->esp + 1;
			char *buffer = f->esp + 2;
			unsigned *size = f->esp + 3;
			f->eax = write((int)filename, (const void *)buffer,(unsigned)size);
		}
  	case SYS_EXIT: // completed 21/12/2020
  		printf(" "); // prvents error
  		int status = *((int*)f->esp + 1);// pulling status out of stack
  		exit(status);
  		break;

  	case SYS_CREATE:// completed 21/12/2020
  		printf(" "); // prvents error
  		const char * file = ((const char*)f->esp + 1); // pull file name out of stack 
  		unsigned initial_size = *((unsigned*)f->esp + 2);// pull file size out of stack
 		f->eax = create(file,initial_size);
  		break;

  	case SYS_FILESIZE: // need to create file descripter 
		printf(" ");
		int fdFileSize = *((int*)f->esp + 1);  		
  		f->eax = fileSize(fdFileSize);
  		break;
  		
  	case SYS_SEEK:
  		printf(" ");
  	    int fdSeek = *((int*)f->esp + 1);
  		unsigned position = *((unsigned*)f->esp + 2);
  		seek(fdSeek,position);
  		break;

	default:
	printf("SYS_CALL (%d) not implemented\n", *p);
	thread_exit();
	} // end of switch
}

void halt(void){
	printf("HALT WAS CALLED");
	shutdown_power_off();
}

/*This will wait for the child process (exec which will handle)*/

int open(const char *file){
	//Goes at fetches the file
	struct file *file_open = filesys_open(file); // This was sysCall_open
	//File cound not be found
	if(file_open == NULL){
		return -1;
	}else{
		//Returns the file
		return (int)file_open;
	}
	return -1;
}
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
void exit(int status){ //created By Benjamin ELl-Jones 
	struct thread* current = thread_current();// getting the current thread
  	current->exit_code = status; // Set the status of the thread to the exit code
  	thread_exit();// terminate current thread
}
bool create(const char * file,unsigned initial_size){//created By Benjamin ELl-Jones
	bool isCreated = false;
	
	if (file == NULL){
		printf("Error No fileName");
		return false;
	}else{
		isCreated = filesys_create(file, initial_size);
		return isCreated;
	}
	return isCreated;
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

off_t fileSize(int fd){//created By Benjamin ELl-Jones
	struct process_file *fileCur = get_file(fd); // gets the file
	off_t fileLength; //stores file length 
	if (fileCur != NULL){ // if file is not null
		fileLength = file_length(fileCur->file);// gets length of file
	}
	return (off_t)file_length;
}
void seek(int fd, unsigned position){//created By Benjamin ELl-Jones
	struct process_file *fileCur = get_file(fd);// gets the file using fd
	if (fileCur != NULL){ // if file is not null
		file_seek (fileCur->file, (off_t)position);
	} 
}