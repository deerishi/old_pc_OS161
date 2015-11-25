#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>

/* 
 * This simple default synchronization mechanism allows only vehicle at a time
 * into the intersection.   The intersectionSem is used as a a lock.
 * We use a semaphore rather than a lock so that this code will work even
 * before locks are implemented.
 */

/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */

/*
 * replace this with declarations of any synchronization and other variables you need here

 */
 
 //Now only allow 2 cars
 int inservice,wait_total;
 Direction va_origin,va_destination; 
 static struct lock *mutex; 
 int waiting[4]; //list of waiting vehicles
 static struct cv *CV[4];
static struct semaphore *intersectionSem;
static bool is_correct_destination;
static Direction next_direction;

bool isTurningRight(Direction origin, Direction destination);

/*bool isTurningRight(Direction origin, Direction destination)
{
	if((va_destination!=destination) && ((va_destination==va_origin+1) || (destination==origin+1)))
	{
		if((va_origin<3 && va_destination==va_origin+1)|| (origin<3 && destination==origin+1) || (origin==3 && destination==0)|| (va_origin==3 && va_destination==0))
		return true;
		
	}
	
		return false;
	
}*/

bool isTurningRight(Direction origin, Direction destination)
{
	if((va_destination!=destination) && ((va_destination==va_origin-1) || (destination==origin-1) || (origin==0 && destination==3) ||(va_origin==0 && va_destination==3)))
	{
		//if((va_origin<3 && va_destination==va_origin+1)|| (origin<3 && destination==origin+1) || (origin==3 && destination==0)|| (va_origin==3 && va_destination==0))
		return true;
		
	}
	
		return false;
	
}



bool are_constraints_satisfied(Direction origin,Direction destination);
bool are_constraints_satisfied(Direction origin,Direction destination)
{
	if((isTurningRight(origin,destination) && va_destination!=destination ) || (va_destination==origin && va_origin==destination) || va_origin==origin )
	{
		return true;
	}
	
		return false;
	
}

/* 
 * The simulation driver will call this function once before starting
 * the simulation
 *
 * You can use it to initialize synchronization and other variables.
 * 
 */
void
intersection_sync_init(void)
{
  /* replace this default implementation with your own implementation */
	
	inservice=0;
	wait_total=0;
  intersectionSem = sem_create("intersectionSem",1);
  if (intersectionSem == NULL) {
    panic("could not create intersection semaphore");
  }
  mutex=lock_create("intersection lock");
  if(mutex==NULL)
  {
  	panic("could not create intersection lock");
  }
  int i;
  for(i=0;i<4;i++)
  {
  	CV[i]=cv_create("condition varaible");
  	if(CV[i]==NULL)
  	{
  		panic("could not create condition variable");
  	}
  }
  return;
}

/* 
 * The simulation driver will call this function once after
 * the simulation has finished
 *
 * You can use it to clean up any synchronization and other variables.
 *
 */
void
intersection_sync_cleanup(void)
{
  /* replace this default implementation with your own implementation */
  KASSERT(intersectionSem != NULL);
  sem_destroy(intersectionSem);
   KASSERT(mutex != NULL);
  lock_destroy(mutex);
  int i;
  for(i=0;i<4;i++)
  {
  	 KASSERT(CV[i] != NULL);
  	 cv_destroy(CV[i]);
  }
  
}


/*
 * The simulation driver will call this function each time a vehicle
 * tries to enter the intersection, before it enters.
 * This function should cause the calling simulation thread 
 * to block until it is OK for the vehicle to enter the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle is arriving
 *     * destination: the Direction in which the vehicle is trying to go
 *
 * return value: none
 */

void
intersection_before_entry(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
 // (void)origin;  /* avoid compiler complaint about unused parameter */
  //(void)destination; /* avoid compiler complaint about unused parameter */
  //KASSERT(intersectionSem != NULL);
  //P(intersectionSem);
   
  lock_acquire(mutex);
  kprintf("\norigin, des is %d-> %d and inservice is %d inside origin,des is %d-> %d and wait_total=%d \n",origin,destination,inservice,va_origin,va_destination,wait_total);
  if(inservice==0) //ie no car has entered
  {
  	KASSERT(wait_total==0);
  	inservice++;
  	is_correct_destination=true;		
  	va_origin=origin;
  	va_destination=destination;
  	next_direction=origin+1;
  	if(next_direction>3)
  	{
  		next_direction=0;
  	}
  	kprintf("add this origin\n");
  	
  }
  else if(inservice==1) //means we have one car in the intersection
  {
   
  	if(is_correct_destination && are_constraints_satisfied(origin,destination))
  	{
  	    kprintf("inservice=1 add this origin\n");
  		inservice++;
  	}
  }
  else
  {
    kprintf("waiting this origin\n");
  	waiting[origin]++;
  	wait_total++;
  	cv_wait(CV[origin],mutex);
  }
  
  lock_release(mutex);
  
}


/*
 * The simulation driver will call this function each time a vehicle
 * leaves the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle arrived
 *    * destination: the Direction in which the vehicle is going
 *
 * return value: none
 */

void
intersection_after_exit(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
  (void)origin;  /* avoid compiler complaint about unused parameter */
  (void)destination; /* avoid compiler complaint about unused parameter */
  /*KASSERT(intersectionSem != NULL);
  V(intersectionSem);*/
  lock_acquire(mutex);
  kprintf("\nexiting origin, des is %d-> %d and inservice is %d inside origin,des is %d-> %d\n",origin,destination,inservice,va_origin,va_destination);
  inservice--;
  is_correct_destination=false;
  if(inservice==0 && wait_total>0)
  {
  	while( wait_total>0)
  	{
  		if(waiting[next_direction]==0)
  		{
  			next_direction=next_direction+1;	
  		}
  		if(next_direction >3)
  		{
  			next_direction=0;
  		}
  	
  	if(waiting[next_direction]!=0)
  	{
  	    kprintf("\nwaking %d \n",next_direction);
      	cv_broadcast(CV[next_direction],mutex);
      	wait_total=wait_total-waiting[next_direction];
    }
  }
    
  }
  lock_release(mutex);
}
