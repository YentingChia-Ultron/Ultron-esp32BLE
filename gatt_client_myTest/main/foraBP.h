#ifndef FORA_BP_H
#define FORA_BP_H
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

/* 
foraBPData.IHB : irregular heartbeat 
IHB =00 : Normal Heart Beats
IHB =01 : Tachycardia(>110) or Bradycardia(<50) 
IHB =10 : Varied Heart Rate ( ±20%)
IHB=11 : Atrail Fibrillation (AF)

foraBPData.AVG :  AVG=0 : Single measurement reading, AVG=1 : Average measurement reading
*/
struct foraBPData
{
    uint8_t dateY;                     
    uint8_t dateM;                     
    uint8_t dateD;                     
    uint8_t hour;
    uint8_t min;
    uint8_t IHB;  
    uint8_t AVG;        
    uint8_t systolic;       // Unit:mmHG
    uint8_t MAP;           // Mean Pressure, Unit:mmHG
    uint8_t diastolic;    // Unit:mmHG
    uint8_t pulse;        // Unit:pul/min
};

uint16_t getForaBPDataNum();
uint8_t getForaBPCheckSum(const uint8_t *data);
uint8_t setAllForaBPData(const uint8_t *data);
void printAllForaBPData();

/*
status:
0 : wait
1 : read store data num
2 : read time & other
3 : read temp
4 : clear
*/
uint8_t getForaBPStatus();
void waitForaBPData();

/* 
type :
0 : read store data num
1 : read time & other
2 : read temp
3 : clear
4 : init
*/
void getForaBPCmd(uint8_t type, uint8_t *buf);

/*
data type
0  dateY                     
1  dateM                     
2  dateD                     
3  hour
4  min
5  IHB  
6  AVG        
7  systolic       // Unit:mmHG
8  MAP           // Mean Pressure, Unit:mmHG
9  diastolic    // Unit:mmHG
10 pulse        // Unit:pul/min
*/
uint16_t getForaBPData(const uint8_t type, const uint16_t index);


#endif /*   FORA_BP_H   */