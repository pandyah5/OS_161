/*
 * Main.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <lib.h>
#include <machine/spl.h>
#include <test.h>
#include <synch.h>
#include <thread.h>
#include <scheduler.h>
#include <dev.h>
#include <vfs.h>
#include <vm.h>
#include <syscall.h>
#include <version.h>
#include <clock.h>

/*
 * These two pieces of data are maintained by the makefiles and build system.
 * buildconfig is the name of the config file the kernel was configured with.
 * buildversion starts at 1 and is incremented every time you link a kernel. 
 *
 * The purpose is not to show off how many kernels you've linked, but
 * to make it easy to make sure that the kernel you just booted is the
 * same one you just built.
 */
extern const int buildversion;
extern const char buildconfig[];

/*
 * Copyright message for the OS/161 base code.
 */
static const char harvard_copyright[] =
    "Copyright (c) 2000, 2001, 2002, 2003\n"
    "   President and Fellows of Harvard College.  All rights reserved.\n";


/*
 * Initial boot sequence.
 */
static
void
boot(void)
{
	/*
	 * The order of these is important!
	 * Don't go changing it without thinking about the consequences.
	 *
	 * Among other things, be aware that console output gets
	 * buffered up at first and does not actually appear until
	 * dev_bootstrap() attaches the console device. This can be
	 * remarkably confusing if a bug occurs at this point. So
	 * don't put new code before dev_bootstrap if you don't
	 * absolutely have to.
	 *
	 * Also note that the buffer for this is only 1k. If you
	 * overflow it, the system will crash without printing
	 * anything at all. You can make it larger though (it's in
	 * dev/generic/console.c).
	 */

	kprintf("\n");
	kprintf("OS/161 base system version %s\n", BASE_VERSION);
	kprintf("%s", harvard_copyright);
	kprintf("\n");

	kprintf("Shinchan's system version %s (%s #%d)\n", 
		GROUP_VERSION, buildconfig, buildversion);
	kprintf("\n");

	ram_bootstrap();
	scheduler_bootstrap();
	thread_bootstrap();
	vfs_bootstrap();
	dev_bootstrap();
	vm_bootstrap();
	kprintf_bootstrap();

	/* Default bootfs - but ignore failure, in case emu0 doesn't exist */
	vfs_setbootfs("emu0");


	/*
	 * Make sure various things aren't screwed up.
	 */
	assert(sizeof(userptr_t)==sizeof(char *));
	assert(sizeof(*(userptr_t)0)==sizeof(char));
}

/*
 * Shutdown sequence. Opposite to boot().
 */
static
void
shutdown(void)
{

	kprintf("Shutting down.\n");
	
	vfs_clearbootfs();
	vfs_clearcurdir();
	vfs_unmountall();

	splhigh();

	scheduler_shutdown();
	thread_shutdown();
}

/*****************************************/

/*
 * reboot() system call.
 *
 * Note: this is here because it's directly related to the code above,
 * not because this is where system call code should go. Other syscall
 * code should probably live in the "userprog" directory.
 */
int
sys_reboot(int code)
{
	switch (code) {
	    case RB_REBOOT:
	    case RB_HALT:
	    case RB_POWEROFF:
		break;
	    default:
		return EINVAL;
	}

	shutdown();

	switch (code) {
	    case RB_HALT:
		kprintf("The system is halted.\n");
		md_halt();
		break;
	    case RB_REBOOT:
		kprintf("Rebooting...\n");
		md_reboot();
		break;
	    case RB_POWEROFF:
		kprintf("The system is halted.\n");
		md_poweroff();
		break;
	}

	panic("reboot operation failed\n");
	return 0;
}
/*
 * This is the system exit call which just prints a statement out nothing else.
 */

int sys__exit(int code){
	kprintf("The program wants to exit. The code was: %d\n", code);
	return 0;
}

int sys_write(int filehandle, const void *buf, size_t size, int32_t* retval){
	/*
	The values 0, 1, 2 for filehandle can also be given, for standard input, standard output 
	& standard error, respectively. 
	*/
	if (filehandle == 1 || filehandle == 2){
		char* kernel_dest = kmalloc(size + 1);
		kernel_dest[size] = '\0';

		if (copyin(buf, kernel_dest, size) == EFAULT){
			return EFAULT;
		}
		kprintf("%s", kernel_dest);
		kfree(kernel_dest);
		*retval = size;
		return 0;
	}
	else{
		return EBADF;
	}

	//return 0;
}

int sys_read(int fd, void *buf, size_t buflen, int32_t* retval){
	if (buf == NULL){
		*retval = -1;
		return EFAULT;
	}
	if (fd != 0){
		*retval = -1;
		return EBADF;
	}
	else {
		if(buflen != 1){
			*retval = -1;
			return -1;
		}
		char* kernel_dest = kmalloc(buflen);
		*kernel_dest = getch();

		if (copyout(kernel_dest, buf, buflen) == EFAULT){
			return EFAULT;
		}

		kfree(kernel_dest);
		*retval = buflen;
		return 0;
	}
}

time_t sys___time(time_t* seconds, unsigned long* nanoseconds, int32_t* retval){
	if (seconds != NULL && nanoseconds != NULL){
		time_t kern_seconds;
		u_int32_t kern_nanoseconds;
		gettime(&kern_seconds, &kern_nanoseconds);
		if(copyout(&kern_seconds, (userptr_t) seconds, sizeof(kern_seconds)) == EFAULT){
			return EFAULT;
		}
		if(copyout(&kern_nanoseconds, (userptr_t) nanoseconds, sizeof(kern_nanoseconds)) == EFAULT){
			return EFAULT;
		}
		*retval = kern_seconds;
		return 0;
	}
	else if(seconds != NULL){
		time_t kern_seconds;
		u_int32_t kern_nanoseconds;
		gettime(&kern_seconds, &kern_nanoseconds);
		if(copyout(&kern_seconds, (userptr_t) seconds, sizeof(kern_seconds)) == EFAULT){
			return EFAULT;
		}
		*retval = kern_seconds;
		return 0;
	}
	else if(nanoseconds != NULL){
		time_t kern_seconds;
		u_int32_t kern_nanoseconds;
		gettime(&kern_seconds, &kern_nanoseconds);
		if(copyout(&kern_nanoseconds, (userptr_t) nanoseconds, sizeof(kern_nanoseconds)) == EFAULT){
			return EFAULT;
		}
		*retval = kern_seconds;
		return 0;
	}
	else{
		time_t kern_seconds;
		u_int32_t kern_nanoseconds;
		gettime(&kern_seconds, &kern_nanoseconds);
		*retval = kern_seconds;
		return 0;
	}
}

unsigned int sys_sleep(unsigned int seconds){
	clocksleep((int)seconds);
	return 0;
}

/*
 * Kernel main. Boot up, then fork the menu thread; wait for a reboot
 * request, and then shut down.
 */
int
kmain(char *arguments)
{
	boot();

	menu(arguments);

	/* Should not get here */
	return 0;
}
