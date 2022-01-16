#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);

// Adding the _exit() boiler plate code to supress "Unknown syscall 0" warning
int sys__exit(int code);

#endif /* _SYSCALL_H_ */
