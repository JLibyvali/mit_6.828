#include <stdio.h>
#include <unistd.h>

//#######################fork()

int main(){


	int* pid = fork();

	if(pid >0){	// It's in parent process,
	
	printf("\"fork()\" return in parent process, child pid =%d\n",pid);
	pid = wait();	// wait() will wait one child process belongs to process called, and return the child-pid  
	printf("child process %d  had exited\n",pid);	
		
	}else if(pid == 0){	// It's in child process
	
	printf("\"fork()\" return in child process,continue exiting\n");
	exit(0);	// exit() will free system resource for process  called 
	
	}else{
		printf("\"fork()\" failed\n");
	}


return 0;
}
