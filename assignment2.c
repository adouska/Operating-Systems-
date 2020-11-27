/*
 * file assignment2.c
 * authors Douska Anna<el16771> & Papaspiliopoulos Polykarpos<el14796>
 * this program creates as many child processes as the number of inputs given
 * the number given is the delay in second that shows how long it takes 
 * each process to grow its counter 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#define N 100

pid_t *pid, y, fy;
int *state;  
int x, cnt;
int length;


void usage(const char *prog){
	printf("Usage: %s [delay in seconds for each proccess of n] d1 d2 d3 ... dn\n", prog);
	exit(EXIT_FAILURE);
}

int safe_atoi(char *s, int *val){
	long l;
	char *endp;

	l = strtol(s, &endp, 10);
	if (s != endp && *endp == '\0') {
		*val = l;
		return 0;
	} else
		return -1; 
}

void child_handler(int signal){
	if (signal == SIGUSR1){
		printf("[Child Process %d: %d] Value: %d!\n",x ,y ,cnt);
	}
	if (signal == SIGUSR2){
		printf("[Child Process %d] Echo\n",y);
	}
	if (signal == SIGALRM){
		printf("[Child process %d: %d] Time Expired! Final Value: %d\n", x, y, cnt);
		exit(EXIT_SUCCESS);
	}	
}

void parent_handler(int signal){
	if (signal == SIGUSR1){
		printf("[Father process: %d] Will ask current values (SIGUSR1) from all active children processes.\n", fy);
		for (int i=0; i<length; i++){
			if (state[i]!=2){
				if(kill(pid[i],SIGUSR1)==-1){
					perror("kill -SIGUSR1 to children");
				}
			}
		}
	}
	else if (signal == SIGUSR2){
		printf("[Process: %d] Echo!\n", fy); 
	}
	else{ //SIGTERM or SIGNINT 
		for (int i=0; i<length; i++){
			if (state[i]!=2){
				printf("[Father process: %d] Will terminate (SIGTERM) child process %d: %d}\n", fy, i+1, pid[i]);
				if(kill(pid[i],SIGTERM)==-1){
					perror("kill -SIGTERM to children");
				}
			}
		}
	}
}

int main(int argc, char** argv){

	length = argc-1;
	pid = malloc((sizeof(pid_t))*(length));
	if (pid == NULL) {
		perror("malloc pid[]");
		exit(EXIT_FAILURE);
	}

	fy = getpid(); //parent pid

	int g;
	for (int i=0; i<length; i++){
		if (safe_atoi(argv[i+1], &g) == -1){
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}	

	printf("Maximum execution time of children is set to %d secs\n",N);

	for (int i=0; i<length; i++){	
		int d;
		safe_atoi(argv[i+1], &d);

		pid[i]=fork(); //parent process keeps in pid[] all children  pids
		if (pid[i] == -1){
			perror("fork");
			exit(EXIT_FAILURE);
		}
		else if ( pid[i] == 0){ //child process code

			y = getpid();
			x = i+1; //process number 
			printf("[Child process %d : %d] Was created and will pause!\n", x, y);

			cnt=0;

			struct sigaction child_action;
			child_action.sa_handler= child_handler;
			if (sigaction(SIGUSR1, &child_action, NULL) == -1){
				perror("sigaction child SIGUSR1");
				exit(EXIT_FAILURE);
			}
			if (sigaction(SIGUSR2, &child_action, NULL) == -1){
				perror("sigaction child SIGUSR2");
				exit(EXIT_FAILURE);
			}
			if (sigaction(SIGALRM, &child_action, NULL) == -1){
				perror("sigaction child SIGALRM");
				exit(EXIT_FAILURE);
			}

			if (raise(SIGSTOP)!=0){
				perror("raise");
				exit(EXIT_FAILURE);
			}

			alarm(N);	//maximum child process duration is N after than SIGALRM is sent

			printf("[Child Process %d: %d] Is starting!\n",x,y);
			while(1){
				cnt++;
				struct timespec req = { .tv_sec = d, .tv_nsec = 0 };
				struct timespec rem, *a = &req, *b = &rem; 
				while (nanosleep (a, b) && errno == EINTR){ //if nanosleep is interrupted by a system-call it continues from where it stopped
					struct timespec *tmp = a;
					a = b;
					b = tmp;
				}
			}
		}
	}

	int status; 
	pid_t w;
	struct sigaction parent_action;

	parent_action.sa_handler=parent_handler;
	parent_action.sa_flags = SA_RESTART; //if systemcall is interrupted by handling a signal it is restarted 
	if (sigaction(SIGUSR1, &parent_action, NULL) == -1){
		perror("sigaction parent SIGUSR1");
		exit(EXIT_FAILURE);
	}
	if (sigaction(SIGUSR2, &parent_action, NULL) == -1){
		perror("sigaction parent SIGUSR2");
		exit(EXIT_FAILURE);
	}
	if (sigaction(SIGTERM, &parent_action, NULL) == -1){
		perror("sigaction parent SIGTERM");
		exit(EXIT_FAILURE);
	}
	if (sigaction(SIGINT, &parent_action, NULL) == -1){
		perror("sigaction parent SIGINT");
		exit(EXIT_FAILURE);
	}


	state = malloc((sizeof(int))*(length));
	if (state == NULL) {
		perror("malloc stopped[]");
		exit(EXIT_FAILURE);
	}
	for (int i=0; i<argc-1; i++){
		state[i]=0;
	}    

	for (int i=0; i<argc-1; i++){
		w = waitpid(pid[i], &status, WUNTRACED); //returns pid if process is terminated or stopped
		if (w == -1){
			perror("waitpid SIGSTOP");
			exit(EXIT_FAILURE);
		}else if (WIFSTOPPED(status)) { //if it was stopped else it was terminated 
			state[i]=1; //stopped
		}else{
			state[i]=2; //terminated
		}
	}	

	for (int i=0; i<argc-1; i++){
		if (state[i]==1){
			if(kill(pid[i],SIGCONT)==-1){
				perror("SIGCONT to children");
				exit(EXIT_FAILURE);		
			}	
		}
	}
	int check;
	check = length;
	for (int i=0; i<length; i++){
		check=length-i;
		if (check==1){
			printf("[Parent process: %d] Waiting for %d child that is still working to terminate!\n", fy, check);
		}
		else if(check>1){
			printf("[Parent process: %d] Waiting for %d children that are still working to terminate!\n", fy, check);
		}
		w = wait(&status);	//wait any child 
		if (w == -1) {
			perror("wait");
			exit(EXIT_FAILURE);
		}
		for (int i=0; i<length; i++){
			if(pid[i] == w){
				state[i]=2; //when child is terminated
			}
		}
	}
	printf("[Parent process: %d] Will exit as all children have been terminated.\n", fy);
	free(pid);	
	return 0;
}
