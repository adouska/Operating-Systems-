/*
* file assignment.c
* authors Douska Anna<el16771> & Papaspiliopoulos Polykarpos<el14796>
* this program creates 2 child procceses that encrypt and decrypt text from an
* input file or STDIN to 2 other files
* using caesar encryption algorithm with any key given
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#define BUFFER_SIZE 64
#define PERMS 0644 //when creating a file-> user:read-write   group&others: read

typedef enum {
    ENCRYPT,
    DECRYPT
} encrypt_mode;

void usage(const char *prog){
	printf("Usage: %s --input [INPUT_FILE] --key [KEY(between 0 and 25)]\n", prog);
	exit(EXIT_FAILURE);
}
	
char caesar(unsigned char ch, encrypt_mode mode, int key)
{
    if (ch >= 'a' && ch <= 'z') {
        if (mode == ENCRYPT) {
            ch += key;
            if (ch > 'z') ch -= 26;
        } else {
            ch -= key;
            if (ch < 'a') ch += 26;
        }
        return ch;
    }

    if (ch >= 'A' && ch <= 'Z') {
        if (mode == ENCRYPT) {
            ch += key;
            if (ch > 'Z') ch -= 26;
        } else {
            ch -= key;
            if (ch < 'A') ch += 26;
        }
        return ch;
    }
    return ch; //return unchanged if not a-z or A-Z
}

int safe_atoi(char *s, int *val){
    long l;
    char *endp; //is set to first non-number character of the string

    l = strtol(s, &endp, 10);
    if (s != endp && *endp == '\0') {
        *val = l;
        return 0;
    } else
        return -1; 
}

int main(int argc, char** argv){
	int	flag, key, fd_in, i, fd_en, fd_de, n_read, n_write, status, w;
	char buffer[BUFFER_SIZE], *input;
	pid_t pid;
	
	flag = -1;
	if (argc == 5){
		if (strcmp(argv[1],"--input") == 0 && strcmp(argv[3],"--key") == 0){ /*	--input () --key ()	*/
			flag = safe_atoi(argv[4],&key);
			input = argv[2];
		}else if(strcmp(argv[1],"--key") == 0 && strcmp(argv[3],"--input") == 0){ /* --key () --input () */
			flag = safe_atoi(argv[2],&key);
			input = argv[4];
		}else{
			usage(argv[0]);}
	}else{
		usage(argv[0]);
	}

	if	(flag < 0 || key < 0 || key > 25){
	usage(argv[0]);
	}
	
	
	fd_in= open(input,O_RDONLY);
	if (fd_in == -1){
		perror(input);
		printf("Insert text to be processed (when done, press Ctrl+D for Linux or Ctrl+Z for Windows):\n");
		fd_in = STDIN_FILENO; 
     }
      
      
	pid = fork();
	if (pid == -1){
		perror("fork child1");
		exit(EXIT_FAILURE);
	}
	else if ( pid == 0){	 /*code of child 1*/
		fd_en = open("encrypted.txt",O_TRUNC | O_CREAT | O_WRONLY, PERMS);
		if (fd_en == -1){
			perror("open encrypted.txt");
			exit(EXIT_FAILURE);
		}
		do {
			n_read = read(fd_in, buffer, BUFFER_SIZE);
			if (n_read == -1) {
				perror("read input");
				exit(EXIT_FAILURE);
			}
			for(i = 0; i < n_read; i++){
				buffer[i] = caesar(buffer[i], ENCRYPT, key);
			}
			n_write = write(fd_en,buffer, n_read);
		   	if (n_write < n_read) {
		       	perror("write encrypted.txt");
		       	exit(EXIT_FAILURE);
		   	}
     	} while (n_read > 0);
		close(fd_in);
		close(fd_en);
	}
	else{	/*parent code:waits for child 1 to terminate normally else exits*/
 		w = waitpid(pid,&status,0);
 		if (w == -1){
 		perror("waitpid child1");
 		exit(EXIT_FAILURE);
 		}
		if (WIFEXITED(status)){
			if (WEXITSTATUS(status) != 0){
				exit(EXIT_FAILURE);				
			}
		}else{
			exit(EXIT_FAILURE);
		}
		pid = fork();
		
		if(pid == -1){
			perror("fork child2");
			exit(EXIT_FAILURE);
		}
		else if (pid == 0){ 	/*code of child 2*/
			fd_en = open("encrypted.txt",  O_RDONLY);
			if (fd_en == -1){
				perror("open encrypted.txt for child2");
				exit(EXIT_FAILURE);
			}
			fd_de = open("decrypted.txt",O_TRUNC | O_CREAT | O_WRONLY, PERMS);
			if (fd_de == -1){
				perror("open decrypted.txt");
				exit(EXIT_FAILURE);
			}
			do{ 
				n_read = read(fd_en, buffer, BUFFER_SIZE);
				if (n_read == -1) {
					perror("read encrypted.txt");
					exit(EXIT_FAILURE);
				}
				for(i=0;i<n_read;i++){           
					buffer[i] = caesar(buffer[i], DECRYPT, key);
				}
				n_write = write(fd_de,buffer, n_read);
				if (n_write < n_read) {
					perror("write decrypted.txt");
					exit(EXIT_FAILURE);
				}
			} while (n_read > 0);
			close(fd_en);
			close(fd_de);
		}
		else{	/*parent code waits for child 2 to terminate normally before terminating as well*/
			w = waitpid(pid,&status,0);
			if (w == -1){
		 		perror("waitpid child2");
		 		exit(EXIT_FAILURE);
	 		}
			if (WIFEXITED(status)){
				if (WEXITSTATUS(status) != 0){
					exit(EXIT_FAILURE);				
				}
			}else{
				exit(EXIT_FAILURE);
			}
		}
	}
	return 0;	/*mutual return for parent procedure,child1 & child 2*/
}
