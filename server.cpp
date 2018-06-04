/* A simple server in the internet domain using TCP
   The port number is passed as an argument
   This version runs forever, forking off a separate
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>  /* signal name macros, and the kill() prototype */
#include <iostream>
#include <string>
#include <time.h>
#include <vector>
#include <map>
#include <fstream>
#include <fcntl.h>
#include "packet.h"
using namespace std;

int sockfd, portno;
struct sockaddr_in serv_addr, cli_addr;
socklen_t clilen = sizeof(cli_addr);
int32_t currSeqNum, currAckNum;

void error(const char *msg)
{
  perror(msg);
  exit(1);
}

void setUpSocket()
{
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // create socket
  if (sockfd < 0)
      error("ERROR opening socket");
  memset((char *) &serv_addr, 0, sizeof(serv_addr));  // reset memory

  // fill in address info
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    error("ERROR on binding");
  }
}

int main(int argc, char *argv[])
{

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    setUpSocket();

    Packet syn;
    int n = recvfrom(sockfd, (char*)&syn, MAXPACKETSIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
    if (n < 0) {
      error("ERROR on recvfrom");
    }

    printf("Here is the message: %s\n", syn.getData());

    Packet synack;
    synack.setAckNum(1);
    synack.setPorts(0, 0);        //change this to something useful
    synack.setSeqNum(0);
    synack.setData("synack", 6);
    synack.setPacketType(SYNACK);

    fprintf(stderr, "%s\n", "SENDING SYNACK");
    n = sendto(sockfd, (char*)&synack, MAXPACKETSIZE, 0, (struct sockaddr *) &cli_addr, clilen);
    if (n < 0) error("ERROR writing to socket");

    close(sockfd);

    return 0;
}
