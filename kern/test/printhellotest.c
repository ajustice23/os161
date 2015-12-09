#include <types.h>
#include <lib.h>
#include <test.h>

int printhellotest(int nargs, char ** args){
	if(nargs<2){
		kprintf("Type in the testname and your name.\n");
	}else{
		kprintf("Hello There %s!\n",args[1]);
	}

	return 0;
}
