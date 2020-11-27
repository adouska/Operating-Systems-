#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define WHITE "\033[37m"
#define MAX(a, b) ((a) > (b) ? (a) : (b))

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

/*
void handler(int signal){
	if (signal == SIGTERM)	{
			for (int i=0; i<n; i++){
						if(kill(pid[i],SIGTERM)==-1){
						perror("kill -SIGTERM to children");
						}
			}
	}
}
*/

void usage(const char *prog){
	printf("Usage: %s <nChildren> [--random] [--round-robin]\n", prog);
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
	pid_t *pid;
	int n;
	int mode = 0; 					// Default mode (round_robin)
	int child=0; 					//used to switch child process 
	int number;
	int *read_parent, *write_child, *read_child, *write_parent;		//for file descriptors

	if (argc == 1 || argc > 3) {	//wrong number of arguments
		usage(argv[0]);
	}
	if (safe_atoi(argv[1], &n) != 0) {	//non-number 2nd argument
		usage(argv[0]);
	}
	if (argc == 3) {		
		if (!(strcmp(argv[2], "--round-robin") || strcmp(argv[2], "--random"))) {
			usage(argv[0]);				//wrong 3rd argument (mode)
		}
		if (strcmp(argv[2], "--random")) {
			mode = 1;
		}
	}

	pid = malloc((sizeof(pid_t)) * (n)); 
	if (pid == NULL) {
		perror("malloc pid[]");
		exit(EXIT_FAILURE);
	}
	read_parent=malloc((sizeof(int)) * (n)); 
	if (read_parent == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	write_parent=malloc((sizeof(int)) * (n)); 
	if (write_parent == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	read_child=malloc((sizeof(int)) * (n)); 
	if (read_child == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	write_child=malloc((sizeof(int)) * (n)); 
	if (write_child == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	
	int fd[2];
	for(int i = 0; i < 2*n; i++){
		if ((i%2) == 0){  				//for even values of i 
			if (pipe(fd) != 0){			//creates all pipes for child[i]--->parent communication
				perror("pipe");
				exit(EXIT_FAILURE);
			}else{						//for odd values of i 
				read_parent[(i/2)]=fd[0];
				write_child[(i/2)]=fd[1];
			}
		}else{
			if (pipe(fd) != 0){			//creates all pipes for parent--->child[i] communication
				perror("pipe");
				exit(EXIT_FAILURE);
			}else{
				read_child[(i/2)]=fd[0]; 
				write_parent[(i/2)]=fd[1];
			}								// i and i+1 are for child[i/2] as i/2=i+1/2=i/2
		}
	}

	for (int i = 0; i < n; ++i) {
		pid[i] = fork();					//create children
		if (pid[i] == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
		if (pid[i] == 0) { 					//child code
			int value;
			close(write_parent[i]);			//not used
			close(read_parent[i]);
			int n_read;
			int n_write;
			/*
			struct sigaction action;
			action.sa_handler= handler;
			if (sigaction(SIGTERM, &action, NULL) == -1){
				perror("sigaction child SIGTERM");
				exit(EXIT_FAILURE);
			}*/
			while (1) {						//always check for information in pipe
				n_read = read(read_child[i], &value, sizeof(int));
				if (n_read == -1) {
					perror("read");
					exit(EXIT_FAILURE);
				}else if(n_read > 0){		//when there is something to read
					printf("[Child %d] [%d] Child received %d!"WHITE"\n", i + 1, getpid(), value);
					value++;
					sleep(5);
					n_write = write(write_child[i], &value, sizeof(int));
					if ((n_write < n_read) || (n_write==-1)) {
						perror("write");
						exit(EXIT_FAILURE);
					}
					printf("[Child %d] [%d] Child Finished hard work, writing back %d\n", i + 1, getpid(), value);
				}
			}
		}
	}
	for (int i = 0; i < n; ++i) {
		//if (pid[i] != 0){ // parent
		close(read_child[i]);
		close(write_child[i]);
	}
	while (1) {
		fd_set inset;
		int maxfd;
		FD_ZERO(&inset);                // we must initialize before each call to select
		FD_SET(STDIN_FILENO, &inset);   // select will check for input from stdin
		for (int i = 0; i < n; i++) {
			FD_SET(read_parent[i], &inset);  // select will check for input from pipe
		}
		// select only considers file descriptors that are smaller than maxfd
		maxfd = MAX(STDIN_FILENO, read_parent[n-1]) + 1;
		// wait until any of the input file descriptors are ready to receive
		int ready_fds = select(maxfd, &inset, NULL, NULL, NULL);
		if (ready_fds == -1) {
			perror("select");
			continue;                                      // just try again
		}
		// user has typed something, we can read from stdin without blocking
		if (FD_ISSET(STDIN_FILENO, &inset)) {
			char buffer[101];
			int n_read = read(STDIN_FILENO, buffer, 100);  
			if(n_read == -1){
				perror("read");
				exit(EXIT_FAILURE);
			}
			buffer[n_read] = '\0';                          // end of string
			// New-line is also read from the stream, discard it.
			if (n_read > 0 && buffer[n_read - 1] == '\n') {
				buffer[n_read - 1] = '\0';
			}
			printf(BLUE"Got user input: '%s'"WHITE"\n", buffer);
			if ((n_read >= 4) && (strncmp(buffer, "exit", 4) == 0)) { //kill children and exit 
				for (int i = 0; i < n; ++i) {
					if(kill(pid[i], SIGTERM) == -1){
						perror("kill");
						exit(EXIT_FAILURE);
					}else{
						close(read_parent[i]);
						close(write_parent[i]); //close apo ton patera 
					}                     
					if(wait(NULL)==-1 ){
						perror("wait");
						exit(EXIT_FAILURE);							
					}                   
				}
				printf("All children terminated\n");
				free(pid);
				free(read_parent);
				free(write_parent);
				free(read_child);
				free(write_child);
				
				exit(EXIT_SUCCESS);
			}
			if (safe_atoi(buffer, &number) != -1) {
				if (mode == 0) { 		//round-robin
					if (child < n) {
						child++;
					} else {
						child = 1;
					}
				} else {				//random
					child = (rand() % n) + 1;
				}
				int n_write = write(write_parent[child-1], &number , sizeof(int));
				if (n_write == -1) {
					perror("write parent\n");
					exit(EXIT_FAILURE);
				}
				printf("[Parent] Assigned %d to child %d\n", number, child);
			} else {
				printf("Type a number to send job to a child!\n");
			}
		}
		// someone has written bytes to the pipe, we can read without blocking
		for (int i = 0; i < n; i++) {
			if (FD_ISSET(read_parent[i], &inset)) {
				int val;
				if (read(read_parent[i], &val, sizeof(int)) == -1){
					perror("read parent");
					exit(EXIT_FAILURE);
				}               
				printf(MAGENTA"Got input from pipe: '%d'"WHITE"\n", val);
			}
		}
	}
} 
