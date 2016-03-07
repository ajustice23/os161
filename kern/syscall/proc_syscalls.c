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
#include <array.h>
#include <mips/trapframe.h>

/* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  
 


  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
 
 //tell children they can now exit
 
  for (unsigned int i = array_num(&p->p_children); i > 0 ; i--) {

		struct proc *cp = array_get(&p->p_children,i-1);
		lock_release(cp->p_exit_lk);
		array_remove(&p->p_children,i-1);
  }
 
  //make sure children array is empty
  KASSERT(array_num(&p->p_children) == 0);
 
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
  
	
  p->p_exited = true;
  p->p_exitcode = _MKWAIT_EXIT(exitcode);

  cv_broadcast(p->p_wait_cv, p->p_wait_lk);

  lock_acquire(p->p_exit_lk);  
  lock_release(p->p_exit_lk);
  proc_destroy(p);

  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}

int
sys_getpid(pid_t *retval)
{
  *retval = curproc->pid;
  return(0);
}



int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
	kprintf("sys_waitpid called on pid:%d",pid);
 	int exitstatus;
  int result;

  struct proc *p = procarray_allprocs_proc_by_pid(pid);
	
  if(p==NULL){
		return ESRCH;
  }

  if(p == curproc){
		return ECHILD;
  }
	
  if (options != 0) {
    return(EINVAL);
  }
  
  lock_acquire(p->p_wait_lk);
  		if(!p->p_exited){
			cv_wait(p->p_wait_cv,p->p_wait_lk);
		}
	KASSERT(p->p_exited ==1);
	//lock_release(p->p_wait_lk);

	exitstatus = p->p_exitcode;

  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
   lock_release(p->p_wait_lk);
  kprintf(" sys_waitpid end\n");
  return(0);
}

int sys_fork(struct trapframe *child_tf, pid_t *retval){
	struct proc *cur_p = curproc;
	struct proc *new_p = proc_create_runprogram(cur_p->p_name);

	if(new_p==NULL){
		proc_destroy(new_p);
		return ENPROC;
	}

	//copy addrspace to new process
	
	as_copy(curproc_getas(), &(new_p->p_addrspace));
	if(new_p->p_addrspace == NULL){
		proc_destroy(new_p);
		return ENOMEM;
	}


	//make new trapframe
	struct trapframe *new_tf = kmalloc(sizeof(struct trapframe));
	if(new_tf == NULL){
		proc_destroy(new_p);
		return ENOMEM;	
	}

	memcpy(new_tf, child_tf, sizeof(struct trapframe));

	int result = thread_fork(curthread->t_name,new_p,&enter_forked_process,new_tf,0);
	if(result){
		proc_destroy(new_p);
		kfree(new_tf);
		new_tf=NULL;
		return result;
	}

	array_add(&cur_p->p_children,new_p,NULL);

	lock_acquire(new_p->p_exit_lk);

	*retval = new_p->pid;

	return 0;
}
