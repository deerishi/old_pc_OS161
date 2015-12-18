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
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <spl.h>
#include <vm.h>
#include <vfs.h>
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
    if (res) 
    {
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

int sys_execv(userptr_t progname, userptr_t args)
{
    args=args;
    struct addrspace *as;
	struct vnode *v;
	char *Progname=(char *)progname;
	vaddr_t entrypoint, stackptr;
	int result;
    char *Kbuf[14];
    char **kargs=(char **)args;
    //char **kbuf;
    int i=0,length,space=0,res;
    //copying all the arguments in to the kernel buffer
    char *str=kmalloc(sizeof(char)*256);
    int arrlength=0;
    while(kargs[i]!=NULL)
    {
        length=strlen(kargs[i])+1;
        arrlength++;
        if(length%4 !=0 )
        {
            length+= 4 - (length%4);
        }
        space+=length;
        /*kbuf[i]=kmalloc(length);
        if(kbuf[i]==NULL)
        {
            return ENOMEM;
        }*/
        size_t strlength=strlen(kargs[i]);
        Kbuf[i]=kmalloc(sizeof(char)*256);
        res=copyinstr((userptr_t)kargs[i],str,(size_t)length,&strlength);
        Kbuf[i]=str;
        if(res)
        {
             return(res);
        }   
        i++; 
    }
    Kbuf[i]=NULL;
    //
	/* Open the file. */
	result = vfs_open(Progname, O_RDONLY, 0, &v);
	if (result) 
	{
		return result;
	}

	
	/* Create a new address space. */
	as = as_create();
	if (as ==NULL) 
	{
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	curproc_setas(as);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) 
	{
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	DEBUG(DB_SYSCALL,"stack addresses are 0x%x and 0x%x\n",stackptr,USERSTACK);
	if (result) 
	{
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}
	int j;
	//stackptr=stackptr-i-(arrlength*sizeof);	
    for(j=0;j<=i;j++)
    {
        //char *arg=Kbuf[i];

        result = copyout((void *)&Kbuf[i],(userptr_t)stackptr,(sizeof(Kbuf)));
                DEBUG(DB_SYSCALL,"kbuf addresses are 0x%x \n",stackptr);
        stackptr=stackptr-4;
    }
    if (result) 
	{
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}
	/* Warp to user mode. */
	enter_new_process(arrlength/*argc*/,(userptr_t) Kbuf /*userspace addr of argv*/,stackptr, entrypoint);
	
	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;    
}
