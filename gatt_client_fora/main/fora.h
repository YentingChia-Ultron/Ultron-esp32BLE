#ifndef FORA_H
#define FORA_H
#include <stdint.h>
#include <stdbool.h>

void foraInit();
uint16_t getForaDataNum();
uint16_t getForaReadNum();
uint8_t getForaCheckSum(const uint8_t *data);
uint8_t setAllForaData(const uint8_t *data);
uint8_t printAllForaData();
uint8_t printSingleForaData(const uint16_t index);
uint8_t getForaStatus();
void waitForaData();
uint8_t getForaUserNum();
uint8_t restartSetting();

/* 
type :
0 : read store data num
1 : read time & other
2 : read temp
3 : clear
4 : init
5 : Read Mean Artrial Pressure
*/
void getForaCmd(uint8_t type, uint8_t *buf);
uint16_t getForaData(const uint8_t type, const int index);

struct ForaDevice {
    uint8_t type; 
    uint8_t (*setAllData)(const uint8_t *data);
    uint16_t (*getData)(const uint8_t type, const int index); 
    uint8_t (*printAll)();
    uint8_t (*printSingle)(const uint16_t index);
    uint16_t (*getDataNum)();
    uint16_t (*getReadNum)();
    uint16_t (*restart)();
};

typedef struct ForaDevice* ForaDevice_t;

#endif /*   FORA_H   */