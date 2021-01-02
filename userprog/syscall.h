#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/thread.h"	

void syscall_init (void);


struct process_file {
    struct file *file;
    int fd;
    struct list_elem elem;
};


#endif /* userprog/syscall.h */
