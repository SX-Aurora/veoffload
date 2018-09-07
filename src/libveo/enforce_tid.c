/*
 * Enforce TID returned by syscall(SYS_gettid) to a certain value.
 * This is necessary because libvepseudo is using gettid a lot in
 * order to specify the targeted VE process ID. When using VEO
 * in a multithreaded VH program, this targeting can go wrong. 
 *
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <stdlib.h>		/* for EXIT_FAILURE */
#include <unistd.h>		/* for _exit() */
#include <sys/syscall.h>
#include <sys/types.h>

#ifndef RTLD_NEXT
#define RTLD_NEXT      ((void *) -1l)
#endif

typedef long (*syscall_t) (long number, ...);

#define MAX_TID_STACK 10
struct tid_stack {
	long tid[MAX_TID_STACK];
	int ptr;
};

static syscall_t __real_syscall = (syscall_t)NULL;
static __thread struct tid_stack _fake_tid;


static void *_get_libc_func(const char *funcname)
{
	void *func;
	char *error;

	func = dlsym(RTLD_NEXT, funcname);
	if ((error = dlerror()) != NULL) {
		fprintf(stderr, "I can't locate libc function `%s' error: %s",
				funcname, error);
		_exit(EXIT_FAILURE);
	}
	return func;
}

long syscall(long number, ...)
{
	long a[6], tid;
	va_list ap;
	int i;

	if (__real_syscall == (syscall_t)NULL)
		__real_syscall = (syscall_t)_get_libc_func("syscall");
		
	if (number == SYS_gettid) {
		tid = _fake_tid.tid[_fake_tid.ptr];
		if (tid != 0L)
			return tid;
	}
	/*
	 * hack to pass through args to variadic function,
	 * luckily syscall() just takes all (maximum) 6
	 * args as 64 bit integers.
	 */
	va_start(ap, number);
	for (i = 0; i < 6; i++)
		a[i] = va_arg(ap, long);
	va_end(ap);
	return __real_syscall(number, a[0], a[1], a[2], a[3], a[4], a[5]);
}

static void _enforce_tid_push(long tid)
{
	if (_fake_tid.ptr == MAX_TID_STACK - 1) {
		fprintf(stderr, "ERROR: tid stack full!\n");
		return;
	}
	_fake_tid.tid[++_fake_tid.ptr] = tid;
}

static void _enforce_tid_pop()
{
	if (_fake_tid.ptr == 0) {
		fprintf(stderr, "ERROR: tid stack empty!\n");
		return;
	}
	_fake_tid.tid[_fake_tid.ptr] = 0;
	_fake_tid.ptr--;
}


void _enforce_tid(long tid)
{
	if (tid > 0)
		_enforce_tid_push(tid);
	else
		_enforce_tid_pop(tid);
}
