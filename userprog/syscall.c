#include <stdio.h>
#include "userprog/syscall.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "lib/user/syscall.h"
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"


static void syscall_handler (struct intr_frame *);
void sys_halt(void);
pid_t exec(const char *cmd_line);
int wait(pid_t pid);
int open(const char *file);
// static int get_user(const uint8_t *uaddr);
// static bool put_user(uint8_t *udst, uint8_t byte);

/*IMPORTANT
	When performing a syscall
	when a function returns a value
	you need to modify the register on the stack called uint32_t eax;
	you need to push values ? onto the stack*/

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_handler (struct intr_frame *f UNUSED)
{	
	int *p = f->esp;
	switch(*p){
		case SYS_OPEN:{
			char *file = f->esp + 1;
			printf("FILE: %c", file);
			f->eax = open(file);
			}
			break;
		
	}
	// wait((pid_t)p);
  	thread_exit ();
}

// pid_t exec(const char *cmd_line){
// 	switch(cmd_line){
// 		case SYS_OPEN:
// 			const char *file = f->esp + 1;
// 			printf("FILE: %c", file);
// 			f->eax = open(file);
// 	}
// 	return 1;
// }

// int wait(pid_t pid){
// 	Will wait for child process to complete
// 	return exec((const char *)pid);
// }


void halt(void){
	printf("HALT WAS CALLED");
	shutdown_power_off();
}

/*This will wait for the child process (exec which will handle)*/


int open(const char *file){
	//Goes at fetches the file
	struct file *file_open = syscall_open(file);
	//File cound not be found
	if(file_open == NULL){
		return -1;
	}else{
		//Returns the file
		return (int)file_open;
	}
	return -1;
}

// bool check_file(const char *file){

// }