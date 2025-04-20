#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/poll.h>
FILE *fp;
//const int cargc=2;
//int aargc=0;
const int spriority=(LOG_DEBUG|LOG_USER);
const int epriority=(LOG_ERR|LOG_USER);


#define PORT "9000"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold

#define MAXDATASIZE 2 // max number of bytes we can get at once 

#define MAXRECSIZE 10 //maximum mumber of bytes to be received by by one call of rcv, -1 character (so we can store \0 to the last index)    

#define STARTPSIZE 30 //standard mumber of bytes to be reserved for storing a complete package 

char *rptr, *pptr;
int pbufs;

int sockfd;
int new_fd;  // listen on sock_fd, new connection on new_fd


bool nomore_connect=false;
bool daemon_act=false;

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
	switch (s) {
	case SIGCHLD:
	    while(waitpid(-1, NULL, WNOHANG) > 0);
	    break;
	case SIGINT:
	case SIGTERM:
	    //Gracefully exits, 
	    //completing any open connection operations, 
	    //closing any open sockets, 
	    //and deleting the file /var/tmp/aesdsocketdata.
	    //signal to not accept any more connection requests after this one is finished processing
	    nomore_connect=true;
	    printf ("Caught signal, exiting\n");
            syslog(spriority, "Caught signal, exiting\n");
	    break;
	default: /*Should never get this case*/
	    break;
    }
    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



int main (int argc, char * argv[])
{

//open system logger
openlog(argv[0], LOG_PID, LOG_USER);

//path to file file to append to create, if required
const char *stfile="/var/tmp/aesdsocketdata";


struct addrinfo hints, *servinfo, *p;
struct sockaddr_storage their_addr; // connector's address information
socklen_t sin_size;
struct sigaction sa;
int yes=1;
char s[INET6_ADDRSTRLEN];
char nlcheck='\n';
int rv;
char rchar;

int numbytes;
int pcounter;  
//char buf[MAXDATASIZE];

int    timeout=100; //100ms tiemout for poll 
struct pollfd fds;
int    nfds = 1;
int pollres=0;
int checkpar=1;
int pid=-1;

memset(&hints, 0, sizeof hints);
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE; // use my IP

//start as daemon if -d handed over as argument
if (argc == 2)
{
    checkpar=strcmp(argv[1], "-d");
    if (checkpar == 0)
    {
    	printf ("starting as daemon\n");
    	syslog(spriority, "starting as daemon\n");
    	daemon_act=true;	
    }
}


if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    printf ("getaddrinfo: %s\n", gai_strerror(rv));
    syslog(epriority, "getaddrinfo: %s\n", gai_strerror(rv));
    closelog();
    return 1;
}

 // loop through all the results in servinfo and bind to the first we can
 for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            printf ("error server: socket");
    	    syslog(epriority, "error server: socket");
            //closelog();
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            printf ("error setsockopt");
    	    syslog(epriority, "error setsockopt");
            closelog();
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            printf ("error server: bind");
    	    syslog(epriority, "error server: bind");
            continue;
        }
        break;
    }


    freeaddrinfo(servinfo); // all done with this structure, necessary info is on p

    if (p == NULL)  {
        printf ("server: failed to bind\n");
    	syslog(epriority, "server: failed to bind\n");
        closelog();
        exit(1);
    }
