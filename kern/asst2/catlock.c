/*
 * catlock.c
 *
 * Please use LOCKS/CV'S to solve the cat syncronization problem in 
 * this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include "catmouse.h"
#include "synch.h"
#include <machine/spl.h>

/*
 * 
 * Function Definitions
 * 
 */

/*
 * catlock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS -
 *      1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using locks/cv's.
 *
 */

int turn = 0; // 0 for cats and 1 for mice
int cat_num = 0; // Number of cats in the area
int mouse_num = 0; // Number of mice in the area

// Variables for dishes, 0 means available and 1 means busy
int dish_1 = 0;
int dish_2 = 0;

struct lock * entry_exit_lock;
struct lock * cat_num_lock;
struct lock * dish_lock;
struct lock * dish_lock_1;
struct lock * dish_lock_2;

char sleep_addr = 'I';

static
void
catlock(void * unusedpointer, 
        unsigned long catnumber)
{
        //kprintf("Cat number %lu entered\n", catnumber);
        int iter;
        for(iter = 0; iter < NMEALS; iter++){
                while(1){

                        // Wait till the area is free for cats or cats finish eating
                        while(turn == 1 || cat_num > 1){
                                //kprintf("I am just in this inner most while loop t:%d n:%d\n", turn, cat_num);
                                thread_yield();
                        }

                        // This is for competing cats, mice won't compete here
                        lock_acquire(entry_exit_lock);
                        int repeat = 0;
                        if (cat_num < 2 && turn == 0){
                                cat_num += 1;
                        }
                        else{
                                repeat = 1;
                        }
                        lock_release(entry_exit_lock);

                        // If we succeeded in entering we move forward else repeat
                        if (repeat){
                                //kprintf("Ah we go back to inf while loop. Cat num: %d\n", cat_num);
                                continue;
                        }

                        // Now check which dish is available
                        lock_acquire(dish_lock);
                        if (dish_1 == 0){
                                lock_acquire(dish_lock_1);
                                dish_1 = 1;
                                lock_release(dish_lock);
                                const char* name = "cat";
                                catmouse_eat(name, catnumber, 1, iter);
                                dish_1 = 0;
                                lock_release(dish_lock_1);
                        }
                        else if(dish_2 == 0){
                                lock_acquire(dish_lock_2);
                                dish_2 = 1;
                                lock_release(dish_lock);
                                const char* name = "cat";
                                catmouse_eat(name, catnumber, 2, iter);
                                dish_2 = 0;
                                lock_release(dish_lock_2);
                        }
                        else{
                                lock_release(dish_lock);
                                kprintf("This should not happen! Atleast one dish should be empty.\n");
                                //continue;
                        }

                        // This is the exit procedure for cats
                        lock_acquire(entry_exit_lock);
                        cat_num -= 1;

                        // The last cat is exiting so the mice can now enter
                        if (cat_num == 0){
                                turn = 1;
                        }

                        lock_release(entry_exit_lock);
                        break;
                }
        }

        (void) unusedpointer;
        (void) catnumber;
}
	

/*
 * mouselock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using locks/cv's.
 *
 */

static
void
mouselock(void * unusedpointer,
          unsigned long mousenumber)
{
        int iter;
        for(iter = 0; iter < NMEALS; iter++){
                while(1){
                        while(turn == 0){
                                thread_yield();
                        }

                        lock_acquire(entry_exit_lock);
                        int repeat = 0;
                        if (turn == 1){
                                mouse_num += 1;
                        }
                        else{
                                repeat = 1;
                        }
                        lock_release(entry_exit_lock);

                        // If we succeeded in entering we move forward else repeat
                        if (repeat){
                                continue;
                        }

                        lock_acquire(dish_lock);
                        if (dish_1 == 0){
                                lock_acquire(dish_lock_1);
                                dish_1 = 1;
                                lock_release(dish_lock);
                                const char* name = "mouse";
                                catmouse_eat(name, mousenumber, 1, iter);
                                dish_1 = 0;
                                lock_release(dish_lock_1);
                        }
                        else if(dish_2 == 0){
                                lock_acquire(dish_lock_2);
                                dish_2 = 1;
                                lock_release(dish_lock);
                                const char* name = "mouse";
                                catmouse_eat(name, mousenumber, 2, iter);
                                dish_2 = 0;
                                lock_release(dish_lock_2);
                        }
                        else{
                                lock_release(dish_lock);
                                kprintf("This should not happen! Atleast one dish should be empty.\n");
                                //continue;
                        }

                        // This is the exit procedure for mice
                        lock_acquire(entry_exit_lock);
                        mouse_num -= 1;

                        // The last cat is exiting so the mice can now enter
                        if (mouse_num == 0){
                                turn = 0;
                        }

                        lock_release(entry_exit_lock);
                        break;
                }
        }
        
        (void) unusedpointer;
        (void) mousenumber;
}


/*
 * catmouselock()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catlock() and mouselock() threads.  Change
 *      this code as necessary for your solution.
 */

int
catmouselock(int nargs,
             char ** args)
{
        int index, error;
   
        /*
         * Start NCATS catlock() threads.
         */

        entry_exit_lock = lock_create("entry_exit_lock");
        cat_num_lock = lock_create("cat_num_lock");
        dish_lock = lock_create("dish_lock");
        dish_lock_1 = lock_create("dish_lock_1");
        dish_lock_2 = lock_create("dish_lock_2");

        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catlock thread", 
                                    NULL, 
                                    index, 
                                    catlock, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catlock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        /*
         * Start NMICE mouselock() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mouselock thread", 
                                    NULL, 
                                    index, 
                                    mouselock, 
                                    NULL
                                    );
      
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mouselock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }
        
        /*
         * wait until all other threads finish
         */
        
        while (thread_count() > 1)
                thread_yield();

        (void)nargs;
        (void)args;
        kprintf("catlock test done\n");

        return 0;
}

