/*
 A simple client in the internet domain using TCP
 Usage: ./client hostname port (./client 192.168.0.151 10000)
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <time.h>
#include <vector>
#include <map>
#include <fstream>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include "packet.h"
using namespace std;

int sockfd;
int portno, n;
struct sockaddr_in serv_addr;
struct hostent *server;  // contains tons of information, including the server's IP address
int32_t currSeqNum, currAckNum;

void error(const char* msg)
{
    perror(msg);
    exit(0);
}

void printMessage(Packet p, string sendOrRec)
{
  if (sendOrRec=="send") {
    fprintf(stderr, "%s\n", );
  }
  else {

  }
}

void setUpSocket()
{
  portno = atoi(argv[2]);
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // create a new socket
  if (sockfd < 0)
      error("ERROR opening socket");

  server = gethostbyname(argv[1]);  // takes a string like "www.yahoo.com", and returns a struct hostent which contains information, as IP address, address type, the length of the addresses...
  if (server == NULL) {
      fprintf(stderr, "ERROR, no such host\n");
      exit(0);
  }

  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET; //initialize server's address
  bcopy((char *)server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(portno);
  socklen_t servlen = sizeof(serv_addr);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
       fprintf(stderr, "Usage: %s hostname port\n", argv[0]);
       exit(0);
    }

    setUpSocket();

    currSeqNum = rand() % MAXSEQNUM + 1;      //initial seq num
    Packet syn;
    syn.setPorts(0, 0);        //change this to something useful
    syn.setSeqNum(currSeqNum);
    syn.setData("syn", 3);      //get rid of this
    syn.setPacketType(SYN);

    fprintf(stderr, "%s\n", "SENDING SYN");
    n = sendto(sockfd, (char*)&syn, MAXPACKETSIZE, 0, (struct sockaddr*)&serv_addr, servlen);  // write to the socket
    if (n < 0) {
      error("ERROR writing to socket");
    }

    Packet synack;
    n = recvfrom(sockfd, (char*)&synack, MAXPACKETSIZE, 0, (struct sockaddr*)&serv_addr, &servlen);  // read from the socket
    if (n < 0) {
      error("ERROR reading from socket");
    }
    printf("Here is the message: %s\n", synack.getData());

    close(sockfd);  // close socket

    return 0;
}
