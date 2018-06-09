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
#include <math.h>
#include <cerrno>
#include "packet.h"
using namespace std;

int sockfd, portno;
struct sockaddr_in serv_addr, cli_addr;
socklen_t clilen = sizeof(cli_addr);
fd_set sock;
struct timeval tv;
int32_t currSeqNum_server, currAckNum_server;
const char* file_req_name;
vector<Packet*> packets;
vector<Packet*> window;
int debug=1;

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
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // create socket
  if (sockfd < 0) {
    error("socket");
  }
  memset((char *) &serv_addr, 0, sizeof(serv_addr));  // reset memory

  // fill in address info
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    error("bind");
  }
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
  currSeqNum_server = rand() % MAXSEQNUM + 1;
  currAckNum_server = 0;
  int n_send=0;
  int n_rec=0;
  Packet syn;
  while (1) {
    int isSYN = wait_for_data();
    if (isSYN) {
      n_rec = recvfrom(sockfd, (char*)&syn, MAXPACKETSIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
      if (n_rec<0) {
        error("recv");
      }
      if (syn.getPacketType()==SYN) {
        printMessage(&syn, "recv");
        currAckNum_server=syn.getSeqNum()+1;
        Packet synack;
        synack.setAckNum(currAckNum_server);
        synack.setPorts(0, 0);        //change this to something useful
        synack.setSeqNum(currSeqNum_server);
        synack.setPacketType(SYNACK);
        synack.setWinSize(5120);
        synack.addTransmission();
        n_send = sendto(sockfd, (char*)&synack, MAXPACKETSIZE, 0, (struct sockaddr *) &cli_addr, clilen);
        printMessage(&synack, "send");
        if (n_send < 0) {
          error("sendto");
        }
        Packet ack;
        int getACK = wait_for_data();
        if (getACK) {
          n_rec = recvfrom(sockfd, (char*)&ack, MAXPACKETSIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
          if (n_rec<0) {
            error("recv");
          }
          if (ack.getPacketType()==ACK) {
            printMessage(&ack, "recv");
            currAckNum_server=ack.getSeqNum()+ack.getDataSize();
            currSeqNum_server=ack.getAckNum();
            file_req_name = (char*) calloc(1, ack.getDataSize());
            file_req_name=ack.getData();
            return true;
            break;
          }
          else {
            continue;
          }
        }
        else {
          continue;
        }
      }
      else {
        continue;
      }
    }
    else {
      continue;
    }
  }
  return false;
}

vector<Packet*> splitFile(string fname)
{
  vector<Packet*> packets;
  string filename = fname;
  ifstream file;
  file.open(filename.c_str(), ifstream::in | ios::binary);
  if(!file) {
      fprintf(stderr, "File does not exist.\n");
      exit(EXIT_FAILURE);
  }
  file.seekg(0, ios::end);
  long double fileSize = file.tellg();
  file.seekg(0, ios::beg);
  char c;
  string text;
  while(file.get(c)) {
      text.push_back(c);
  }
  if (file.eof())
  {
      double pnumber = ceil(fileSize / MAXDATASIZE);

      for(int i = 0; i < pnumber; i++)
      {
          Packet *p = new Packet();
          packets.push_back(p);
      }
      string data="";
      for(int i = 0; i < pnumber; i++)
      {
          packets[i]->setPorts(0, 0);
          packets[i]->setPacketType(REG);
          try{
              data = text.substr(i*MAXDATASIZE, MAXDATASIZE);
          }
          catch(...) {
              data = text.substr(i*MAXDATASIZE, data.length() - 1);
          }
          packets[i]->setData(data, data.length());
          packets[i]->setPackIndex(i);
          packets[i]->setWinSize(5120);
          packets[i]->setAckNum(currAckNum_server);
          packets[i]->setSeqNum(currSeqNum_server);
          currSeqNum_server=currSeqNum_server+packets[i]->getDataSize();
      }
  }
  else {
    error("processing file");
  }
  return packets;
}

void sendPackets()
{
  packets = splitFile(file_req_name);
  for (i=0; i<WINSIZE/MAXPACKETSIZE; i++) {
    window.push_back(packets[i]);
  }
  while (1) {
    int n=0;
    int win_index=0;
    while (window.size!=0) {
      window[win_index]->addTransmission();
      n = sendto(sockfd, (char*)window[win_index], MAXPACKETSIZE, 0, (struct sockaddr *) &cli_addr, clilen);
      win_index++;
    }

  }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
    portno = atoi(argv[1]);

    setUpSocket();

    if (handshake()) {
      sendPackets();
    }

    close(sockfd);

    return 0;
}
