#ifndef FORA_THMETER_H
#define FORA_THMETER_H
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

/* 
foraMeterData.type : 
1. Value=0, means ear temperature.
2. Value=1, means forehead temperature.
3. Value=2, means rectal temperature.
4. Value=3, means armpit temperature.
5. Value=4, means object surface temperature.
6. Value=5, means room temperature.
7. Value=6, means children temperature.
*/
struct foraMeterData
{
    uint8_t type; 
    uint8_t outUint;                    //0 : Celsius(°C) , 1 : Fahrenheit (°F)
    uint8_t dateY;                     
    uint8_t dateM;                     
    uint8_t dateD;                     
    uint8_t hour;
    uint8_t min;
    uint16_t objectTemperature;        // Unit=0.1°C
    uint16_t backgroundTemperature;    // Unit=0.1°C
};

uint16_t getForaMeterDataNum();
uint8_t getForaCheckSum(const uint8_t *data);
uint8_t setAllForaMeterData(const uint8_t *data);
void printAllForaMeterData();

/*
status:
0 : wait
1 : read store data num
2 : read time & other
3 : read temp
4 : clear
*/
uint8_t getForaMeterStatus();
void waitForaMeterData();

/* 
type :
0 : read store data num
1 : read time & other
2 : read temp
3 : clear
4 : init
*/
void getForaMeterCmd(uint8_t type, uint8_t *buf);

/*
data type
0 : temperature type
1 : unit ( 0 : Celsius(°C) , 1 : Fahrenheit (°F) )
2 : date(year)
3 : date(month)
4 : date(day)
5 : time(hour)
6 : time(min)
7 : object temperature  ( Unit=0.1°C )
8 : background temperature  ( Unit=0.1°C )
*/
uint16_t getForaMeteData(const uint8_t type, const uint16_t index);

#endif  /*  FORA_THMETER_H  */
