/*

gcc -g -o test_veothread test_veothread.c -I/opt/nec/ve/veos/include -L/opt/nec/ve/veos/lib64 -lveo -lvepseudo -lpthread

*/

#define _GUN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <ve_offload.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

void _enforce_tid(long) __attribute__((weak));
long syscall(long, ...);

int flag = 0;
struct veo_proc_handle* proc;
uint64_t lib_handle;
uint64_t sym;
struct veo_thr_ctxt *ctx;


void* thread1(void* arg)
{
    proc = veo_proc_create(1);

    fprintf(stderr, "thread1 (%ld): proc=%p\n", syscall(SYS_gettid), proc);
#if 0
    lib_handle = veo_load_library(proc, "./libvehello.so");
    fprintf(stderr, "lib_handle = %p\n", (void *)lib_handle);
    if (!lib_handle)
        return (void *)0;
#endif

    flag = 1;
    for (;;) {
        if (flag == 2)
            break;
    }
    fprintf(stderr, "thread1: resumes\n");
#if 0
    ctx = veo_context_open(proc);
    fprintf(stderr, "VEO context = %p\n", ctx);
#endif
    struct veo_args *argp = veo_args_alloc();
    veo_args_set_i64(argp, 0, 42);
    uint64_t id = veo_call_async(ctx, sym, argp);
    fprintf(stderr, "VEO request ID = 0x%lx\n", id);

    uint64_t retval;
    int ret = veo_call_wait_result(ctx, id, &retval);
    fprintf(stderr, "0x%lx: %d, %lu\n", id, ret, retval);

    veo_args_free(argp);
    int close_status = veo_context_close(ctx);
    fprintf(stderr, "close status = %d\n", close_status);

    return (void*)0;
}

void* thread2(void* arg)
{
    fprintf(stderr, "thread2 (%ld): alive\n", syscall(SYS_gettid));
    for (;;) {
        if (flag == 1)
            break;
    }
    fprintf(stderr, "thread2: continues\n");

    lib_handle = veo_load_library(proc, "./libvehello.so");
    fprintf(stderr, "lib_handle = %p\n", (void *)lib_handle);
    if (!lib_handle)
        return (void *)0;

    sym = veo_get_sym(proc, lib_handle, "hello");
    fprintf(stderr, "symbol address = %p\n", (void *)sym);
#if 1
    ctx = veo_context_open(proc);
    fprintf(stderr, "VEO context = %p\n", ctx);
#endif 
    flag = 2;

    return (void*)0;
}

int
main()
{
    _enforce_tid(1);
    _enforce_tid(0);
    pthread_t t2;
    pthread_create(&t2, NULL, thread2, NULL);
    thread1(NULL);
    pthread_join(t2, NULL);
    return 0;

#if 0
    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);

    pthread_join(t2, NULL);
    pthread_join(t1, NULL);
    return 0;
#endif
}
