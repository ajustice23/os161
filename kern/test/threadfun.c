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
const char* lockname;
struct lock *lock;

static
void init_semmine(void)
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
	lock_acquire(lock);
	//char threadnum = '0' + num;
	
	unsigned int i;
	//lock_acquire(lock);
	kprintf("\n");
	for(i=0;i<num;i++){
		kprintf("%d",count++);
	}

	lock_release(lock);
	V(tsem);
}

static 
void runthreadsmine(int nargs, char **args)
{ 
       char name[16];
	//int nthreads = atoi(args[1]);
	//kprintf("%d",nthreads);
        int i, result;
	const char* locknamefori;
        struct lock *lock1;
	lockname = "lock";
	locknamefori ="lock1";
	lock = lock_create(lockname);
	lock1= lock_create(locknamefori);
	if (nargs == 1){
        	for (i=0; i<NTHREADS; i++) {
               		snprintf(name, sizeof(name), "threadtest%d", i);
                	result = thread_fork(name, NULL, mythreadtest,
                     	                NULL, 10);
               		if (result) {
                        	panic("threadtest: thread_fork failed %s)\n",
                              	strerror(result));
                	}
        	}

        	for (i=0; i<NTHREADS; i++) {
                	P(tsem);
        	}
	}else if(nargs == 2){
		for (i=0; i<atoi(args[1]); i++) {
			lock_acquire(lock1);
                        snprintf(name, sizeof(name), "threadtest%d", i);
                        result = thread_fork(name, NULL, mythreadtest,
                                        NULL, 10);
                        if (result) {
                                panic("threadtest: thread_fork failed %s)\n",
                                strerror(result));
                        }
			lock_release(lock1);
                }

                for (i=0; i<atoi(args[1]); i++) {
                        P(tsem);
                }
	}else{
		int loop = atoi(args[2]);
		for (i=0; i<atoi(args[1]); i++) {
                        snprintf(name, sizeof(name), "threadtest%d", i);
                        result = thread_fork(name, NULL, mythreadtest,
                                        NULL, loop);
                        if (result) {
                                panic("threadtest: thread_fork failed %s)\n",
                                strerror(result));
                        }
                }

                for (i=0; i<atoi(args[1]); i++) {
                        P(tsem);
                }
	}
	lock_destroy(lock1);
}


int
threadtestmine(int nargs, char **args)
{
//        (void)nargs;
//       (void)args;
	count = 0;
        init_semmine();
        kprintf("Starting thread test...\n");
        runthreadsmine(nargs, args);
	//lock_destory(lock);
        kprintf("\nThread test done.\n");

        return 0;
}
