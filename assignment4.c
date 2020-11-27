#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#define RED "\033[33m"
#define BLUE "\033[36m"
#define MAGENTA "\033[35m"
#define GREEN "\033[32m"
#define WHITE "\033[37m"
#define MAX(a, b) ((a) > (b) ? (a) : (b))

char token[20][100];
int a = 0;

int safe_atoi(char *s, int *val) {
    long l;
    char *endp;
    l = strtol(s, &endp, 10);
    if (s != endp && *endp == '\0') {
        *val = l;
        return 0;
    } else
        return -1;
}

void usage(const char *prog) {
    printf("Usage: %s [--host HOST] [--port PORT] [--debug]\n", prog);
    exit(EXIT_FAILURE);
}

int split_word(char *input) {
    int i, j, ctr;
    j = 0;
    ctr = 0;
    
    for (i = 0; i <= (strlen(input)); i++) {    
        if (input[i] == ' ' || input[i] == '\0') {
		        token[ctr][j] = '\0';
		        ctr++;  //for next word
		        j = 0;    //for next word, init index to 0
        }else {
            	token[ctr][j] = input[i];
            	j++;
        }
    }
    a = ctr;		 //number of words that were read from STDIN
    return a;
}

int check_string(const char *s){
	int g=0;
	unsigned char c;
	while((c = s[g])>0 && (isupper(c)!=0 || islower(c)!=0)){
		g++;
	}
	if  (g == strlen(s)){
		return 0;
	}else{
		return -1;
	}
}
		///// MAIN PROGRAMM /////
