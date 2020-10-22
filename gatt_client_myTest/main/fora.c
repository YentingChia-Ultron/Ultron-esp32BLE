#include <string.h>
#include <stdio.h>
#include "fora.h"
#define FORA_TEMP_METER_T   0
#define FORA_BLOOD_PRESSURE_METER_T   1
#define FORA_2IN1_METER_T   2
#define DEVICE_TYPE   FORA_2IN1_METER_T
#define USER_NUM   4


#if (DEVICE_TYPE == FORA_TEMP_METER_T)
    #include "foraThermometer.h"
#elif (DEVICE_TYPE == FORA_BLOOD_PRESSURE_METER_T)
    #include "foraBP.h"
#elif (DEVICE_TYPE == FORA_2IN1_METER_T)
    #include "foraBPG.h"
#endif

static const uint8_t cmds[6][8] = {
    {0x51, 0x2B, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x1F},         //read data num
    {0x51, 0x25, 0, 0, 0x00, 0x00, 0xA3, 0x19},               //read time[index]
    {0x51, 0x26, 0, 0, 0x00, 0x00, 0xA3, 0x1A},               //read time[index]
    {0x51, 0x52, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x46},         //clear all
    {0x51, 0x24, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x18},        //init (read mode)
    {0x51, 0x49, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x3D}};       //Read Mean Artrial Pressure
    
static struct ForaDevice d;
static ForaDevice_t fora_device = &d;
uint8_t status = 0;

uint8_t getForaUserNum()
{
    return (USER_NUM);
}

void foraInit()
{
    fora_device->type = DEVICE_TYPE;
    #if (DEVICE_TYPE == FORA_TEMP_METER_T)
        // fora_device->setAllData = setAllForaMeterData;
        // fora_device->getData = getForaMeterData;
        // fora_device->printAll = printAllForaMeterData;
        // fora_device->getDataNum = getForaMeterDataNum;
    #elif (DEVICE_TYPE == FORA_BLOOD_PRESSURE_METER_T)
        // fora_device->setAllData = setAllForaBPData;
        // fora_device->getData = getForaBPData;
        // fora_device->printAll = printAllForaBPData;
        // fora_device->getDataNum = getForaBPDataNum;
    #elif (DEVICE_TYPE == FORA_2IN1_METER_T)
        fora_device->setAllData = setAllForaBPGData;
        fora_device->getData = getForaBPGData;
        fora_device->printAll = printAllForaBPGData;
        fora_device->printSingle = printSingleForaBPGData;
        fora_device->getDataNum = getForaBPGDataNum;
        fora_device->getReadNum = getForaBPGReadNum;
        fora_device->restart = restartSetBPG;
    #endif
}


void getForaCmd(uint8_t type, uint8_t *buf)
{
    memcpy(buf, cmds[type], 8);
}

uint16_t getForaDataNum()
{
    uint16_t ret = (*(fora_device->getDataNum))();
    return ret;
}
uint16_t getForaReadNum()
{
    uint16_t ret = (*(fora_device->getReadNum))();
    return ret;
}
uint8_t getForaStatus()
{
    return status;
}
void waitForaData()
{
    status = 0;
}

uint8_t getForaCheckSum(const uint8_t *data)
{
    uint16_t ret = 0;
    for(uint8_t i = 0; i < 7; i++)
        ret += data[i];
    return (uint8_t)(ret & 0xff);
}

uint8_t setAllForaData(const uint8_t *data)
{                

    uint8_t ret = 0;
    uint16_t check = 0;
    for(uint8_t i = 0; i < 7; i++)
            check += data[i];
    if((check & 0xff) == data[7])
    {
        ret = (*(fora_device->setAllData))(data);
        switch (data[1])
        {
            case 0x2B:
                status = 1;
                break;
            case 0x25:
                status = 2;
                break;
            case 0x26:
                status = 3;
                break;
            case 0x52:
                status = 4;
                break;
            case 0x49:
                status = 5;
                break;
            default:
                break;
        }
    }
    return ret;
}
uint8_t printAllForaData()
{
    uint8_t ret = (*(fora_device->printAll))();
    return ret;
}
uint8_t printSingleForaData(const uint16_t index)
{
    uint8_t ret = (*(fora_device->printSingle))(index);
    return ret;
}

uint16_t getForaData(const uint8_t type, const int index)
{
    uint16_t ret = (*(fora_device->getData))(type, index);
    return ret;
}
uint8_t restartSetting()
{
    status = 0;
    uint8_t ret = (*(fora_device->restart))();
    return ret;
}