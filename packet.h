#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <fcntl.h>
#include <time.h>
using namespace std;

#define MAXPACKETSIZE 1024
#define MAXDATASIZE 992     //header size if 28 bytes. So data can have 1024-31=996 bytes.
#define MAXSEQNUM 30720
#define WINSIZE 5120

#define SYN 0
#define SYNACK 1
#define ACK 2
#define FIN 3
#define REG 4
#define FINACK 5
#define NOTHING -1

class Packet {
public:
    Packet();
    Packet(const Packet &p);
    Packet& operator = (const Packet &p);
    int setPackIndex(int i);
    int getPackIndex();
    int setSeqNum(int32_t seq);
    int32_t getSeqNum();
    int setAckNum(int32_t ack);
    int32_t getAckNum();
    int setWinSize(int16_t win);
    int16_t getWinSize();
    void setPacketType(int type);
    int getPacketType();
    int setData(string data_s, int size);
    char* getData();
    int getDataSize();
    void addTransmission();
    int getTransmissions();
    void setSize(int s);
private:
    int packet_index;
    int32_t seq_num;
    int32_t ack_num;
    int control_bit;
    int16_t win_size;
    int num_transmissions;
    int size;
    bool isSent;
    bool isAcked;
    char data[MAXDATASIZE];
};

Packet::Packet() {
    packet_index=-1;
    seq_num = -1;
    ack_num = -1;
    control_bit = NOTHING;
    win_size = 5120;
    num_transmissions=0;
    size=0;
    isSent=false;
    isAcked=true;
    bzero(data, MAXDATASIZE);
}

Packet::Packet(const Packet &p) {
    this->packet_index=p.packet_index;
    this->seq_num=p.seq_num;
    this->ack_num=p.ack_num;
    this->control_bit=p.control_bit;
    this->win_size=p.win_size;
    strcpy(this->data, p.data);
}

Packet&
Packet::operator=(const Packet &p) {
    this->packet_index=p.packet_index;
    this->seq_num=p.seq_num;
    this->ack_num=p.ack_num;
    this->control_bit=p.control_bit;
    this->win_size=p.win_size;
    strcpy(this->data, p.data);
    return *this;
}

void Packet::addTransmission() {
    this->num_transmissions++;
}

int Packet::getTransmissions() {
    return this->num_transmissions;
}

int Packet::setPackIndex(int i) {
    this->packet_index=i;
    return 1;
}

int Packet::getPackIndex() {
    return this->packet_index;
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

void Packet::setPacketType(int type) {
    this->control_bit=type;
}

int Packet::getPacketType() {
    return this->control_bit;
}

int Packet::setData(string data_s, int size_s) {
    for (int i=0; i<size_s; i++) {
      this->data[i]=data_s[i];
    }
    return 1;
}

void Packet::setSize(int s) {
  this->size=s;
}

char* Packet::getData() {
  return this->data;
}

int Packet::getDataSize() {
  return this->size;
}
