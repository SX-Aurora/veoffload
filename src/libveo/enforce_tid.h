#ifndef _ENFORCE_TID_H
#define _ENFORCE_TID_H

void _enforce_tid(long tid) __attribute__((weak));

#define enforce_tid(tid) \
	if (_enforce_tid) \
		_enforce_tid(tid)

#endif

