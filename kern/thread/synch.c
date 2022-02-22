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
// int test_and_set(volatile int* old_ptr, int new_val){
// 	int spl;
// 	spl = splhigh();

// 	int old_val = *old_ptr;
// 	*old_ptr = new_val;
	
// 	splx(spl);
	
// 	return old_val;
// }

struct lock *
lock_create(const char *name)
{
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

	lock->owner = kmalloc(1000);
	if (lock->owner == NULL){
		return NULL;
	}
	
	// No one holds the lock initially
	lock->flag = 0;
	lock->owner = kstrdup("no_owner");
	
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
	assert(lock->flag == 0);
	splx(spl);

	// Free the lock
	kfree(lock->owner);
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

	int spl;
	spl = splhigh();

	while (lock->flag == 1){
		splx(spl);
		thread_yield();
		spl = splhigh();
	}

	kprintf("Lock acquired\n");
	
	lock->owner = kstrdup(curthread->t_name);
	lock->flag = 1;
	//kprintf("\nI captured the lock %s\n", lock->name);

	splx(spl);

	// Return back
}

void
lock_release(struct lock *lock)
{
	// Write this
	int spl;
	spl = splhigh();

	//kprintf("I released the lock %s\n", lock->name);
	lock->flag = 0;
	lock->owner = kstrdup("no_owner");

	kprintf("Lock_release\n");

	splx(spl);
}

int
lock_do_i_hold(struct lock *lock)
{
	int spl;
	spl = splhigh();
	if(strcmp(lock->owner, curthread->t_name) == 0){
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
	struct cv* cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name==NULL) {
		kfree(cv);
		return NULL;
	}

	// No one is resting initially
	cv->num_of_threads = 0;
	cv->waiting_line = q_create(1); // Initial size is 10 threads
	
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	int spl;
	assert(cv != NULL);

	spl = splhigh();
	assert(cv->num_of_threads==0);
	splx(spl);

	// add stuff here as needed
	q_destroy(cv->waiting_line);
	kfree(cv->name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	// If the thread does not hold the lock return
	int spl;
	spl = splhigh();

	if (!lock_do_i_hold(lock)){
		splx(spl);
		return;
	}

	if(q_addtail(cv->waiting_line, curthread->t_name) != 0){
		assert("Problem in adding element to wait queue in cv.");
	}
	cv->num_of_threads += 1;

	lock_release(lock);
	thread_sleep(curthread->t_name);
	lock_acquire(lock);

	splx(spl);
	return;
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	int spl;
	spl = splhigh();
	if (!lock_do_i_hold(lock)){
		splx(spl);
		return;
	}
	void* to_wakeup = q_remhead(cv->waiting_line);
	cv->num_of_threads--;

	thread_wakeup(to_wakeup);
	splx(spl);
	return;
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	int spl;
	spl = splhigh();
	if (!lock_do_i_hold(lock)){
		splx(spl);
		return;
	}
	
	int i; 
	for (i = q_getstart(cv->waiting_line); i != q_getend(cv->waiting_line); i = (i+1)%q_getsize(cv->waiting_line)) {
		void *ptr = q_getguy(cv->waiting_line, i);
		kprintf("i: %d\n", i);
		thread_wakeup(ptr);
	}

	int j = cv->num_of_threads;
	while (j > 1){
		kprintf("Size: %d \n", j);
		q_remhead(cv->waiting_line);
		j--;
	}

	kprintf("Here_too\n");
	splx(spl);
}
