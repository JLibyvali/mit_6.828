#include <stdio.h>                                                                                                                                                          Included header stdio.h is not used directly (fixes available) 
#include <unistd.h> 
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
 
 /*Inside system, xv6 maintain file-description table for  every process. There exist 3 file-description
  * description 0: also means "stdin", process read  data from it.
  * description 1: also means "stdout", process write data in it.
  * description 2: also means "stderr", process read  error in it.
  *
  * 'fork()' and 'file descriptions' work together to implement I/O redirection
  *
  * */
/* Every file's file-description has the offset, offset can move by 'write and red' system call.
 *
 * */ 
     
// exec() can create new memory block for the process with reading from "file arguments" inputs; 
 int main(){
 
//    char* argv[3];
//
//    argv[0] = "echo",argv[1] = "hello",argv[2] = 0; 
//    execvp("/bin/echo",argv);   // "exec" hadn't be allowed since ISO99, so use execvp(), it will run the "file" with argv.
//    printf("exec error\n"); // exec will not return;

// open 
    char* arg[2];
    arg[0]="cat",arg[1]=0;

    pid_t  pid = fork();
    if(pid == 0){
    
            write(1,"hello-\n",7);

            close(0);// free fd0;
            open("FD_input.txt",O_RDONLY);
            execvp("cat",arg);
            exit(0);
    }else{
         int status;
         pid = wait(&status);
         write(1,"world\n",6);
         printf("open had finished in child process with st:%d",status);

    }

 
    return 0;
 }

