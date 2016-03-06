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

	struct process_table_entry *temp1; // for ppid
	for(temp1=process_table; temp1->pid!=curthread->ppid || temp1 == NULL; temp1=temp1->next);
	
	if(temp1==NULL){
		//do nothing if process doesn't exit 
	}else if(temp1->proc->exited == false){
		struct process_table_entry *temp2; //for pid
		for(temp2=process_table; temp2->pid!=curthread->pid || temp2==NULL; temp2=temp2->next);
		
		temp2->proc->exitcode=_MKWAIT_EXIT(exitcode);
		temp2->proc->exited=true;
		V(temp2->proc->exitsem);
	}else{
		destroy_process(curthread->pid);
	}

	thread_exit();

}


int sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
  *retval = curthread->pid;
  return(0);
}


int sys_waitpid(pid_t pid, userptr_t status, int options, pid_t *retval)
{
	int result;
	struct process *child;
	
	/*options should always be 0*/

	if(options != 0){  
		return EINVAL;
	}

	/*If there is no status pointer */
	if(status == NULL){
		return EFAULT;
	}

	/*Make sure pid is not -1 or 0 which are reserved for special cases */
	if(pid <PID_MIN){
		return EINVAL;
	}

	/*Make sure pid is not above the valid number of pids*/
	if(pid> PID_MAX){
		return ESRCH;
	}

	/*Check that pid provided isn't the current thread's pid*/
	if(curthread->pid==pid){
		return ECHILD;
	}
	
	/*Check that pid provided isn't the current thread's ppid*/
	if(curthread->ppid == pid){
		return ECHILD;
	}

	struct process_table_entry *temp1;
	for(temp1=process_table; (temp1->pid!=pid || temp1==NULL); temp1=temp1->next);

	/*Process doesn't exist*/
	if(temp1==NULL){
		return ESRCH;
	}

	/*If the proxes has not exited then use the exit semaphore for that process*/
	if(temp1->proc->exited == false){
		P(temp1->proc->exitsem);
	}

	child =temp1->proc;



  result = copyout((const void *)&(child->exitcode),status,sizeof(int));
  if (result) {
    return EFAULT;
  }
  *retval = pid;

	destroy_process(pid);
  return(0);
}

void entrypoint(void *data1, unsigned long data2){
	struct trapframe *tf = data1;
	struct trapframe tfnew;
	
	

	(void) data2;


	tfnew = *tf;
	kfree(tf);
enter_forked_process(&tfnew);
}

int sys_fork(struct trapframe *tf, pid_t *retval){
	int result;
	struct thread *child_thread;

	struct addrspace *child_addrspace;
	result = as_copy(curthread->t_addrspace, &child_addrspace); //copy parent address space to child

	if(result){
		return ENOMEM; //not enough memory to copy
	}

	struct trapframe *child_tf = kmalloc(sizeof(struct trapframe));
	
	if(child_tf==NULL){
		return ENOMEM; //not enough memory to kmalloc
	}

	//*child_tf = *tf; //copy parent's trapframe to child
	memcpy(child_tf,tf,sizeof(struct trapframe));
//	result = thread_fork1("Child Thread",
//			entrypoint
//			,(void *)child_tf
//			,child_addrspace
//			,&child_thread);
	
	result = thread_fork1("Child Thread",entrypoint,(struct trapframe *) child_tf,0 ,&child_thread);

	if(result){
		return ENOMEM; //not enough memory to fork
	}
	
	//parent gets child's pid
	*retval = child_thread->pid;
	return (0);
}
