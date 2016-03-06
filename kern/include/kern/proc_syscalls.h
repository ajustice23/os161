#ifndef PROC_SYSCALLS_H
#define PROC_SYSCALLS_H

#include <limits.h>
#include <types.h>
#include <../../arch/mips/include/trapframe.h>

struct process_table_entry{
	struct process *proc;
	pid_t pid;
	struct process_table_entry *next;	
};

struct process{
	pid_t ppid; /*parent process id*/
	struct semaphore *exitsem;
	bool exited;
	int exitcode;
	struct thread *self;
};

struct process_table_entry *process_table;
int proc_count;

pid_t givepid(void);
void init_process(struct thread *th, pid_t id);
void destroy_process(pid_t pid);
void entrypoint(void* data1, unsigned long data2);
void changeppid(pid_t change, pid_t ppid);

void sys__exit(int exitcode);
int sys_getpid(pid_t *retval);
int sys_waitpid(pid_t pid, userptr_t status,int options, pid_t *retval);
int sys_fork(struct trapframe *trap, pid_t *retval);
//int sys_execv(const char *program, char **args);


#endif
