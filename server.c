/* 
This code primarily comes from 
http://www.prasannatech.net/2008/07/socket-programming-tutorial.html
and
http://www.binarii.com/files/papers/c_sockets.txt
 */
#include "linkedlist.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <float.h>


/* global variables */
char msg[500];
linkedlist* list = NULL;
linkedlist* recentTen = NULL;
double currentReading = 0;
double total = 0;
int count = 0;
double low = 56.7;
double high = -273.15;
double avg = 0;
char* webFile = NULL;
int arduino_fd;
int thread_index = 0;
int recentTen_count = 0;
int connected = 0;
pthread_mutex_t lock;

/* function headers */
int start_server(int PORT_NUMBER);
int fileReader(char* fileName);

/* functions */
void configure(int fd) {
  struct  termios pts;
  tcgetattr(fd, &pts);
  cfsetospeed(&pts, 9600);   
  cfsetispeed(&pts, 9600);   
  tcsetattr(fd, TCSANOW, &pts);
}


/* a wrapper function for the start_server function,
   so that it is thread-friendly */
void* httpRequest(void* port) {
  int port_number = *(int*)port;
  int ret = start_server(port_number);
  return port;
}


/* fuction that constantly awaits input from terminal,
   if it's "q", force shut down the server */
void* getInput (void* list) {
  char str[100] = "\0";
  
  while (strcmp(str, "q") != 0) {
    scanf("%s", str);
    if (strcmp(str, "q") == 0) {
      fprintf(stderr, "%s\n", "User force quit");
      //pthread_mutex_unlock(&lock); 
      exit(1);
    }
  }
  pthread_exit(NULL);
}

/* function that handles incoming http request,
   determines the type of the request,
   and what to send back to the browser and hardware */
void* requestHandler(void* data) {
  char* d = (char*)data;
  char* req = strtok(d, "@");
  //get the fd
  char* fd = strtok(NULL, "@");
  int serverFd = atoi(fd);
  printf("%d\n", serverFd);
  char* delim = " ";
   
  if (*req == 'P') {
    //if the incoming request is a POST request 
    char* token = strtok(req, "\n");
    for (int i = 0; i < 12; i++) {
       token = strtok(NULL, "\n");
    }  
    printf("token is: %s\n", token);
    if (strcmp(token, "group2=CTemp") == 0) {
      send(serverFd, webFile, strlen(webFile), 0);
      char buf[10];
      strcpy(buf, "c");
      //pthread_mutex_lock(&lock); 
      int byte_wrote = write(arduino_fd, buf, 1); 
      //pthread_mutex_unlock(&lock); 
      close(serverFd);
    } else if (strcmp(token, "group2=FTemp") == 0) {
      send(serverFd, webFile, strlen(webFile), 0);
      char buf[10];
      //pthread_mutex_lock(&lock);
      double reading_f = currentReading * 9/5 + 32;
      sprintf(buf, "%5f", reading_f);
      int byte_wrote = write(arduino_fd, buf, 5); 
      //pthread_mutex_lock(&lock);
      close(serverFd);
    } else if (strcmp(token, "group3=standBy") == 0) {
      char buf[5];
      strcpy(buf, "f");
      //pthread_mutex_lock(&lock);
      int byte_wrote = write(arduino_fd, buf, 1); 
      //pthread_mutex_unlock(&lock);
      send(serverFd, webFile, strlen(webFile), 0);
      close(serverFd);
    } else if (strcmp(token, "group3=resume") == 0) {
      char buf[5];
      strcpy(buf, "o");
      //pthread_mutex_lock(&lock);
      int byte_wrote = write(arduino_fd, buf, 1); 
      //pthread_mutex_unlock(&lock);
      send(serverFd, webFile, strlen(webFile), 0);
      close(serverFd);
    } else if (strcmp(token, "group4=red") == 0) {
      char buf[5];
      strcpy(buf, "r");
      //pthread_mutex_lock(&lock);
      int byte_wrote = write(arduino_fd, buf, 1);  
      //pthread_mutex_unlock(&lock);
      send(serverFd, webFile, strlen(webFile), 0);
      close(serverFd);
    } else if (strcmp(token, "group4=blue") == 0) {
      char buf[5];
      strcpy(buf, "b");
      //pthread_mutex_lock(&lock);
      int byte_wrote = write(arduino_fd, buf, 1); 
      //pthread_mutex_unlock(&lock); 
      send(serverFd, webFile, strlen(webFile), 0);
      close(serverFd);
    } else if (strcmp(token, "group4=green") == 0) {
      char buf[5];
      strcpy(buf, "g");
      //pthread_mutex_lock(&lock);
      int byte_wrote = write(arduino_fd, buf, 1); 
      //pthread_mutex_unlock(&lock); 
      send(serverFd, webFile, strlen(webFile), 0);
      close(serverFd);
    } 
  } else if (*req == 'G') {
    // if the incoming request is GET request 
    char* token = strtok(req, delim);
    token = strtok(NULL, delim);
    char sum[20];
    strncpy(sum, token, 18);
    char* num = strstr(token, "=");
    if (token == NULL) {
      //continue w/o any processing;
    } else if (strcmp(token, "/index-2.html") == 0) {
      //fileReader(token+1);
      send(serverFd, webFile, strlen(webFile), 0);
      close(serverFd);
    } else if (strcmp(token, "/begin=update") == 0) {
      char reply[200];
      printf("%s\n", "entered correct if, processing...");
      //pthread_mutex_lock(&lock);
      printf("%s\n", "locked");
      sprintf(msg, "{ \"currentReading\": %.4f, \"low\": %.4f, \"high\": %.4f, \"avg\": %.4f }", currentReading, low, high, avg);
      sprintf(reply, "HTTP/1.1 200 OK\nContent-Type: text/json\n\n%s", msg);
      //pthread_mutex_unlock(&lock);
      printf("%s\n", "unlocked");
      printf("reply is %s\n", reply);
      send(serverFd, reply, strlen(reply), 0);
      close(serverFd);
    } else if (strcmp(token, "/begin=draw") == 0) {
      printf("%s\n", "entered correct if, processing...");
      double tenReading[10] = {0};
      int array_index = 0;
      char reply[1000];
      //pthread_mutex_lock(&lock);
      printf("%s\n", "locked");
      node* temp = recentTen->head;
      while (temp != NULL) {
        tenReading[array_index] = *(double*)temp->value;
        printf("%f\n", tenReading[array_index]);
        array_index++;
        temp = temp->next;
      }
      sprintf(msg, "{ \"reading1\": %.4f, \"reading2\": %.4f, \"reading3\": %.4f, \"reading4\": %.4f, \"reading5\": %.4f, \"reading6\": %.4f, \"reading7\": %.4f, \"reading8\": %.4f, \"reading9\": %.4f, \"reading10\": %.4f }", tenReading[0], tenReading[1], tenReading[2], tenReading[3], tenReading[4], tenReading[5], tenReading[6], tenReading[7], tenReading[8], tenReading[9]);
      sprintf(reply, "HTTP/1.1 200 OK\nContent-Type: text/json\n\n%s", msg);
      //pthread_mutex_unlock(&lock);
      printf("%s\n", "unlocked");
      printf("reply is %s\n", reply);
      send(serverFd, reply, strlen(reply), 0);
      close(serverFd);
    } else if (strcmp(sum, "/addition_endpoint") == 0) {
      char* number_to_send = num+1;
      send(serverFd, webFile, strlen(webFile), 0);
      //pthread_mutex_lock(&lock);
      int byte_wrote = write(arduino_fd, number_to_send, strlen(number_to_send));  
      //pthread_mutex_unlock(&lock);
      close(serverFd);
    } else {
      //do nothing
    } 
  }
  return data;
}

