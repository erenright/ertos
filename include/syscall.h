#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <types.h>

#define SYS_WAIT		0
#define SYS_WAKE		1
#define SYS_SLEEP		2
#define SYS_YIELD		3
#define SYS_EVENT_SET		4
#define SYS_EVENT_WAIT		5
#define SYS_ALARM		6
#define SYS_UTT_DONE		7
#define SYS_RESET		8
#define SYS_KSTAT		9
#define SYS_NETSTAT		10

// syscall_arm.s
#define _syscall(num) __syscall(num, 0)
int __syscall(int num, uint32_t arg1);

#endif // !_SYSCALL_H
