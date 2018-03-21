//
// /opt/nec/ve/bin/ncc -shared -fpic -pthread -o libvestackargs.so libvestackargs.c
//
#include <stdio.h>

void ftest(double *d, char *t, int *i)
{
	int a;
	printf("VE: sp = %p\n", (void *)&a);
	printf("VE: arguments passed: %p, %p, %p\n", (void *)d, (void *)t, (void *)i);
	printf("VE: arguments passed by reference: %f, %s, %d\n", *d, t, *i);
}