int start_server(int PORT_NUMBER) {
  // structs to represent the server and client
  struct sockaddr_in server_addr,client_addr;    
  int sock; // socket descriptor

  // 1. socket: creates a socket descriptor that you later use to make other system calls
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket");
    exit(1);
    }
  int temp;
  if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&temp,sizeof(int)) == -1) {
    perror("Setsockopt");
    exit(1);
    }

  // configure the server
  server_addr.sin_port = htons(PORT_NUMBER); // specify port number
  server_addr.sin_family = AF_INET;         
  server_addr.sin_addr.s_addr = INADDR_ANY; 
  bzero(&(server_addr.sin_zero),8); 
    
  // 2. bind: use the socket and associate it with the port number
  if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
  perror("Unable to bind");
  exit(1);
  }

  //array of threads handling simultaneous requests
  pthread_t t_array[50];
   
  // 3. listen: indicates that we want to listen to the port to which we bound; second arg is number of allowed connections
  if (listen(sock, 1) == -1) {
  perror("Listen");
  exit(1);
  }

  // once you get here, the server is set up and about to start listening
  printf("\nServer configured to listen on port %d\n", PORT_NUMBER);
  fflush(stdout);

  while (1) {
    // 4. accept: wait here until we get a connection on that port
    int sin_size = sizeof(struct sockaddr_in);
    int fd = accept(sock, (struct sockaddr *)&client_addr,(socklen_t *)&sin_size);
    if (fd != -1) {
      printf("Server got a connection from (%s, %d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

      // buffer to read data into
      char request[1024];

      // 5. recv: read incoming message (request) into buffer
      int bytes_received = recv(fd,request,1024,0);
      // null-terminate the string
      request[bytes_received] = '\0';
      // print it to standard out
      printf("This is the incoming request:\n%s\n", request);

      // data contains the request and the fd 
      char data[2000];
      sprintf(data, "%s@%d", request, fd);
      //printf("data is: %s\n", data);

      //for each request, start a new thread 
      thread_index++;
      if (thread_index >49) thread_index = 0;
      // 6. send: send the outgoing message (response) over the socket
   	  // note that the second argument is a char*, and the third is the number of chars 
      pthread_create(&t_array[thread_index], NULL, &requestHandler, &data);
      printf("thread_index is%d\n", thread_index);
      //requestHandler((void*)data);
    } 
    // 7. close: close the connection
    // close(fd);
    printf("Server closed connection\n");   
  }
  // 8. close: close the socket
  close(sock);
  printf("Server shutting down\n");

  return 0;
} 

