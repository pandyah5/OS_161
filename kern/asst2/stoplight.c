/* 
 * stoplight.c
 *
 * You can use any synchronization primitives available to solve
 * the stoplight problem in this file.
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
#include "synch.h"


/*
 * Number of cars created.
 */

#define NCARS 20

// Locks required for the problem
struct lock * NW_lock;
struct lock * SW_lock;
struct lock * NE_lock;
struct lock * SE_lock;
struct lock * NUM_CAR_LOCK;
struct lock * north_road;
struct lock * south_road;
struct lock * east_road;
struct lock * west_road;


// Global variables needed for this problem
int NW = 0; // 0 means available
int SW = 0; // 0 means available
int NE = 0; // 0 means available
int SE = 0; // 0 means available
int num_cars = 0; // Number of cars in intersection (should not exceed 3)

/*
 *
 * Function Definitions
 *
 */

static const char *directions[] = { "N", "E", "S", "W" };

static const char *msgs[] = {
        "approaching:",
        "region1:    ",
        "region2:    ",
        "region3:    ",
        "leaving:    "
};

/* use these constants for the first parameter of message */
enum { APPROACHING, REGION1, REGION2, REGION3, LEAVING };

static void
message(int msg_nr, int carnumber, int cardirection, int destdirection)
{
        kprintf("%s car = %2d, direction = %s, destination = %s\n",
                msgs[msg_nr], carnumber,
                directions[cardirection], directions[destdirection]);
}
 
/*
 * gostraight()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement passing straight through the
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
gostraight(unsigned long cardirection,
           unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */
        while(1){
                while(num_cars > 2){
                        thread_yield();
                }
                int repeat = 0;
                lock_acquire(NUM_CAR_LOCK);
                if (num_cars > 2){
                        repeat = 1;
                }
                else{
                        num_cars += 1;
                }
                lock_release(NUM_CAR_LOCK);

                if (repeat){
                        continue;
                }

                // Decide which region to acquire next
                if (cardirection == 0){
                        // We are at north and need to reach south

                        // Step 1: Acquire NW region
                        lock_acquire(NW_lock);
                        NW = 1;
                        message(REGION1, carnumber, cardirection, 2);

                        // Step 2: Acquire SW region and release NW region
                        lock_acquire(SW_lock);
                        NW = 0;
                        SW = 1;
                        message(REGION2, carnumber, cardirection, 2);
                        lock_release(NW_lock);

                        // Step 3: Leave
                        message(LEAVING, carnumber, cardirection, 2);
                        SW = 0;
                        lock_release(SW_lock);
                }
                else if (cardirection == 1){
                        // We are at east and need to go to west

                        // Step 1: Acquire NE region
                        lock_acquire(NE_lock);
                        NE = 1;
                        message(REGION1, carnumber, cardirection, 3);

                        // Step 2: Acquire the NW region and leave NE region
                        lock_acquire(NW_lock);
                        NE = 0;
                        NW = 1;
                        message(REGION2, carnumber, cardirection, 3);
                        lock_release(NE_lock);

                        // Step 3: Leave
                        message(LEAVING, carnumber, cardirection, 3);
                        NW = 0;
                        lock_release(NW_lock);
                }
                else if (cardirection == 2){
                        // We are at south going to the north
                        lock_acquire(SE_lock);
                        SE = 1;
                        message(REGION1, carnumber, cardirection, 0);

                        lock_acquire(NE_lock);
                        SE = 0;
                        NE = 1;
                        message(REGION2, carnumber, cardirection, 0);
                        lock_release(SE_lock);

                        message(LEAVING, carnumber, cardirection, 0);
                        NE = 0;
                        lock_release(NE_lock);
                }
                else if (cardirection == 3){
                        // We are at west and going to the east
                        lock_acquire(SW_lock);
                        SW = 1;
                        message(REGION1, carnumber, cardirection, 1);

                        lock_acquire(SE_lock);
                        SW = 0;
                        SE = 1;
                        message(REGION2, carnumber, cardirection, 1);
                        lock_release(SW_lock);

                        message(LEAVING, carnumber, cardirection, 1);
                        SE = 0;
                        lock_release(SE_lock);
                }
                else{
                        kprintf("Why is cardirection not in range (0-3).\n");
                        continue;
                }

                lock_acquire(NUM_CAR_LOCK);
                num_cars -= 1;
                lock_release(NUM_CAR_LOCK);
                break;
        }
}


