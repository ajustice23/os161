/*
 * Thread test code.
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <test.h>

#define NTHREADS  10
int count;
static struct semaphore *tsem = NULL;

static
void
init_semmine(void)
{
        if (tsem==NULL) {
                tsem = sem_create("tsem", 0);
                if (tsem == NULL) {
                        panic("threadtest: sem_create failed\n");
                }
        }
}
static void mythreadtest(void *junk, unsigned long num){
	(void)junk;
	(void)num;

	kprintf("%d",count++);
	V(tsem);
}
static 
void
runthreadsmine(void)
{
        char name[16];
        int i, result;

        for (i=0; i<NTHREADS; i++) {
                snprintf(name, sizeof(name), "threadtest%d", i);
                result = thread_fork(name, NULL, mythreadtest,
                                     NULL, i);
                if (result) {
                        panic("threadtest: thread_fork failed %s)\n",
                              strerror(result));
                }
        }

        for (i=0; i<NTHREADS; i++) {
                P(tsem);
        }
}


int
threadtestmine(int nargs, char **args)
{
        (void)nargs;
        (void)args;
	count = 0;
        init_semmine();
        kprintf("Starting thread test...\n");
        runthreadsmine();
        kprintf("\nThread test done.\n");

        return 0;
}