/* function that keep reading temperature info 
   from the arduino hardware, 
   appends it to the linkedlist, 
   while updating the low, high and average */
void* readArduino (void* fileName) {
  char* file_name = (char*) fileName;

  while (1) {

    int fd = open(file_name, O_RDWR | O_NOCTTY | O_NDELAY);
    pthread_mutex_lock(&lock);
    arduino_fd = fd;
    pthread_mutex_unlock(&lock);
    if (fd < 0) {
   	  perror("Could not open file\n");
   	  pthread_mutex_lock(&lock);
      connected = 0;
      currentReading = -100;
      pthread_mutex_unlock(&lock);
      sleep(2);
      continue;
    } else {
      pthread_mutex_lock(&lock);	
      connected = 1;
      pthread_mutex_unlock(&lock);
      printf("Successfully opened %s for reading and writing\n", file_name);
    }

  	configure(fd);

    char buf[1];
    char temp[100];
    char reading[100];
    int index = 0;
    int fail = 0;

    while (connected == 1) {	  	
      pthread_mutex_lock(&lock);  
      int bytes_read = read(fd, buf, 1);   

      if (bytes_read > 0) {
        if (buf[0] != '\n') {
          temp[index] = buf[0];
          index++;
        } else {
          temp[index] = '\0';   
          if (strlen(temp) != 0) {
            strcpy(reading, temp);
            sscanf(reading, "The temperature is %lf degrees C", &currentReading);
            //printf("%s\n", reading);
            if (currentReading == 0) {
              break;
            }
            double* val = malloc(sizeof(double));
            *val = currentReading;
            add_to_tail(val, recentTen);
            recentTen_count++;
            if (recentTen_count > 10) {
              remove_from_front(recentTen);
              recentTen_count = 10;
            }
            add_to_tail(val, list);
            count++;
            if (count > 3600) {
              double old_value = remove_from_front(list);
              count = 3600;
              total -= old_value;
            }
            total += currentReading;
            if (low == 0) {
              low = currentReading;
            } else {
              if (currentReading < low) low = currentReading;
            }
            if (currentReading > high) high = currentReading;
            avg = total/count; 
          }
          index = 0;
        }
        fail = 0;
      } else if (bytes_read==0) {
     	//printf("fail is %d\n", fail);
        sleep(1);
        fail++;
        if (fail == 8) {
          connected = 0;
          pthread_mutex_unlock(&lock); 
          break;
        }
      } else {
      	pthread_mutex_unlock(&lock); 
      	continue;
      }   
      pthread_mutex_unlock(&lock); 
   	}
   	
    close(fd);
  } 
  pthread_exit(NULL); 
}

/* funtion that reads the html file,
   and prints it into one string */
int fileReader (char* fileName) {
  FILE *fp;
  long size;
  char *buffer;
  char *header = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n\0";

  fp = fopen (fileName, "r");
  if( !fp ) perror(fileName), exit(1);

  fseek(fp , 0L , SEEK_END);
  size = ftell(fp);
  rewind(fp);

  buffer = malloc(size+1);
  if( !buffer ) fclose(fp), fprintf(stderr, "%s\n", "memory alloc fails"), exit(1);

  if(fread( buffer , size, 1 , fp) != 1) 
  fclose(fp),free(buffer), fprintf(stderr, "%s\n", "read fails"), exit(1);

  webFile = malloc(strlen(buffer) + strlen(header) + 1);
  sprintf(webFile, "%s%s\n", header, buffer);

  fclose(fp);
  free(buffer);
  return 0;
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
      printf("Please specify the name of the serial port (USB) device file!\n");
      exit(0);
    }
    fileReader("index-2.html");

    list = malloc(sizeof(linkedlist)); 
    list->head = NULL;
    recentTen = malloc(sizeof(linkedlist));
    recentTen->head = NULL;

    pthread_t t1, t2, t3, t4;
    void* r1 = NULL, *r2 = NULL, *r3=NULL, *r4=NULL;

    char* file_name = argv[2];
    pthread_create(&t2, NULL, &readArduino, file_name);

    int port_number = atoi(argv[1]);
    if (port_number <= 1024) {
      printf("\nPlease specify a port number greater than 1024\n");
      exit(-1);
    }

    pthread_create(&t3, NULL, &getInput, list);
    pthread_create(&t1, NULL, &httpRequest, &port_number);
    
    pthread_join(t3, &r3);
    pthread_join(t2, &r2);
    pthread_join(t1, &r1);

    if (list->head != NULL) {
		node* head = list->head;
		node* temp;
		while (head != NULL) {
			temp = head;
			head = head->next;
			free(temp->value);
			free(temp);	
		}	
	}
	if (recentTen->head != NULL) {
		node* head = recentTen->head;
		node* temp;
		while (head != NULL) {
			temp = head;
			head = head->next;
			free(temp->value);
			free(temp);	
		}	
	}
	free(recentTen);
	free(list);
	free(webFile);
  return 0;
}