/*
 * turnleft()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a left turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnleft(unsigned long cardirection,
         unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */
        while(1){
                while(num_cars > 2){
                        thread_yield();
                }
                int repeat = 0;
                lock_acquire(NUM_CAR_LOCK);
                if (num_cars > 2){
                        repeat = 1;
                }
                else{
                        num_cars += 1;
                }
                lock_release(NUM_CAR_LOCK);

                if (repeat){
                        continue;
                }

                // Decide which direction to go next
                if (cardirection == 0){
                        // We are at north and need to go to the east

                        lock_acquire(NW_lock);
                        NW = 1;
                        message(REGION1, carnumber, cardirection, 1);

                        lock_acquire(SW_lock);
                        NW = 0;
                        SW = 1;
                        message(REGION2, carnumber, cardirection, 1);
                        lock_release(NW_lock);

                        lock_acquire(SE_lock);
                        SW = 0;
                        SE = 1;
                        message(REGION3, carnumber, cardirection, 1);
                        lock_release(SW_lock);

                        message(LEAVING, carnumber, cardirection, 1);
                        SE = 0;
                        lock_release(SE_lock);
                }
                else if (cardirection == 1){
                        // We are at east and need to go to south
                        lock_acquire(NE_lock);
                        NE = 1;
                        message(REGION1, carnumber, cardirection, 2);

                        lock_acquire(NW_lock);
                        NE = 0;
                        NW = 1;
                        message(REGION2, carnumber, cardirection, 2);
                        lock_release(NE_lock);

                        lock_acquire(SW_lock);
                        NW = 0;
                        SW = 1;
                        message(REGION3, carnumber, cardirection, 2);
                        lock_release(NW_lock);

                        message(LEAVING, carnumber, cardirection, 2);
                        SW = 0;
                        lock_release(SW_lock);
                }
                else if (cardirection == 2){
                        // We are at south and need to go to west
                        lock_acquire(SE_lock);
                        SE = 1;
                        message(REGION1, carnumber, cardirection, 3);

                        lock_acquire(NE_lock);
                        SE = 0;
                        NE = 1;
                        message(REGION2, carnumber, cardirection, 3);
                        lock_release(SE_lock);

                        lock_acquire(NW_lock);
                        NE = 0;
                        NW = 1;
                        message(REGION3, carnumber, cardirection, 3);
                        lock_release(NE_lock);

                        message(LEAVING, carnumber, cardirection, 3);
                        NW = 0;
                        lock_release(NW_lock);
                }
                else if (cardirection == 3){
                        // We are at west and need to go to north
                        lock_acquire(SW_lock);
                        SW = 1;
                        message(REGION1, carnumber, cardirection, 0);

                        lock_acquire(SE_lock);
                        SW = 0;
                        SE = 1;
                        message(REGION2, carnumber, cardirection, 0);
                        lock_release(SW_lock);

                        lock_acquire(NE_lock);
                        SE = 0;
                        NE = 1;
                        message(REGION3, carnumber, cardirection, 0);
                        lock_release(SE_lock);

                        message(LEAVING, carnumber, cardirection, 0);
                        NE = 0;
                        lock_release(NE_lock);
                }
                else{
                        kprintf("Why is cardirection not in range (0-3).\n");
                        continue;
                }

                lock_acquire(NUM_CAR_LOCK);
                num_cars -= 1;
                lock_release(NUM_CAR_LOCK);

                break;
        }
}


