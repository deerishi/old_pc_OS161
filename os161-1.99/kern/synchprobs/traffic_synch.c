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
statc bool is_correct_destination;

bool isTurningRight(Direction origin, Direction destination)
{
	if(()va_destination!=destination) && (va\\))
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
 *    * destination: the Direction in which the vehicle is trying to go
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
  KASSERT(intersectionSem != NULL);
  V(intersectionSem);
}
