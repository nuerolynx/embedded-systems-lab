#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "pins.h"
#include "credential.h"
// Bounded SPI transfers; never waits for a card or polls in a blocking loop.
// NXP PN532 UM0701 rev02: SPI LSB-first mode0, commands 14/32/4A.
class Pn532Async {
  enum Stage { RESET_LOW, BOOT_WAIT, SEND_SAM, SEND_RETRY, SEND_SCAN, ACK, REPLY, BACKOFF };
  Stage state=RESET_LOW, next=SEND_SAM;
  uint32_t due=0; uint8_t expected=0;
  SPISettings settings{1000000, LSBFIRST, SPI_MODE0};
  void select(){SPI.beginTransaction(settings); digitalWrite(Pins::HF_CS,LOW); delayMicroseconds(2);}
  void release(){digitalWrite(Pins::HF_CS,HIGH); SPI.endTransaction();}
  bool ready(){select(); SPI.transfer(2); bool yes=SPI.transfer(0)==1; release(); return yes;}
  void send(const uint8_t *cmd, uint8_t n, Stage after, uint32_t now){
    select(); SPI.transfer(1); SPI.transfer(0); SPI.transfer(0); SPI.transfer(0xFF);
    SPI.transfer(n+1); SPI.transfer(uint8_t(-(n+1))); SPI.transfer(0xD4);
    uint8_t sum=0xD4; for(uint8_t i=0;i<n;i++){SPI.transfer(cmd[i]); sum+=cmd[i];}
    SPI.transfer(uint8_t(-sum)); SPI.transfer(0); release();
    expected=cmd[0]+1; next=after; state=ACK; due=now+120;
  }
  bool readAck(){
    const uint8_t ack[]={0,0,0xFF,0,0xFF,0}; bool valid=true;
    select(); SPI.transfer(3); for(uint8_t b:ack) if(SPI.transfer(0)!=b) valid=false;
    release(); return valid;
  }
  int readReply(uint8_t *data, uint8_t cap){
    select(); SPI.transfer(3);
    uint8_t a=SPI.transfer(0), b=SPI.transfer(0), c=SPI.transfer(0);
    uint8_t n=SPI.transfer(0), nc=SPI.transfer(0);
    if(a || b || c!=0xFF || uint8_t(n+nc)!=0 || n<2 || n>cap) {release(); return -1;}
    uint8_t sum=0; for(uint8_t i=0;i<n;i++){data[i]=SPI.transfer(0); sum+=data[i];}
    sum+=SPI.transfer(0); uint8_t post=SPI.transfer(0); release();
    return sum==0 && post==0 && data[0]==0xD5 && data[1]==expected ? n : -1;
  }
  void fail(uint32_t now){++errors; digitalWrite(Pins::HF_RESET,LOW); state=RESET_LOW; due=now+1000;}
public:
  uint32_t errors=0;
  void begin(uint32_t now){
    digitalWrite(Pins::HF_CS,HIGH); pinMode(Pins::HF_CS,OUTPUT);
    digitalWrite(Pins::HF_RESET,LOW); pinMode(Pins::HF_RESET,OUTPUT_OPEN_DRAIN);
    SPI.begin(Pins::HF_SCK,Pins::HF_MISO,Pins::HF_MOSI,Pins::HF_CS);
    state=RESET_LOW; due=now+10;
  }
  bool tick(uint32_t now, Credential &out){
    if(state==ACK || state==REPLY){
      if(int32_t(now-due)>=0){fail(now); return false;}
      if(!ready()) return false;
      if(state==ACK){if(!readAck()){fail(now);return false;}state=REPLY;due=now+600;return false;}
      uint8_t rx[64]; int n=readReply(rx,sizeof(rx));
      if(n<0){fail(now);return false;}
      state=next; due=now+40;
      if(expected==0x4B && n>=9 && rx[2]==1){
        uint8_t len=rx[7]; // D5 4B NbTg Tg SENS_RES[2] SEL_RES NFCIDLength NFCID
        if((len==4 || len==7 || len==10) && n>=8+len){
          out={}; out.kind=CredentialKind::HF_UID; out.length=len;
          memcpy(out.data,rx+8,len); out.seen=now; return true;
        }
      }
      return false;
    }
    if(int32_t(now-due)<0)return false;
    switch(state){
      case RESET_LOW: digitalWrite(Pins::HF_RESET,HIGH);state=BOOT_WAIT;due=now+120;break;
      case BOOT_WAIT: state=SEND_SAM;break;
      case SEND_SAM:{const uint8_t c[]={0x14,1,0x14,0};send(c,sizeof(c),SEND_RETRY,now);break;}
      case SEND_RETRY:{const uint8_t c[]={0x32,5,0xFF,1,0};send(c,sizeof(c),SEND_SCAN,now);break;}
      case SEND_SCAN:{const uint8_t c[]={0x4A,1,0};send(c,sizeof(c),SEND_SCAN,now);break;}
      default:break;
    }
    return false;
  }
};
