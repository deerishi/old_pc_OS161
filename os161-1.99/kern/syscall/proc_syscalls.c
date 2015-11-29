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
#include <mips/trapframe.h>
#include <limits.h>
#include <spl.h>
#include "opt-A2.h"

  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
    lock_acquire(lockForTable);
    
    (void)exitcode;
    p->exitCode=_MKWAIT_EXIT(exitcode);
    p->exited=__WEXITED;
    decoupleParents(p->pid); //decouple this process from its children
    if(p->parent==NULL || p->parent->pid==2) //MEANS THAT THE PARENT OF THE PROCESS HAS ALREADY EXITED THEN WE CAN SIMPLY DESTROY THE PROCESS
    {
        KASSERT(curproc->p_addrspace != NULL);
        as_deactivate();
        as = curproc_setas(NULL);
        as_destroy(as);
        cv_broadcast(p->waitcv,lockForTable);
        lock_release(lockForTable);
        proc_remthread(curthread);
        proc_destroy(p);
        thread_exit();
        panic("return from thread_exit in sys_exit\n");
    }

    
    cv_broadcast(p->waitcv,lockForTable);
    lock_release(lockForTable);
  

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
  //proc_destroy(p); dont destroy the process here, we might need it later
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
  *retval = curthread->t_proc->pid;
  //kprintf("returning %d",*retval);
  return(0);
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;
  //lock_acquire(lockForTable);
    struct proc *process=getProcessFromPid(pid); //get the process structure for the parent
    if(process->parent!=curproc) //meaning that the parent is not calling the 
    {
        //status=-1;

        return EAGAIN;
        
    }
  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */
    if(process==NULL) //that is the child process is NULL, meaning that its alraeady destroyed
    {
        
        return ESRCH;
    }
    lock_acquire(lockForTable);
        while(process->exited!=__WEXITED)
        {
        //DEBUG(DB_SYSCALL,"Syscall: _waitpid() is blocked to run process %d\n",pid);
            cv_wait(process->waitcv,lockForTable);
        }
        //return process->pid;
    lock_release(lockForTable);
  if (options != 0) {
  
    return(EINVAL);
  }
  /* for now, just pretend the exitstatus is 0 */
  exitstatus = process->exitCode;
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
          
  return(0);
}

int sys_fork(struct trapframe *tf,int32_t *retval)
{
    
     char *childName= kmalloc(sizeof(char) * NAME_MAX);
    strcpy(childName, curproc->p_name); 
    strcat(childName, "_forked");
    //kprintf("in fork\n");
    struct proc *childProcess=proc_create_runprogram(childName)   ; //Creates the process for the child
     //returkprintf(" 2 in fork\n");
    if(childProcess==NULL)
    {
        panic("Could Not Create child process\n");
    }
    struct trapframe *copy=kmalloc(sizeof(struct trapframe));
    //kprintf(" 3 in fork\n");
    childProcess->parent=curproc;
    if(copy==NULL)
    {
        kfree(childName);
        proc_destroy(childProcess);
        return ENOMEM;
    }
    //kprintf(" 4 in fork\n");
    memcpy(copy,tf,sizeof(struct trapframe));
    //kprintf("curproc pid is %d\n",curproc->pid);
     struct addrspace *addr2=kmalloc(sizeof(struct addrspace));
    if(addr2==NULL)
    {
        kfree(childName);
        kfree(copy);
        proc_destroy(childProcess);
        return ENOMEM;
        
    }
    
    int flag=as_copy(curthread->t_proc->p_addrspace,&addr2);
    
    //kprintf(" 3.3 in fork\n");
    if(flag==ENOMEM)
    {
        //Means out of memory 
        kfree(childName);
        kfree(copy);
        proc_destroy(childProcess);
        return ENOMEM;
    }
    childProcess->p_addrspace=addr2;
    int spl=splhigh();
   	//struct addrspace *old= curproc_setas(childProcess->p_addrspace);
    //old=old;
    //as_activate();
    //struct thread **ret;
    //kprintf("jijij\n");
    int res=thread_fork(childName,childProcess,(void *) enter_forked_process, copy,(unsigned long)childProcess->p_addrspace);
    if (res) {
        kfree(childName);
        kfree(copy);
        as_destroy(addr2);
        proc_destroy(childProcess);
        return ENOMEM;
    }
    //loading the success parameters
    tf->tf_v0=0;
    tf->tf_a3=0;
    splx(spl);
    *retval=childProcess->pid;
    //retpid=childProcess->pid;
    //kprintf("returning %d\n",childProcess->pid);
    return 0;

    
}

