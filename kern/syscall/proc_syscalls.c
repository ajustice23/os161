#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include <synch.h>
#include <kern/proc_syscalls.h>
  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

struct process_table_entry *process_table = NULL;
int proc_count;


pid_t givepid(void){
	if(process_table == NULL){
		proc_count = 1;
		process_table = (struct process_table_entry *)kmalloc(sizeof(struct process_table_entry));
		process_table->next = NULL;
		process_table->pid = 1;
		process_table->proc = (struct process *)kmalloc(sizeof(struct process));
	}

	return ++proc_count;
}

void init_process(struct thread *th, pid_t id){
	struct process *temp;
	temp = (struct process *)kmalloc(sizeof(struct process));
	temp->ppid=th->ppid;
	temp->exited=false;
	temp->exitcode=0;
	temp->self = th;
	struct semaphore *sem=  sem_create("Child",0);
	temp->exitsem = sem;
	struct process_table_entry *tmp;
	for(tmp=process_table;tmp->next!=NULL;tmp=tmp->next);
	tmp->next = (struct process_table_entry *)kmalloc(sizeof(struct process_table_entry));
	tmp->next->next=NULL;
	tmp->next->proc = temp;
	tmp->next->pid=id;
}

void destroy_process(pid_t pid) {
	struct process_table_entry * temp1, *temp2;
	for(temp1=process_table;temp1->next->pid!=pid;temp1=temp1->next);
	temp2 = temp1->next;
	temp1->next = temp1->next->next;
	kfree(temp2->proc);
	kfree(temp2);
}

void changeppid(pid_t change, pid_t ppid) {
	//process_table[change]->ppid = ppid;
	struct process_table_entry * temp1;
	for(temp1=process_table;temp1->pid!=change;temp1=temp1->next);
	temp1->proc->ppid = ppid;
}

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
  (void)exitcode;

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


int sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
  *retval = curthread->pid;
  return(0);
}

/* stub handler for waitpid() system call                */

int sys_waitpid(pid_t pid, userptr_t status, int options, pid_t *retval)
{
  int exitstatus;
  int result;

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */

  if (options != 0) {
    return(EINVAL);
  }
  /* for now, just pretend the exitstatus is 0 */
  exitstatus = 0;
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