/*
 * turnright()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a right turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnright(unsigned long cardirection,
          unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */
        while(1){
                while(num_cars > 2){
                        thread_yield();
                }
                int repeat = 0;
                lock_acquire(NUM_CAR_LOCK);
                if (num_cars > 2){
                        repeat = 1;
                }
                else{
                        num_cars += 1;
                }
                lock_release(NUM_CAR_LOCK);

                if (repeat){
                        continue;
                }

                // Decide which direction to go next
                if (cardirection == 0){
                        // We are at north and need to go to west
                        lock_acquire(NW_lock);
                        NW = 1;
                        message(REGION1, carnumber, cardirection, 3);

                        message(LEAVING, carnumber, cardirection, 3);
                        NW = 0;
                        lock_release(NW_lock);
                }
                else if (cardirection == 1){
                        // We are at east and need to go to north
                        lock_acquire(NE_lock);
                        NE = 1;
                        message(REGION1, carnumber, cardirection, 0);

                        message(LEAVING, carnumber, cardirection, 0);
                        NE = 0;
                        lock_release(NE_lock);
                }
                else if (cardirection == 2){
                        // We are at south and we need to go to east
                        lock_acquire(SE_lock);
                        SE = 1;
                        message(REGION1, carnumber, cardirection, 1);

                        message(LEAVING, carnumber, cardirection, 1);
                        SE = 0;
                        lock_release(SE_lock);
                }
                else if (cardirection == 3){
                        // We are at west and need to go to south
                        lock_acquire(SW_lock);
                        SW = 1;
                        message(REGION1, carnumber, cardirection, 2);

                        message(LEAVING, carnumber, cardirection, 2);
                        SW = 0;
                        lock_release(SW_lock);
                }
                else{
                        kprintf("Why is cardirection not in range (0-3).\n");
                        continue;
                }

                // Exit
                lock_acquire(NUM_CAR_LOCK);
                num_cars -= 1;
                lock_release(NUM_CAR_LOCK);

                break;
        }
}


/*
 * approachintersection()
 *
 * Arguments: 
 *      void * unusedpointer: currently unused.
 *      unsigned long carnumber: holds car id number.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Change this function as necessary to implement your solution. These
 *      threads are created by createcars().  Each one must choose a direction
 *      randomly, approach the intersection, choose a turn randomly, and then
 *      complete that turn.  The code to choose a direction randomly is
 *      provided, the rest is left to you to implement.  Making a turn
 *      or going straight should be done by calling one of the functions
 *      above.
 */
 
