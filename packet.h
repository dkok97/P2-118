#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <fcntl.h>
using namespace std;

#define MAXPACKETSIZE 1024
#define MAXDATASIZE 1006     //header size if 17 bytes=136 bits. So data can have 1024-17=1007 bytes.
#define MAXSEQNUM 30720
#define WINSIZE 1024        //change to 5120

enum pkt_type { SYN = 0, SYNACK = 1, ACK = 2, FIN = 3, REG = 4 };

class Packet {
public:
    Packet();
    Packet(const Packet &p);
    Packet& operator = (const Packet &p);
    int setPorts(int src, int dst);
    int setSeqNum(int32_t seq);
    int32_t getSeqNum();
    int setAckNum(int32_t ack);
    int32_t getAckNum();
    int setWinSize(int16_t win);
    int16_t getWinSize();
    void setPacketType(pkt_type type);
    pkt_type getPacketType();
    int setData(const char* data, size_t size);
    char* getData();
private:
    int16_t src_port;
    int16_t dst_port;
    int32_t seq_num;
    int32_t ack_num;
    int8_t control_bit;
    int16_t win_size;
    int16_t checksum;
    char data[MAXDATASIZE+1];
};

Packet::Packet() {
    src_port = -1;
    dst_port = -1;
    seq_num = -1;
    ack_num = -1;
    control_bit = -1;
    win_size = -1;
    checksum = -1;
    bzero(data, MAXDATASIZE+1);
}

Packet::Packet(const Packet &p) {
  this->src_port=p.src_port;
  this->dst_port=p.dst_port;
  this->seq_num=p.seq_num;
  this->ack_num=p.ack_num;
  this->control_bit=p.control_bit;
  this->win_size=p.win_size;
  this->checksum=p.checksum;
  strcpy(this->data, p.data);
}

Packet&
Packet::operator=(const Packet &p) {
  this->src_port=p.src_port;
  this->dst_port=p.dst_port;
  this->seq_num=p.seq_num;
  this->ack_num=p.ack_num;
  this->control_bit=p.control_bit;
  this->win_size=p.win_size;
  this->checksum=p.checksum;
  strcpy(this->data, p.data);
  return *this;
}

int Packet::setPorts(int src, int dst) {
    this->src_port=src;
    this->dst_port=dst;
    return 1;
}

int Packet::setSeqNum(int32_t seq) {
    this->seq_num=seq;
    return 1;
}

int32_t Packet::getSeqNum() {
    return this->seq_num;
}

int Packet::setAckNum(int32_t ack) {
    this->ack_num=ack;
    return 1;
}

int32_t Packet::getAckNum() {
    return this->ack_num;
}

int Packet::setWinSize(int16_t win) {
    this->win_size=win;
    return 1;
}

int16_t Packet::getWinSize() {
    return this->win_size;
}

void Packet::setPacketType(pkt_type type) {
    switch(type) {
        case SYN:
            this->control_bit=0;
        case SYNACK:
            this->control_bit=1;
        case ACK:
            this->control_bit=2;
        case FIN:
            this->control_bit=3;
        case REG:
            this->control_bit=4;
        default:
            this->control_bit=4;
    }
}

pkt_type Packet::getPacketType() {
    switch (this->control_bit) {
        case 0:
            return SYN;
        case 1:
            return SYNACK;
        case 2:
            return ACK;
        case 3:
            return FIN;
        case 4:
            return REG;
        default:
            return REG;
    }
}

int Packet::setData(const char* data, size_t size) {
    strcpy(this->data, data);
    this->data[size+1]='\0';
    return 1;
}

char* Packet::getData() {
    return this->data;
}
