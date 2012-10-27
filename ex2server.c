/* A threaded server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include <errno.h>

#define BUFFERLENGTH 256
/* displays error messages from system calls */
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int checkvalid (char *entry) {

  if ((!isalnum(entry[0])) || (strchr(entry,':') == NULL)) {
    printf ("Failed at condition 1\n");
    return -1;
  }

  int i = 0;
  while (entry[i] != ':') {
    if (!isalnum(entry[i])) {
      printf ("Failed at condition 2\n");
      return -1;
    }
    i++;
  }
  //i++;
  //while (entry[i] != '\0') {
  //  if (!isprint(entry[i])) {
  //    printf ("Failed at condition 3");
  //    return -1;
  //  }
  //  i++;
  //}
  printf ("Passed\n");
  return 1;
}

FILE *p_file;
char *filename;

int returnValue; /* not used; need something to keep compiler happy */
pthread_mutex_t mut; /* the lock */

void
sig_chld(int signo) //if the  child dies, we'll handle this here
{
	pid_t	pid;
	int		stat;

	pid = wait(&stat);
	printf("child %d terminated\n", pid);
	return;
}

/* the procedure called for each request */
void *processRequest (void *args) {
  signal(SIGPIPE, SIG_IGN); //we'll attempt to ignore SIGPIPE / Broken Pipe
  int *newsockfd = (int *) args;
  char buffer[BUFFERLENGTH];
  int n;
  while (1) {
    
    n = read (*newsockfd, buffer, BUFFERLENGTH -1);
    if (n < 0) {
      if (errno == ECONNRESET){ //See, the thing is, by the time we've got connection reset by peer, (which is what 104 means)
    	  int rv=-1;  // The socket's already dead, so we can't call close on it. 
        //free(*newsockfd); //we can free it.. I think
	      pthread_exit(&rv); //then terminate the thread, and return to parent. 
      }
      //Otherwise we can do stuff here to wrap up.

      error ("ERROR reading from socket");
      shutdown(*newsockfd,SHUT_RDWR);
      close(*newsockfd);
      free (*newsockfd);
      printf("Errno: %d\n",errno);
      returnValue = -1;  /* cannot guarantee that it stays constant */
      pthread_exit (&returnValue);
    }

    //if (strchr(buffer,EOF) != NULL) {

      printf ("Submitted log: %s\n",buffer);
      pthread_mutex_lock (&mut); /* lock exclusive access to variable p_file */
      printf ("Locked.\n");

      if (checkvalid(buffer) == 1) {
        p_file = fopen(filename, "a");
        fprintf(p_file, "%s",buffer);
        fclose(p_file);
        //bzero (buffer, BUFFERLENGTH);
        printf ("Good.\n");
        n = sprintf (buffer, "Entry accepted.\n");
        n = write (*newsockfd, buffer, BUFFERLENGTH);
        if (n < 0){
         error ("ERROR writing to socket");
         shutdown(*newsockfd,SHUT_RDWR);
      close(*newsockfd);
      free (*newsockfd);
      printf("Errno: %d\n",errno);
      returnValue = -1;  /* cannot guarantee that it stays constant */
      pthread_exit (&returnValue);
       }
      } else {
        bzero (buffer, BUFFERLENGTH);
        printf ("Bad.\n");
        n = sprintf (buffer, "Entry malformed and not accepted.\n");
        n = write (*newsockfd, buffer, BUFFERLENGTH);
        if (n < 0)
         error ("ERROR writing to socket");
      }

      pthread_mutex_unlock (&mut); /* release the lock */
      printf ("Unlocked.\n");
      //bzero (buffer, BUFFERLENGTH);
    //} else {
    //  printf("received EOF");
    //  close (*newsockfd); /* important to avoid memory leak */  
    //  free (newsockfd);

    //  returnValue = 0;  /* cannot guarantee that it stays constant */
    //  pthread_exit (&returnValue);
    //}
  }
  close (*newsockfd); /* important to avoid memory leak */  
  free (newsockfd);

  returnValue = 0;  /* cannot guarantee that it stays constant */
  pthread_exit (&returnValue);
}



int main(int argc, char *argv[]) {
  signal(SIGPIPE, SIG_IGN); //we'll attempt to ignore SIGPIPE / Broken Pipe
  signal(SIGCHLD, sig_chld); //we'll also handle sigchild when a child dies. 

  socklen_t clilen;
  int sockfd, portno;
  char buffer[BUFFERLENGTH];
  struct sockaddr_in serv_addr, cli_addr;
  pthread_t *server_thread;
  int result;

  if (argc < 3) {
    fprintf (stderr,"Usage: ex2server filename port\n");
    exit(1);
  }

  filename = argv[1];
  p_file = fopen(filename, "a");
  if (p_file == NULL) {
    fprintf (stderr, "The specified file is not writable.\n");
    exit (1);
  }
  fclose(p_file);
     
  /* create socket */
  sockfd = socket (AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  bzero ((char *) &serv_addr, sizeof(serv_addr));
  portno = atoi(argv[2]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons (portno);

  /* bind it */
  if (bind(sockfd, (struct sockaddr *) &serv_addr,
          sizeof(serv_addr)) < 0) 
          error("ERROR on binding");

  /* ready to accept connections */
  listen (sockfd,5);
  clilen = sizeof (cli_addr);

  /* now wait in an endless loop for connections and process them */
  while (1) {
   
    int *newsockfd; /* allocate memory for each instance to avoid race condition */
    pthread_attr_t pthread_attr; /* attributes for newly created thread */

    newsockfd  = malloc (sizeof (int));
    if (!newsockfd) {
      fprintf (stderr, "Memory allocation failed!\n");
      exit (1);
    }

    /* waiting for connections */
    *newsockfd = accept( sockfd, 
     (struct sockaddr *) &cli_addr, 
      &clilen);
    if (*newsockfd < 0) 
     error ("ERROR on accept");
    bzero (buffer, BUFFERLENGTH);

    /* create separate thread for processing */
    server_thread = malloc (sizeof (pthread_t));
    if (!server_thread) {
      fprintf (stderr, "Couldn't allocate memory for thread!\n");
      exit (1);
    }

    if (pthread_attr_init (&pthread_attr)) {
      fprintf (stderr, "Creating initial thread attributes failed!\n");
      exit (1);
    }

    if (pthread_attr_setdetachstate (&pthread_attr, PTHREAD_CREATE_DETACHED)) {
   	  fprintf (stderr, "setting thread attributes failed!\n");
      exit (1);
    }
    result = pthread_create (server_thread, &pthread_attr,(void *) &processRequest, (void *) newsockfd);
    if (result != 0) {
      fprintf (stderr, "Thread creation failed!\n");
      exit (1);
    }

  }
  return 0; 
}
