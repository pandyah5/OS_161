#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);

// Adding the _exit() boiler plate code to supress "Unknown syscall 0" warning
int sys__exit(int code);

// Adding the write() system call code
int sys_write(int filehandle, const void *buf, size_t size, int32_t* retval);

// Adding the read() system call code
int sys_read(int fd, void *buf, size_t buflen, int32_t* retval);

// Adding the __time() system call code
time_t sys___time(time_t *seconds, unsigned long *nanoseconds, int32_t* retval);

// Adding the sleep() code
unsigned int sys_sleep(unsigned int seconds);

#endif /* _SYSCALL_H_ */
