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

static syscall_t _orig_syscall = (syscall_t)NULL;
static __thread long _fake_tid = 0L;

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
	long a[6];
	va_list ap;
	int i;

	if (_orig_syscall == (syscall_t)NULL)
		orig_syscall = (syscall_t)_get_libc_func("syscall");
		
	if (number == SYS_gettid) {
		if (_fake_tid != 0L)
			return _fake_tid;
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
	return _orig_syscall(number, a[0], a[1], a[2], a[3], a[4], a[5]);
}

void enforce_tid(long tid)
{
	_fake_tid = tid;
}