if (daemon_act==true)
{
	pid=fork();
        if (pid < 0)
        {
        	exit(EXIT_FAILURE);
        }
	if (pid >0)
	{
		//parent process
		//shut down
		closelog();
		exit(0);
	} 
	if (pid == 0)
	{
		//child process turn it into daemon
		setsid();
    		chdir("/");
    		stdin = fopen("/dev/null", "r");
		stdout = fopen("/dev/null", "w+");
		stderr = fopen("/dev/null", "w+");
	}
}
    if (listen(sockfd, BACKLOG) == -1) {
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        printf ("server: sigaction setup failed\n");
    	syslog(epriority, "server: sigaction setup failed\n");
        closelog();
        exit(1);
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        printf ("server: sigaction setup failed\n");
    	syslog(epriority, "server: sigaction setup failed\n");
        closelog();
        exit(1);
    }    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        printf ("server: sigaction setup failed\n");
    	syslog(epriority, "server: sigaction setup failed\n");
        closelog();
        exit(1);
    }
    //intialize file (truncate or create)
	if ((fp = fopen(stfile, "w+")) == NULL)
	{
		printf ("path does not exist or is locked so file cannot be created or truncated\n");
		syslog(epriority, "path does not exist or is locked so file cannot be created or truncated\n");
		closelog();
		return 1;
	}
	else
	{
		printf ("file %s created/truncated \n", stfile );
		syslog(spriority, "file %s created/truncated \n", stfile );
		fclose(fp);
	}
    printf("server: waiting for connections...\n");
    //initialize package buffer size here so it is assured during all transmissions minimum size of package buffer is kept  
    pbufs=STARTPSIZE;
    while(nomore_connect==false) 
    {  // main accept() loop
    	printf("server: wait for next connection\n");
       	fds.fd = sockfd;
  	fds.events = POLLIN;
  	printf("Waiting on poll()...\n");
    	pollres = 0; 
    	while ((pollres <= 0) && (nomore_connect==false))
        {
        	pollres = poll(&fds, nfds, timeout);
        }
        if (nomore_connect==true)
        {
            if (pollres > 0)
            {
            	printf("start shutdown, disregard connection requests waiting in queue \n");
            }
            continue;
        }
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) 
        {
            printf ("server: accept failed\n");
    	    syslog(epriority, "server: accept failed\n");
            continue;
        }
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf ("Accepted connection from %s\n", s);
        syslog(spriority, "server: got connection from %s\n", s);
	    //wait for received packages with the current sesssion until either end of session is signalled (numbytes==0) of error occrurs (numbytes<0)
	    //intitalize reception buffers and counters
	    pcounter=0;
	    rptr = (char*) malloc(MAXRECSIZE * sizeof(char));
	    pptr = (char*) malloc(pbufs * sizeof(char));
	    *pptr='\0';
	    while ((numbytes = recv(new_fd, rptr, MAXRECSIZE-1, 0)) > 0)
	    {
    		//add retrieved data to packet buffer
    		//add end of string character to end of reception so string operations work
		*(rptr + numbytes)='\0';
		//printf("server: received data %s \n",rptr);
		//expand package buffer if necessary (compare with numbytes+1 because of end of string character)
		while ((pcounter+numbytes+1) > pbufs)
		{
			//double size of package buffer and reallocate
			pbufs=pbufs*2;
			pptr = realloc(pptr, pbufs * sizeof(char));
			if (pptr != NULL)
			{
				printf("server: successfully expanded package buffer to size %d \n", pbufs);
			}
			else
			{
				printf ("packet buffer expansion failed\n");
				syslog(epriority, "packet buffer expansion failed\n");
				return 1;
			}
		}
		//concatenate strings into package buffer pptr and update counters, print package data received so far
		strcat(pptr,rptr);
		pcounter=pcounter+numbytes;
		//printf("server: received package data so far %s, counter value is %d \n",pptr, pcounter);
		//check if end of package newline character has been received
		if (*(rptr + numbytes - 1) == '\n')
		{	
			//printf("server: received complete package '%s'\n",pptr);
			
			//package received completely, append to file and send back complete file
			
			if ((fp = fopen(stfile, "a+")) == NULL)
			{
				printf ("path does not exist or is locked so file cannot be created or appended\n");
				syslog(epriority, "path does not exist or is locked so file cannot be created or appended\n");
				closelog();
				return 1;
			}
			else
			{
				//printf ("file %s opened to append package content %s \n", stfile, pptr );
				syslog(spriority, "file %s opened to append package content %s \n", stfile, pptr );
				fwrite(pptr, 1, pcounter, fp);
				fflush;
				fclose(fp);					
				//reset overall package counters and set end string marker
				*pptr='\0';
				pcounter=0;
				//read out the fie line by line and send back to client
				fp = fopen(stfile, "r");
				//we dont check fp as we scuccessfully just wrote to the file
				//read all contents untile end of file
				while (fread(&rchar,1, 1, fp) == 1) 
				{
					/* append byte by byte */
					// expand buffer if required (possible usecase as you read lines from previous transmissions)
					//compare with pcounter+1 because of end of string character required after end of line
					if ((pcounter+1) > pbufs)
					{
						//double size of package buffer and reallocate
						pbufs=pbufs*2;
						pptr = realloc(pptr, pbufs * sizeof(char));
						if (pptr != NULL)
						{
							printf("server: successfully expanded package buffer to size %d \n", pbufs);
						}
						else
						{
							printf ("packet buffer expansion failed\n");
							syslog(epriority, "packet buffer expansion failed\n");
							return 1;
						}
					}
					//add character just read from file to buffer
					*(pptr + pcounter)=rchar;
					pcounter++;
					//if end of line is reached, add end of string character, and send to client
					//them reset counter to zero and buffer string to empty string "" (but size of buffer remains) 
					if (rchar=='\n')
					{
						//end of line reached, finish and send package, then reset buffer
						*(pptr + pcounter)='\0';
						//printf("server: send back complete package line retrieved from file '%s'\n",pptr);
						send(new_fd,pptr,pcounter,0);
						*pptr='\0';
						pcounter=0;	
					}
					
				}
				fclose(fp);
				*pptr='\0';
				pcounter=0;
			}
		}
            }
            if (numbytes==0)
            {
	       	printf ("Closed connection from %s\n", s);
    	    	syslog(spriority, "Closed connection from %s\n", s);
            }
            else
            {
	    	printf ("server: receive failed with return %d \n", numbytes);
    	    	syslog(epriority, "server: receive failed with return %d \n", numbytes);
	 	exit(1);
	    }
	    //finish child connection, free buffers
            free(rptr);
            free(pptr);
            close(new_fd);
            //exit(0);
       
    }
	    
if(nomore_connect==true)
{
	//wait for all connection operations to terminate
	//printf ("parent blocks new connections and waits for all ongoing child connection operations to end\n");
        //waitpid(-1, NULL, 0);
        //printf ("all child connection operations done\n");
	//close open socket
	close(sockfd); 	
	//remove temp file
	remove(stfile);
	//free(rptr);
        //free(pptr);
        //nomore_connect==false;
}
closelog();
return 0;
}




