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
#include <cerrno>
#include "packet.h"
using namespace std;

int sockfd;
int portno;
struct sockaddr_in serv_addr;
struct hostent *server;  // contains tons of information, including the server's IP address
fd_set sock;
struct timeval tv;
string file_req_name;
socklen_t servlen;
int32_t currSeqNum_client, currAckNum_client;
vector<Packet*> packets;
int debug=1;

void writePacketsToFile();
void receivePackets();

void error(const char *msg)
{
  int err = errno;
  fprintf(stderr, "Error due to %s\n", msg);
  fprintf(stderr, "Error: %s\n", strerror(err));
  fprintf(stderr, "Error Number: %i\n", err);
  exit(1);
}

void printMessage(Packet* p, string sendOrRec)
{
  if (debug) {
    if (sendOrRec=="send") {
      fprintf(stderr, "Sending packet SEQ: %i ACK: %i WIN: %i TYPE: %i TRANS: %i\n", p->getSeqNum(), p->getAckNum(), p->getWinSize(), p->getPacketType(), p->getTransmissions());
    }
    else {
      fprintf(stderr, "Receiving packet SEQ: %i ACK: %i WIN: %i TYPE: %i TRANS: %i\n", p->getSeqNum(), p->getAckNum(), p->getWinSize(), p->getPacketType(), p->getTransmissions());
    }
  }
  // if (sendOrRec=="send") {
  //   if (p->getTransmissions()>1) {
  //     if (p->getPacketType()==SYN) {
  //       fprintf(stderr, "Sending packet %i %i %s %s\n", p->getSeqNum(), p->getWinSize(), "Retransmission", "SYN");
  //     }
  //     else if (p->getPacketType()==FIN) {
  //       fprintf(stderr, "Sending packet %i %i %s %s\n", p->getSeqNum(), p->getWinSize(), "Retransmission", "FIN");
  //     }
  //     else {
  //       fprintf(stderr, "Sending packet %i %i %s\n", p->getSeqNum(), p->getWinSize(), "Retransmission");
  //     }
  //   }
  //   else {
  //     if (p->getPacketType()==SYN) {
  //       fprintf(stderr, "Sending packet %i %i %s\n", p->getSeqNum(), p->getWinSize(), "SYN");
  //     }
  //     else if (p->getPacketType()==FIN) {
  //       fprintf(stderr, "Sending packet %i %i %s\n", p->getSeqNum(), p->getWinSize(), "FIN");
  //     }
  //     else {
  //       fprintf(stderr, "Sending packet %i %i\n", p->getSeqNum(), p->getWinSize());
  //     }
  //   }
  // }
  // else {
  //   fprintf(stderr, "Receiving packet %i\n", p->getAckNum());
  // }
}

void setUpSocket()
{
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // create a new socket
  if (sockfd < 0) {
    error("socket");
  }

  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET; //initialize server's address
  bcopy((char *)server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(portno);
  servlen = sizeof(serv_addr);

  int opt = 3;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVLOWAT,&opt,sizeof(opt));
}

int wait_for_data()
{
  tv.tv_sec = 0;
  tv.tv_usec = 50000;
  FD_ZERO(&sock);
  FD_SET(sockfd,&sock);
  int retval = select(sockfd+1, &sock, NULL, NULL, &tv);
  if (retval<0) {
    error("select");
  }
  return retval;
}

bool handshake()
{
  srand(time(NULL));
  int n_send = 0;
  int n_rec = 0;
  currSeqNum_client = rand() % MAXSEQNUM + 1;      //initial seq num
  currAckNum_client = 0;
  Packet syn;
  syn.setPorts(0, 0);        //change this to something useful
  syn.setSeqNum(currSeqNum_client);
  syn.setAckNum(currAckNum_client);         //doesnt matter
  syn.setPacketType(SYN);
  syn.setWinSize(5120);
  while (1) {
    syn.addTransmission();
    n_send = sendto(sockfd, (char*)&syn, MAXPACKETSIZE, 0, (struct sockaddr*)&serv_addr, servlen);
    if (n_send < 0) {
      error("sendto");
    }
    printMessage(&syn, "send");
    Packet synack;
    int getSYNACK = wait_for_data();
    if (getSYNACK) {
      n_rec = recvfrom(sockfd, (char*)&synack, MAXPACKETSIZE, 0, (struct sockaddr*)&serv_addr, &servlen);
      if (n_rec < 0) {
        error("recvfrom");
      }
      if (synack.getPacketType()==SYNACK) {
        printMessage(&synack, "recv");
        currAckNum_client=synack.getSeqNum()+1;
        currSeqNum_client=synack.getAckNum();
        Packet ack;
        ack.setPorts(0, 0);        //change this to something useful
        ack.setSeqNum(currSeqNum_client);
        ack.setAckNum(currAckNum_client);         //doesnt matter
        ack.setPacketType(ACK);
        ack.setWinSize(5120);
        ack.setData(file_req_name, file_req_name.length());
        ack.addTransmission();
        n_send = sendto(sockfd, (char*)&ack, MAXPACKETSIZE, 0, (struct sockaddr*)&serv_addr, servlen);
        if (n_send < 0) {
          error("sendto");
        }
        printMessage(&ack, "send");
        return true;
      }
    }
    else {
      fprintf(stderr, "%s\n", "TIMEOUT");
      continue;
    }
  }
  return false;
}

void writePacketsToFile()
{
  ofstream myfile;
  myfile.open ("received.data", ios::out | ios::binary);
  for(unsigned int i=0; i<packets.size(); i++) {
    myfile.write(packets[i]->getData(), packets[i]->getDataSize());
  }
  myfile.close();
}

void receivePackets()
{
  int n=0;
  while(1) {
    Packet* p = (Packet*) calloc(1, MAXPACKETSIZE);
    int isPacket = wait_for_data();
    if (isPacket) {
      n = recvfrom(sockfd, (char*)p, MAXPACKETSIZE, 0, (struct sockaddr*)&serv_addr, &servlen);
      if (n<0) {
        error("recvfrom");
      }
      else {
        Packet ack;
        ack.setPorts(0, 0);        //change this to something useful
        ack.setSeqNum(p->getAckNum());
        ack.setAckNum(currAckNum_client);         //doesnt matter
        ack.setPacketType(ACK);
        ack.setWinSize(5120);
        ack.setData(file_req_name, file_req_name.length());
        ack.addTransmission();
        int n2=sendto(sockfd, (char*)&ack, MAXPACKETSIZE, 0, (struct sockaddr*)&serv_addr, servlen);
      }
      // fprintf(stderr, "REC: %i\n", n);
      packets.push_back(p);
      continue;
    }
    else {
      break;
    }
  }
  writePacketsToFile();
}

int main(int argc, char *argv[])
{
    if (argc < 4) {
       fprintf(stderr, "Usage: %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    server = gethostbyname(argv[1]);  // takes a string like "www.yahoo.com", and returns a struct hostent which contains information, as IP address, address type, the length of the addresses...
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    char* file_c_str = (char*) calloc(1, strlen(argv[3]));
    strcpy(file_c_str, argv[3]);
    for (unsigned int i=0; i<strlen(file_c_str); i++) {
      file_req_name+=file_c_str[i];
    }

    setUpSocket();

    if (handshake()) {        //FIX HANDSHAKE: CHECK CORRECT NUMS, RT, ETC.
      receivePackets();
    }

    close(sockfd);  // close socket

    return 0;
}
