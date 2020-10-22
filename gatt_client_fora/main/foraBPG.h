#ifndef FORA_BPG_H
#define FORA_BPG_H
#include <stdint.h>

/* 
foraBPGData.IHB : irregular heartbeat 
IHB =00 : Normal Heart Beats
IHB =01 : Tachycardia(>110) or Bradycardia(<50) 
IHB =10 : Varied Heart Rate ( ±20%)
IHB=11 : Atrail Fibrillation (AF)

foraBPGData.AVG :  AVG=0 : Single measurement reading, AVG=1 : Average measurement reading
foraBPGData.pg : P/G=0 means blood glucose, P/G=1 means blood pressure
foraBPGData.arrhy : Arrhy=0 :normal heart beats, Arrhy=1 : arrhythmia

foraBPGData.gType : Glucose Type
gType =00 : Gen
gType =01 : AC (before meal)
gType =10 : PC (after meal)
gType =11 : QC (quality control)
*/
struct foraBPGData
{
    uint8_t dateY;                     
    uint8_t dateM;                     
    uint8_t dateD;                     
    uint8_t hour;
    uint8_t min;
    uint8_t IHB;  
    uint8_t AVG;        
    uint16_t systolic;       // Unit:mmHG
    uint16_t MAP;           // Mean Pressure, Unit:mmHG
    uint8_t diastolic;    // Unit:mmHG
    uint8_t pulse;        // Unit:pul/min
    uint8_t pg;
    uint8_t arrhy;
    uint8_t IHB2;  
    uint16_t glucose;     // Unit:mg/dL
    uint8_t gType;
    uint8_t gCode;
    uint8_t ambientTemp;  //Unit:0.1°C
};

uint16_t getForaBPGReadNum();
uint16_t getForaBPGDataNum();
uint8_t setAllForaBPGData(const uint8_t *data);
uint8_t printAllForaBPGData();
uint8_t printSingleForaBPGData(const uint16_t index);
uint8_t restartSetBPG();

/*
data type
same order from struct
*/
uint16_t getForaBPGData(const uint8_t type, const uint16_t index);


#endif /*   FORA_BPG_H   */