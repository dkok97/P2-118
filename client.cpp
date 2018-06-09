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
#include <algorithm>
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
      fprintf(stderr, "Sending packet SEQ: %i ACK: %i WIN: %i TYPE: %i TRANS: %i INDEX: %i\n", p->getSeqNum(), p->getAckNum(), p->getWinSize(), p->getPacketType(), p->getTransmissions(), p->getPackIndex());
    }
    else {
      fprintf(stderr, "Receiving packet SEQ: %i ACK: %i WIN: %i TYPE: %i TRANS: %i INDEX: %i\n", p->getSeqNum(), p->getAckNum(), p->getWinSize(), p->getPacketType(), p->getTransmissions(), p->getPackIndex());
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

void closeConnection(Packet* fin)
{
  int n_send=0;
  int n_rec=0;
  printMessage(fin, "recv");
  Packet finAck;
  finAck.setAckNum(fin->getSeqNum()+1);
  finAck.setSeqNum(fin->getAckNum());
  finAck.setWinSize(5120);
  finAck.setPacketType(FINACK);
  int i=-1;
  while (1) {
    i++;
    if (i==5) {
      fprintf(stderr, "%s\n", "GOT ACK");
      exit(1);
    }
    finAck.addTransmission();
    n_send=sendto(sockfd, (char*)&finAck, MAXPACKETSIZE, 0, (struct sockaddr*)&serv_addr, servlen);
    if (n_send<0) {
      error("sendto");
    }
    printMessage(&finAck, "send");
    Packet* p = (Packet*) calloc(1, MAXPACKETSIZE);
    int isData=wait_for_data();
    if (isData) {
      n_rec = recvfrom(sockfd, (char*)p, MAXPACKETSIZE, 0, (struct sockaddr*)&serv_addr, &servlen);
      if (n_rec<0) {
        error("recvfrom");
      }
      if (p->getPacketType()==FIN) {
        continue;
      }
      else if (p->getPacketType()==ACK) {
        printMessage(p, "recv");
        exit(1);
      }
    }
    else {
      continue;
    }
  }
}

bool compareByIndex(Packet *a, Packet *b)
{
    return a->getPackIndex() < b->getPackIndex();
}

void writePacketsToFile(Packet* fin)
{
  ofstream myfile;
  myfile.open ("received.data", ios::out | ios::binary);
  sort(packets.begin(), packets.end(), compareByIndex);
  for(unsigned int i=0; i<packets.size(); i++) {
    myfile.write(packets[i]->getData(), packets[i]->getDataSize());
  }
  myfile.close();
  closeConnection(fin);
}

void receivePackets(Packet* fp)
{
  fprintf(stderr, "%s\n", "HANDSHAKE DONE");
  printMessage(fp, "recv");
  int n=0;
  int n2=0;
  Packet* p = (Packet*) calloc(1, MAXPACKETSIZE);
  Packet ack;
  ack.setPackIndex(fp->getPackIndex());
  ack.setPacketType(ACK);
  ack.setWinSize(5120);
  ack.setSeqNum(fp->getAckNum());
  ack.setAckNum(fp->getSeqNum()+MAXPACKETSIZE+1);
  packets.push_back(fp);
  n2=sendto(sockfd, (char*)&ack, MAXPACKETSIZE, 0, (struct sockaddr*)&serv_addr, servlen);
  if (n2<0) {
    error("sendto");
  }
  printMessage(&ack, "send");
  while(1) {
    n = recvfrom(sockfd, (char*)p, MAXPACKETSIZE, 0, (struct sockaddr*)&serv_addr, &servlen);
    if (n<0) {
      error("recvfrom");
    }
    if (p->getPackIndex()==-100) {
      break;
    }
    printMessage(p, "recv");
    ack.setSeqNum(p->getAckNum());
    ack.setAckNum(p->getSeqNum()+MAXPACKETSIZE+1);         //doesnt matter
    ack.setPackIndex(p->getPackIndex());
    n2=sendto(sockfd, (char*)&ack, MAXPACKETSIZE, 0, (struct sockaddr*)&serv_addr, servlen);
    if (n2<0) {
      error("sendto");
    }
    printMessage(&ack, "send");
    bool alreadyRec=false;
    for (unsigned int j=0; j<packets.size(); j++) {
      if (packets[j]->getPackIndex()==p->getPackIndex()) {
        alreadyRec=true;
      }
    }
    if (!alreadyRec) {
      packets.push_back(p);
    }
  }
  writePacketsToFile(p);
}


void handshake()
{
  srand(time(NULL));
  int n_send = 0;
  int n_rec = 0;
  currSeqNum_client = rand() % MAXSEQNUM + 1;      //initial seq num
  currAckNum_client = 0;
  Packet syn;
  Packet synack;
  Packet ack;
  syn.setSeqNum(currSeqNum_client);
  syn.setAckNum(currAckNum_client);         //doesnt matter
  syn.setPacketType(SYN);
  syn.setWinSize(5120);
  bool synackAgain=false;
  while(1) {
    syn.addTransmission();
    n_send = sendto(sockfd, (char*)&syn, MAXPACKETSIZE, 0, (struct sockaddr*)&serv_addr, servlen);
    if (n_send < 0) {
      error("sendto");
    }
    printMessage(&syn, "send");
    int getSYNACK = wait_for_data();
    if (getSYNACK) {
      n_rec = recvfrom(sockfd, (char*)&synack, MAXPACKETSIZE, 0, (struct sockaddr*)&serv_addr, &servlen);
      if (n_rec < 0) {
        error("recvfrom");
      }
      if (synack.getPacketType()==SYNACK) {
        printMessage(&synack, "recv");
        break;
      }
      else {
        fprintf(stderr, "TYPE IS %i\n", synack.getPacketType());
        exit(1);
      }
    }
    else {
      continue;
    }
  }
  //GOT SYNACK
  Packet fp;
  while (1) {
    if (synackAgain) {
      printMessage(&fp, "recv");
    }
    currAckNum_client=synack.getSeqNum()+1;
    currSeqNum_client=synack.getAckNum();
    ack.setSeqNum(currSeqNum_client);
    ack.setAckNum(currAckNum_client);         //doesnt matter
    ack.setPacketType(ACK);
    ack.setWinSize(5120);
    ack.setData(file_req_name, file_req_name.length());
    ack.setSize(file_req_name.length());
    ack.addTransmission();
    n_send = sendto(sockfd, (char*)&ack, MAXPACKETSIZE, 0, (struct sockaddr*)&serv_addr, servlen);
    if (n_send < 0) {
      error("sendto");
    }
    printMessage(&ack, "send");
    int gotPacket=wait_for_data();
    if (gotPacket) {
      n_rec = recvfrom(sockfd, (char*)&fp, MAXPACKETSIZE, 0, (struct sockaddr*)&serv_addr, &servlen);
      if (n_rec<0) {
        error("recv");
      }
      if (fp.getPacketType()==REG) {
        break;
      }
      else if (fp.getPacketType()==SYNACK) {
        synackAgain=true;
        continue;
      }
      else {
        fprintf(stderr, "TYPE IS %i\n", ack.getPacketType());
        exit(1);
      }
    }
    else {
      continue;
    }
  }
  receivePackets(&fp);
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

    handshake();

    close(sockfd);  // close socket

    return 0;
}