int main(int argc, char **argv) {
    int await_code = 0;	// is 1 when we have send a message and we "owe" verification-code
    char coronacode[100];
    int type, lum, stamp, temp, code, grb;
    int mode = 0; 		// mode is 0 when debug option is not activated
    char *hostname = "tcp.akolaitis.os.grnetcloud.net";
    char *service = "8080";									//default values 
    if (argc > 6 || argc == 3 || argc == 4) {				
        usage(argv[0]);
    }

    if (argc == 2) {															//./[name] [--debug]
        if (strcmp(argv[1], "--debug") != 0) {
            usage(argv[0]);
        } else {
            mode = 1; 	// debug option is activated
        }
    }

    if (argc == 5) {
        if (strcmp(argv[1], "--host") != 0 || strcmp(argv[3], "--port") != 0) { // ./[name] [--host][--port]
            usage(argv[0]);
        }
        *hostname = *argv[2];
        *service = *argv[4];
    }

    if (argc == 6) {
        if (strcmp(argv[1], "--debug") != 0 || strcmp(argv[5], "--debug") != 0) {
            usage(argv[0]);
        }
        if (strcmp(argv[1], "--debug") == 0 && strcmp(argv[2],"--host") == 0 && strcmp(argv[4],"--port") == 0) {
            *hostname = *argv[3];
            *service = *argv[5];
            mode = 1;	// debug option is activated
        } else if(strcmp(argv[1],"--host") == 0 && strcmp(argv[3],"--port") == 0){
            *hostname = *argv[2];	//assign in order to connect with these 
            *service = *argv[4];
            mode = 1;
        }
    }

    struct addrinfo hints, *res;	
    memset(&hints, 0, sizeof(hints));	//initialise
    hints.ai_family = AF_INET;			//domain for IPv4 Internet protocol family
    hints.ai_socktype = SOCK_STREAM;	//type Provides sequenced 2-way connection-based byte stream

    int adr = getaddrinfo(hostname, service, &hints, &res); //contains the Internet address that can be specified in a call and sets the structs hints and res
    if (adr != 0) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    int sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol); //creates an endpoint for communication
    if (sock_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    printf(BLUE"Connecting!\n");

    int con = connect(sock_fd, res->ai_addr, res->ai_addrlen); //connects with the server's socket
    if (con != 0) {
        perror("connection");
        exit(EXIT_FAILURE);
    }

    printf(GREEN"Connected to %s:%s!"WHITE"\n", hostname, service);

    while (1) { 	// this eternal loop manages the communication and the read/writes from/to the socket
        fd_set inset;
        int maxfd;
        FD_ZERO(&inset);
        FD_SET(STDIN_FILENO, &inset);	// allows reading from STDIN
        FD_SET(sock_fd, &inset);		// allows reading from socket
        maxfd = MAX(STDIN_FILENO, sock_fd) + 1;
        int ready_fds = select(maxfd, &inset, NULL, NULL, NULL);	// checks availability
        if (ready_fds == -1) {
            perror("select");
            continue;                                      // try again if unavailable
        }
        if (FD_ISSET(STDIN_FILENO, &inset)) {
            int n_write = 0;
            char buffer[101];
            memset(buffer, 0, 101);		//	clears buffer to read
            int n_read = read(STDIN_FILENO, buffer, 100);
            
            if (n_read == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            
            buffer[n_read] = '\0';		//	it marks where the read string ends
            
            if (n_read > 0 && buffer[n_read - 1] == '\n') {
                buffer[n_read - 1] = '\0'; 		//replace new-line with "\0"
            }
            
            a = split_word(buffer);		//split the input into tokens and count them using a
            
            if (strcmp(buffer, "\n") == 0) {
                continue;
            } else if (await_code == 1 && strcmp(buffer, coronacode) == 0) {
                while (n_write < strlen(buffer)) {
                    n_write = write(sock_fd, &buffer, strlen(buffer) + 1);	
                    if (n_write == -1) {
                        perror("write get");
                        exit(EXIT_FAILURE);
                    }
                }
                if (mode == 1) {
                    printf(RED"[DEBUG] sent '%s'"WHITE"\n", buffer);
                }
                await_code = 0;		//	verification-code was send
                
            } else if (strcmp(buffer, "exit") == 0) {
                freeaddrinfo(res);
                close(sock_fd);	
                exit(EXIT_SUCCESS);
                
            } else if (strcmp(buffer, "get") == 0) {
                if (write(sock_fd, buffer, strlen(buffer)) == -1) {
                    perror("write get");
                    exit(EXIT_FAILURE);
                }
                if (mode == 1) {
                    printf(RED"[DEBUG] sent '%s'"WHITE"\n", buffer);
                }
            } else if (safe_atoi(token[0], &code) == 0 && check_string(token[1]) == 0 &&	/* first token must be number and the other 3 only a-z || A-Z */
                       check_string(token[2]) == 0 && check_string(token[3]) == 0 && a==4) {
                if (mode == 1) {
                    printf(RED"[DEBUG] sent '%s'"WHITE"\n", buffer);
                }
                if (code > 6 || code < 1) {
                    printf(RED"N should be between 1 and 6"WHITE"\n");
                } else if (write(sock_fd, &buffer, strlen(buffer) + 1) == -1) {	//check we wrote the whole coronamessage!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                    perror("write coronamessage");
                    exit(EXIT_FAILURE);
                }
            } else {
                printf(RED"Available commands:\n");
                printf(RED"* 'help'                  : Print this help message\n");
                printf(RED"* 'exit'                  : Exit\n");
                printf(RED"* 'get'                   : Retrieve sensor data\n");
                printf(RED"* 'N name surname reason' : Ask permission to go out"WHITE"\n");
            }

        }
        if (FD_ISSET(sock_fd, &inset)) {	//read  from socket
            char val[100];
            memset(val, 0, 101);
            int rd = read(sock_fd, &val, sizeof(val));
            if (rd == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            
            val[rd] = '\0';
            
            if (rd > 0 && val[rd - 1] == '\n') {
                val[rd - 1] = '\0';
            }
            
            a = split_word(val);
            
            if (safe_atoi(token[1], &grb) != 0) {
                if (mode == 1) {
                    printf(RED"[DEBUG] read '%s'"WHITE"\n", val);
                }
                strcpy(coronacode, token[0]);
                printf(BLUE"Send verification code: %s"WHITE"\n", val);
                await_code = 1;
                continue;
            }
            if (safe_atoi(token[0], &type) == 0 && safe_atoi(token[1], &lum) == 0 && safe_atoi(token[2], &temp) == 0 &&
                safe_atoi(token[3], &stamp) == 0) {
                if (mode == 1) {

                    printf(RED"[DEBUG] read '%s'"WHITE"\n", val);
                }
                printf(BLUE"---------------------------"WHITE"\n");
                switch (type) {
                    case 0:
                        printf(BLUE"Latest event: Boot (%d)"WHITE"\n", type);
                        break;
                    case 1:
                        printf(BLUE"Latest event: Setup (%d)"WHITE"\n", type);
                        break;
                    case 2:
                        printf(BLUE"Latest event: Interval (%d)"WHITE"\n", type);
                        break;
                    case 3:
                        printf(BLUE"Latest event: Button (%d)"WHITE"\n", type);
                        break;
                    case 4:
                        printf(BLUE"Latest event: Motion (%d)"WHITE"\n", type);
                        break;
                }
                printf(BLUE"Temperature is: %.2f"WHITE"\n", temp / 100.0f);
                printf(BLUE"Light level is: %d"WHITE"\n", lum);
                time_t rawtime = stamp;
                struct tm *info;
                info = localtime(&rawtime);
                printf(BLUE"Timestamp is: %s", asctime(info));
                printf(BLUE"---------------------------"WHITE"\n");
            } else{
                if (mode == 1) {
                    printf(RED"[DEBUG] read '%s'"WHITE"\n", val);
                }
                printf(BLUE"Response: '%s'"WHITE"\n", val);
            }
        }
    }
}
