#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

/* 
foraData.type : 
1. Value=0, means ear temperature.
2. Value=1, means forehead temperature.
3. Value=2, means rectal temperature.
4. Value=3, means armpit temperature.
5. Value=4, means object surface temperature.
6. Value=5, means room temperature.
7. Value=6, means children temperature.
*/
struct foraData
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

uint16_t getDataNum();
uint8_t getCheckSum(const uint8_t *data);
uint8_t setAllData(const uint8_t *data);
void printAllForaData();

/*
status:
0 : wait
1 : read store data num
2 : read time & other
3 : read temp
4 : clear
*/
uint8_t getStatus();
void setStatus(uint8_t s);
void waitData();

/* 
type :
0 : read store data num
1 : read time & other
2 : read temp
3 : clear
4 : init
*/
void getForaCmd(uint8_t type, uint8_t *buf);

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
uint16_t getForaData(const uint8_t type, const uint16_t index);

uint8_t sendForaCmd();

