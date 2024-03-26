#include <bits/time.h>
#include <bits/types/struct_timeval.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <time.h>


/* use pipe to exchange byte between two process, each direction 1 byte, calculate the speed in the way exchange times/ sec .
 * */

#define ITERATION 10

int main(){
    int pipParrent[2],pipChild[2];
    pid_t pid;
    char byte ='A';

    struct timespec   start, end;
    double elapsed_time;

    if(pipe(pipParrent) == -1 || pipe(pipChild) == -1 ){
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();

    if(pid == -1){
      perror("fork");
      exit(EXIT_FAILURE);
    }

    if(pid == 0 ){
        printf("run child process\n");
        close(pipParrent[1]);   // close unnecessary source.
        close(pipChild[0]);

        while( read(pipParrent[0],&byte,1) >  0){
        
            printf("Child finished read\n");
            write(pipChild[1],&byte,1);
            printf("Child finished write\n");
        
        }
        
        close(pipParrent[0]);
        close(pipChild[1]);
        exit(EXIT_SUCCESS);

    }else {

            printf("run parrent process\n");
            close(pipParrent[0]);   // close unnecessary source.
            close(pipChild[1]);

            clock_gettime(CLOCK_MONOTONIC,&start); 
            
            int i ;
            for(i = 0;i<ITERATION;i++){

                    write(pipParrent[1],&byte,1);
                    printf("Parrent finished write\n");
                    read(pipChild[0],&byte,1);
                    printf("Parrent finished read\n");
            }

            clock_gettime(CLOCK_MONOTONIC,&end);

            close(pipParrent[1]);
            close(pipChild[0]);
            elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec  - start.tv_nsec ) / 1e9;
            printf("exchange times /sec: [%.2f]\n",ITERATION / elapsed_time ) ;
    }



    return 0;
}