static
void
approachintersection(void * unusedpointer,
                     unsigned long carnumber)
{
        int cardirection;
        int journey_type;
        int dest_direction;

        /*
         * Avoid unused variable and function warnings.
         */

        (void) unusedpointer;
        (void) carnumber;
        (void) gostraight;
        (void) turnleft;
        (void) turnright;

        /*
         * cardirection is set randomly.
         */

        cardirection = random() % 4; 
        journey_type = random() % 3;

        if (cardirection == 0){ // North
                if (journey_type == 0){
                        // Choose the destination
                        dest_direction = (cardirection + 2) % 4;

                        lock_acquire(north_road);
                        // Approaching intersection
                        message(APPROACHING, carnumber, cardirection, dest_direction);

                        // Go straight
                        gostraight(cardirection, carnumber);

                        lock_release(north_road);
                }
                else if (journey_type == 1){
                        // Choose the destination
                        dest_direction = (cardirection + 1) % 4;

                        lock_acquire(north_road);
                        // Approaching intersection
                        message(APPROACHING, carnumber, cardirection, dest_direction);

                        // Turn Left
                        turnleft(cardirection, carnumber);

                        lock_release(north_road);
                }
                else{
                        // Choose the destination
                        dest_direction = (cardirection + 3) % 4;

                        lock_acquire(north_road);
                        // Approaching intersection
                        message(APPROACHING, carnumber, cardirection, dest_direction);

                        // Turn right
                        turnright(cardirection, carnumber);

                        lock_release(north_road);
                }
        }
        else if (cardirection == 1){
                if (journey_type == 0){
                        // Choose the destination
                        dest_direction = (cardirection + 2) % 4;

                        lock_acquire(east_road);
                        // Approaching intersection
                        message(APPROACHING, carnumber, cardirection, dest_direction);

                        // Go straight
                        gostraight(cardirection, carnumber);

                        lock_release(east_road);
                }
                else if (journey_type == 1){
                        // Choose the destination
                        dest_direction = (cardirection + 1) % 4;

                        lock_acquire(east_road);
                        // Approaching intersection
                        message(APPROACHING, carnumber, cardirection, dest_direction);

                        // Turn Left
                        turnleft(cardirection, carnumber);

                        lock_release(east_road);
                }
                else{
                        // Choose the destination
                        dest_direction = (cardirection + 3) % 4;

                        lock_acquire(east_road);
                        // Approaching intersection
                        message(APPROACHING, carnumber, cardirection, dest_direction);

                        // Turn right
                        turnright(cardirection, carnumber);

                        lock_release(east_road);
                }
        }
        else if (cardirection == 2){
                if (journey_type == 0){
                        // Choose the destination
                        dest_direction = (cardirection + 2) % 4;

                        lock_acquire(south_road);
                        // Approaching intersection
                        message(APPROACHING, carnumber, cardirection, dest_direction);

                        // Go straight
                        gostraight(cardirection, carnumber);

                        lock_release(south_road);
                }
                else if (journey_type == 1){
                        // Choose the destination
                        dest_direction = (cardirection + 1) % 4;

                        lock_acquire(south_road);
                        // Approaching intersection
                        message(APPROACHING, carnumber, cardirection, dest_direction);

                        // Turn Left
                        turnleft(cardirection, carnumber);

                        lock_release(south_road);
                }
                else{
                        // Choose the destination
                        dest_direction = (cardirection + 3) % 4;

                        lock_acquire(south_road);
                        // Approaching intersection
                        message(APPROACHING, carnumber, cardirection, dest_direction);

                        // Turn right
                        turnright(cardirection, carnumber);

                        lock_release(south_road);
                }
        }
        else if (cardirection == 3){
                if (journey_type == 0){
                        // Choose the destination
                        dest_direction = (cardirection + 2) % 4;

                        lock_acquire(west_road);
                        // Approaching intersection
                        message(APPROACHING, carnumber, cardirection, dest_direction);

                        // Go straight
                        gostraight(cardirection, carnumber);

                        lock_release(west_road);
                }
                else if (journey_type == 1){
                        // Choose the destination
                        dest_direction = (cardirection + 1) % 4;

                        lock_acquire(west_road);
                        // Approaching intersection
                        message(APPROACHING, carnumber, cardirection, dest_direction);

                        // Turn Left
                        turnleft(cardirection, carnumber);

                        lock_release(west_road);
                }
                else{
                        // Choose the destination
                        dest_direction = (cardirection + 3) % 4;

                        lock_acquire(west_road);
                        // Approaching intersection
                        message(APPROACHING, carnumber, cardirection, dest_direction);

                        // Turn right
                        turnright(cardirection, carnumber);

                        lock_release(west_road);
                }
        }
        else{
                kprintf("Where the hell you think you are going?\n");
        }

}


/*
 * createcars()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up the approachintersection() threads.  You are
 *      free to modiy this code as necessary for your solution.
 */

int
createcars(int nargs,
           char ** args)
{
        int index, error;

        // Initialize the locks
        NW_lock = lock_create("nw_lock");
        SW_lock = lock_create("sw_lock");
        NE_lock = lock_create("ne_lock");
        SE_lock = lock_create("se_lock");
        NUM_CAR_LOCK = lock_create("num_car_lock");
        north_road = lock_create("north_road_lock");
        east_road = lock_create("east_road_lock");
        west_road = lock_create("west_road_lock");
        south_road = lock_create("south_road_lock");
    
        /*
         * Start NCARS approachintersection() threads.
         */

        for (index = 0; index < NCARS; index++) {
                error = thread_fork("approachintersection thread",
                                    NULL, index, approachintersection, NULL);

                /*
                * panic() on error.
                */

                if (error) {         
                        panic("approachintersection: thread_fork failed: %s\n",
                              strerror(error));
                }
        }
        
        /*
         * wait until all other threads finish
         */

        while (thread_count() > 1)
                thread_yield();

	(void)message;
        (void)nargs;
        (void)args;
        kprintf("stoplight test done\n");
        return 0;
}

