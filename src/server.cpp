//===----- server.cpp - Server Sockets --------------------------------------===//
////
//// Copyright (c) 2017 Ciro Ceissler
////
//// See LICENSE for details.
////
////===----------------------------------------------------------------------===//
////
//// This server socket receive only command line argument, the port number,
//// and works with a simple specific protocol.
////
////===----------------------------------------------------------------------===//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>

#define BUFFER_SIZE 1024

void dostuff(int); /* function prototype */

void error(const char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[]) {
   int sockfd, newsockfd, portno, pid;
   socklen_t clilen;
   struct sockaddr_in serv_addr, cli_addr;

   if (argc < 2) {
     fprintf(stderr,"ERROR, no port provided\n");
     exit(1);
   }
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd < 0)
      error("ERROR opening socket");
   bzero((char *) &serv_addr, sizeof(serv_addr));
   portno = atoi(argv[1]);
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);
   if (bind(sockfd, (struct sockaddr *) &serv_addr,
         sizeof(serv_addr)) < 0)
     error("ERROR on binding");
   listen(sockfd,5);
   clilen = sizeof(cli_addr);
   while (1) {
     newsockfd = accept(sockfd,
         (struct sockaddr *) &cli_addr, &clilen);
     if (newsockfd < 0)
       error("ERROR on accept");
     pid = fork();
     if (pid < 0)
       error("ERROR on fork");
     if (pid == 0)  {
       close(sockfd);
       dostuff(newsockfd);
       exit(0);
     }
     else close(newsockfd);
   } /* end of while */
   close(sockfd);
   return 0; /* we never get here */
}

int fpga_cmd(int sock) {
  ssize_t n;
  char buffer[1];

  printf("[fpga_cmd]\n");

  bzero(buffer, 1);

  n = read(sock, buffer, 1);
  if (n < 0) error("ERROR reading from socket");

  n = write(sock,"ack",3);
  if (n < 0) error("ERROR writing to socket");

  return buffer[0];
}

void fpga_write(int sock) {
  ssize_t n;
  char buffer[BUFFER_SIZE];

  printf("[fpga_write]\n");

  do {
    bzero(buffer, BUFFER_SIZE);
    n = read(sock, buffer, BUFFER_SIZE);

    if (n < 0) error("ERROR reading from socket");

    for (int i = 0; i < n/sizeof(int); i++) {
      int tmp = 0;

      tmp |= (0xff & buffer[i*4]);
      tmp |= (0xff & buffer[i*4 + 1]) << 8;
      tmp |= (0xff & buffer[i*4 + 2]) << 16;
      tmp |= (0xff & buffer[i*4 + 3]) << 24;

      printf("[fpga_write] buffer[%d] = %d\n", i, tmp);
    }

  } while((BUFFER_SIZE - n) == 0);

  n = write(sock, "ack", 3);
  if (n < 0) error("ERROR writing to socket");
  printf("[fpga_write] send ack!\n");
}

void fpga_read(int sock) {
  ssize_t n;
  char buffer[BUFFER_SIZE];
  int64_t length;

  int test[100];

  printf("[fpga_read]\n");

  bzero(buffer, BUFFER_SIZE);
  n = read(sock, (void *) &length, 8);
  if (n < 0) error("ERROR reading from socket");

  n = write(sock, "ack", 3);
  if (n < 0) error("ERROR writing to socket");
  printf("[fpga_read] send ack!\n");

  printf("[fpga_read] length = %ld\n", length);

  for (int i = 0; i < 100; i++) {
    test[i] = 200 + i;
  }

  n = write(sock, (void*) &test, length);
  if (n < 0) error("ERROR writing to socket");

  bzero(buffer, BUFFER_SIZE);
  n = read(sock, buffer, 3);
  if (n < 0) error("ERROR writing to socket");
  printf("[fpga_read] recv ack!\n");
}

void fpga_go(int sock) {
  printf("[fpga_go]\n");
}

void fpga_program(int sock) {
  ssize_t n;
  char buffer[BUFFER_SIZE];

  printf("[fpga_program]\n");

  do {
    bzero(buffer, BUFFER_SIZE);
    n = read(sock, buffer, BUFFER_SIZE);

    if (n < 0) error("ERROR reading from socket");
  } while((BUFFER_SIZE - n) == 0);

  printf("[fpga_program] buffer = %s\n", buffer);

  n = write(sock, "ack", 3);
  if (n < 0) error("ERROR writing to socket");
  printf("[fpga_program] send ack!\n");
}

/******** DOSTUFF() *********************
 There is a separate instance of this function
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void dostuff (int sock) {
   char cmd;

   for(;;) {
     cmd = fpga_cmd(sock);

     switch(cmd) {
       case 'w': fpga_write(sock);   break;
       case 'r': fpga_read(sock);    break;
       case 'p': fpga_program(sock); break;
       case 'g': fpga_go(sock);      break;
     }
   }
}

