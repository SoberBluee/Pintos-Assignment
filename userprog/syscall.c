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
bool remove (const char *file);
int read (int fd, void *buffer, unsigned size);// carlo - added 11/01/21
int write(int fd, const void *buffer, unsigned size);// carlo - added 11/01/21
void close(int fd);
void close_file(int fd);
void exit(int status);
off_t fileSize(int fd);
unsigned tell (int fd);// carlo - added 11/01/21
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

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  uint32_t *p=f->esp; 
 
  switch(*p) {
  	case SYS_HALT:{
  		halt();
  		}break;

	case SYS_EXEC:{// carlo - added 11/01/21
			char* cmd_line;
			int return_code = exec((const char*)cmd_line);

			f->eax = (uint32_t) return_code;
			
		}break;

  	case SYS_OPEN:{
  		char *filename = f->esp + 1;
		printf("FILE: %c", *filename);
		f->eax = open(filename);
  		}break;
		
  	case SYS_READ:{// carlo - added 11/01/21
		int *filename = f->esp + 1;
		char *buffer = f->esp + 2;
		unsigned *size = f->esp + 3;
		f->eax = read((int)filename, (void *)buffer,(unsigned)size);
		}break;

	case SYS_WRITE:{
		int *filename = f->esp + 1;
		char *buffer = f->esp + 2;
		unsigned *size = f->esp + 3;
		f->eax = write((int)filename, (const void *)buffer,(unsigned)size);
		}break;

  	case SYS_EXIT:{
  		int status = *((int*)f->esp + 1);// pulling status out of stack
  		exit(status);
  		}break;

  	case SYS_CREATE:{
  		const char * file = ((const char*)f->esp + 1); // pull file name out of stack 
  		unsigned initial_size = *((unsigned*)f->esp + 2);// pull file size out of stack
 		f->eax = create(file,initial_size);
  		}break;

  	case SYS_REMOVE:{// carlo - added 11/01/21
  		char* file_name = f->eax+1;
  		f->eax = remove(file_name);
	  	}break;
  	
  	case SYS_FILESIZE:{
		int fdFileSize = *((int*)f->esp + 1);		
  		f->eax = fileSize(fdFileSize);
  		}break;
  		
  	case SYS_TELL:{// carlo - added 11/01/21
  		f->eax = tell((int*)f->esp+1);
  		}break;

  	case SYS_SEEK:{
  	    int fdSeek = *((int*)f->esp + 1);
  		unsigned position = *((unsigned*)f->esp + 2);
  		seek(fdSeek,position);
  		}break;

	default:
		printf("SYS_CALL (%d) not implemented\n", *p);
		break;
	thread_exit();
	}
}

/*21/12/2020 - Ethan Donovan*/
void halt(void){
	shutdown_power_off();
}

/*runs program and returns pid*/
pid_t exec(const char *cmd_line){// carlo - added 11/01/21
	
	pid_t id = process_execute(cmd_line);
	return id;
}

//read size of bytes
int read (int fd, void *buffer, unsigned size){// carlo - added 11/01/21
	int s=0;//size of byte

	//read from keyboard using input_getc()
	if(fd == OUTPUT){
		while(s < size){
			*((char*)buffer+s)=input_getc();
			s++;
		}
		return s;
	}
	//Get file from file struct
	struct file *file_to_read = get_file(fd);
	//if no there is no file
	if(file_to_read == NULL){
		return -1;
	}
	//return file size
	s = file_read(file_to_read,buffer,size);
	return s;
}

bool remove (const char *file){// carlo - added 11/01/21
	if(file == NULL){
		return -1;
	}
	bool succes = filesys_remove(file);
	return succes;
}

// 21/12/2020 - Benjamin ELl-Jones 
void exit(int status){ /*Exit file*/
	struct thread* current = thread_current();// getting the current thread
  	current->exitCode = status; // Set the status of the thread to the exit code
  	thread_exit();// terminate current thread
}

//21/12/2020 - By Benjamin ELl-Jones
bool create(const char * file,unsigned initial_size){/*Create file*/
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

unsigned tell (int fd){// carlo - added 11/01/21
	struct file *tell_next_byte = get_file(fd);
	
	if (tell_next_byte == NULL){
		return -1;

	}
	unsigned pos_of_byte = file_tell(tell_next_byte);
	return pos_of_byte;

}


//18/12/2020 - Ethan Donovan
int open(const char *file){/*open file*/
	//Goes at fetches the file
	struct file *file_open = filesys_open(file); // This was sysCall_open
	//File cound not be found
	if(file_open == NULL){
		return -1;
	}else{
		//Returns the file
		int fd = set_file(file_open);
		return fd;	
	}
	return -1;
}

//created By Benjamin ELl-Jones
off_t fileSize(int fd){/*File size*/
	struct process_file *fileCur = get_file(fd); // gets the file
	off_t fileLength = 0; //stores file length 
	if (fileCur != NULL){ // if file is not null
		fileLength = file_length(fileCur->file);// gets length of file
	}
	return fileLength;
}


// 20/12/2020 - Ethan Donovan
int write(int fd, const void *buffer, unsigned size){/*Write File*/

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

//created By Benjamin ELl-Jones
void seek(int fd, unsigned position){
	struct process_file *fileCur = get_file(fd);// gets the file using fd
	if (fileCur != NULL){ // if file is not null
		file_seek (fileCur->file, (off_t)position);
	} 
}

/*05/01/2021 - Ethan Donovan*/
void close(int fd){/*Close file*/
	close_file(fd);
}

/* 01/01/2021 - Ethan Donovan */
/*add file to file list and return file descriptor of added file*/
int set_file(struct file *fn)
{
  struct process_file *file_to_process = malloc(sizeof(struct process_file)); //Will define the process_file struct and allocating it memory
  if (file_to_process == NULL)//file checking
  {
    return -1;
  }
  file_to_process->file = fn;//setting the filename
  file_to_process->fd = thread_current()->fd;//Defining the file descriptor
  thread_current()->fd++;//Sets the file descriptor
  list_push_back(&thread_current()->file_list, &file_to_process->elem);//Copying the file_list from the thread to the new file structure
  return file_to_process->fd;
}

/* 01/01/2021 - Ethan Donovan */
struct file* get_file (int fd){/* get file that matches file descriptor */

  struct thread *t = thread_current(); //Gets the currently running thread
  struct list_elem* next;//Will hold the next file element in a list_elem
  struct list_elem* e = list_begin(&t->file_list); //Gets the file names from the file_list and starts at the begining of the list

  for (; e != list_end(&t->file_list); e = next)  //Loop through file_list and gets each element in the list
  {	
    next = list_next(e);//sets the next element
    struct process_file *file_to_process = list_entry(e, struct process_file, elem);//Goes and fetches that file name from the process_file structure
    if (fd == file_to_process->fd)
    {
      return file_to_process->file;//Return the file
    }
  }
  return NULL; // nothing found
}

/* 04/01/2021 - Ethan Donovan */
void close_file(int fd){
	struct thread *t = thread_current();//Gets the currently running thread
	struct list_elem* next; //Will hold the next file element in a list_elem
	struct list_elem* e = list_begin(&t->file_list); //Gets the file names from the file_list and starts at the begining of the list

	for (; e != list_end(&t->file_list); e = next)//Loop through file_list and gets each element in the list
	{
		next = list_next(e);//sets the next element
		struct process_file *file_to_process = list_entry(e, struct process_file, elem);//Goes and fetches that file name from the process_file structure
		if (fd == file_to_process->fd)
		{
			file_close(file_to_process->file);//Will close that file
		}
	}
}
