// /opt/nec/ve/bin/ncc -fopenmp -fpic -shared -o libvetestomp.so libvetestomp.c
//

#include <stdio.h>
#include <omp.h>

void omp_loop(void)
{
  int tid;
#pragma omp parallel private(tid)
  {
    tid = omp_get_thread_num();
    printf("tid=%d\n", tid, a);
  }
}
