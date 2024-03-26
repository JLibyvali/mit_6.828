#include <unistd.h> 
#include <stdio.h>

int main(){

 int p[2];  // create a pipe, p[0] is pipe's read-end in fd0 , and p[1] is pipe's write-end in fd1.
 char* arg[2];
 arg[0] = "wc",arg[1] = 0;

 pipe(p);   // create a pipe, and record the 'FD' in p.

 /*dup() copys's process will share the same FD offset
  * */

 pid_t pid = fork();

 if(pid == 0){ // in child process
    printf("run child process\n");
    close(0);   // close  fd 0
    dup(p[0]);  // copy 'p[0](pipe read-end)' , so it turns 'stdin'  to pipe's read-end.
              
    close(p[0]);
    close(p[1]);

    execvp("/bin/wc",arg);

 }else {
         printf("run parrent process\n");
         close(p[0]); 
         write(p[1], "hello world!",12);
         close(p[1]);
 
 }

    return 0;

}
