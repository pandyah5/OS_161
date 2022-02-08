/*
 * Synchronization primitives.
 * See synch.h for specifications of the functions.
 */

#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>
#include <queue.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *namearg, int initial_count)
{
	struct semaphore *sem;

	assert(initial_count >= 0);

	sem = kmalloc(sizeof(struct semaphore));
	if (sem == NULL) {
		return NULL;
	}

	sem->name = kstrdup(namearg);
	if (sem->name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->count = initial_count;
	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	spl = splhigh();
	assert(thread_hassleepers(sem)==0);
	splx(spl);

	/*
	 * Note: while someone could theoretically start sleeping on
	 * the semaphore after the above test but before we free it,
	 * if they're going to do that, they can just as easily wait
	 * a bit and start sleeping on the semaphore after it's been
	 * freed. Consequently, there's not a whole lot of point in 
	 * including the kfrees in the splhigh block, so we don't.
	 */

	kfree(sem->name);
	kfree(sem);
}

void 
P(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	assert(in_interrupt==0);

	spl = splhigh();
	while (sem->count==0) {
		thread_sleep(sem);
	}
	assert(sem->count>0);
	sem->count--;
	splx(spl);
}

void
V(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count>0);
	thread_wakeup(sem);
	splx(spl);
}

////////////////////////////////////////////////////////////
//
// Lock.

// Atomic instruction used for locking
int test_and_set(volatile int* old_ptr, int new_val, char* owner){
	int spl;
	spl = splhigh();

	int old_val = *old_ptr;
	*old_ptr = new_val;

	// Set the name to identify which thread owns it
	if (old_val == 0){
		owner = kstrdup(curthread->t_name);
	}

	splx(spl);

	return old_val;
}

struct lock *
lock_create(const char *name)
{
	// Allocate memory
	struct lock *lock;

	lock = kmalloc(sizeof(struct lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->name = kstrdup(name);
	if (lock->name == NULL) {
		kfree(lock);
		return NULL;
	}

	// No one holds the lock initially
	lock->flag = 0;
	
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	// Check if lock exists
	assert(lock != NULL);

	// Check if someone is holding the lock
	int spl;
	spl = splhigh();
	assert(lock->flag == 0)
	splx(spl);

	// Free the lock
	kfree(lock->name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
	// Check if lock is NULL
	assert(lock!=NULL);

	// Check if we are in an interrupt handler
	assert(in_interrupt==0);

	// Check if lock is held, if yes then spin, if no then book it
	while (test_and_set(&(lock->flag), 1, lock->name) == 1){
		;
	}

	// Return back
}

void
lock_release(struct lock *lock)
{
	// Write this
	if (lock_do_i_hold(lock) == 1){
		lock->flag = 0;
	}
}

int
lock_do_i_hold(struct lock *lock)
{
	// Block interrupts
	int spl;
	spl = splhigh();

	if(strcmp(lock->name, curthread->t_name) == 0 && lock->flag == 1){
		splx(spl);
		return 1;
	}
	else{
		splx(spl);
		return 0;
	}
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	// Allocate memory
	struct cv *cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name==NULL) {
		kfree(cv);
		return NULL;
	}

	// Create a waiting queue for sleeping threads
	cv->waiting_line = q_create(10);

	if(cv->waiting_line == NULL){
		kfree(cv);
		return NULL;
	}

	// Intially no thread sleeps on cv
	cv->num_of_threads = 0;
	
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	// Block interrupts, check is cv is empty
	int spl;
	spl = splhigh();
	assert(cv->num_of_threads == 0);
	splx(spl);
	
	kfree(cv->name);
	q_destroy(cv->waiting_line);
	kfree(cv->waiting_line);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{	
	// Check if you hold the lock
	if(lock_do_i_hold(lock) == 1){
		// Wrap in a no interrupt segment
		int spl;
		spl = splhigh();

		// Check if there is enough space in queue, if not make some
		q_preallocate(cv->waiting_line, cv->num_of_threads + 1);

		// Add the current thread to the queue
		q_addtail(cv->waiting_line, curthread);

		// Add one to the number of threads put to sleep
		cv->num_of_threads += 1;

		// Release the lock
		lock_release(lock);

		// Send the thread to sleep
		thread_sleep(curthread);

		// Acquire the lock again
		lock_acquire(lock);

		splx(spl);
	}
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	// We are already hold a lock so we are atomic

	struct thread* wake_up;

	if(lock_do_i_hold(lock) == 1){

		// Wrap in a no interrupt segment
		int spl;
		spl = splhigh();

		// Get the head of the queue
		int head_index = q_getstart(cv->waiting_line);

		// Check if head is valid
		if (head_index > cv->num_of_threads){
			kprintf("ERROR: Accesing elements beyond the valid elements in queue.\n");
		}

		// Get the thread pointed to by the head
		wake_up = q_getguy(cv->waiting_line, head_index);

		// Remove the head from the queue
		q_remhead(cv->waiting_line);

		// Decrement number of threads on sleep
		cv->num_of_threads -= 1;

		// Wake up the thread
		thread_wakeup(wake_up);

		splx(spl);
	}
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	if(lock_do_i_hold(lock) == 1){
		// Wrap in a no interrupt segment
		int spl;
		spl = splhigh();

		int i;
		struct thread * ptr;
		
		while(cv->num_of_threads != 0) {
			i = q_getstart(cv->waiting_line);
			ptr = q_getguy(cv->waiting_line, i);

			// Remove the head from the queue
			q_remhead(cv->waiting_line);

			// Decrement number of threads on sleep
			cv->num_of_threads -= 1;

			// Wake up the thread
			thread_wakeup(ptr);
		}

		// Reset number of threads on sleep
		assert(cv->num_of_threads == 0);

		splx(spl);
	}
}
