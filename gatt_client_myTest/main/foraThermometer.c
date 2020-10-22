#include "foraThermometer.h"

static uint16_t dataNum = 0;
struct foraMeterData myData[100];
static uint8_t status = 0;
static uint16_t nowIndex = 0;


static const uint8_t cmds[5][8] = {
    {0x51, 0x2B, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x1F},         //read data num
    {0x51, 0x25, 0, 0, 0x00, 0x00, 0xA3, 0x19},               //read time[index]
    {0x51, 0x26, 0, 0, 0x00, 0x00, 0xA3, 0x1A},               //read time[index]
    {0x51, 0x52, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x46},         //clear all
    {0x51, 0x24, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x18}};        //init (read mode)

uint16_t getForaMeterDataNum()
{
    return dataNum;
}

static void setDataNum(const uint8_t *data)
{
    dataNum = data[2] + (data[3] << 8);
}

static void setOtherData(const uint8_t *data, const uint16_t index)
{
    uint16_t date = (data[3] << 8) + data[2];   // Y(7 bit) + M(4 bit) + D(5 bit)
    myData[index].dateY = date >> 9;
    myData[index].dateM = (date >> 5) & 0xf;
    myData[index].dateD = date & (32 - 1);
    myData[index].hour = data[5] & (32 - 1);
    myData[index].min = data[4] & (64 - 1);
    myData[index].outUint = data[5] >> 7;
    myData[index].type = (((data[5] >> 5) & 1 ) << 2) + (data[4] >> 6);
}

static void setTempData(const uint8_t *data, const uint16_t index)
{
    myData[index].objectTemperature = (data[3] << 8) + data[2];
    myData[index].backgroundTemperature = (data[5] << 8) + data[4];
}

uint8_t setAllForaMeterData(const uint8_t *data) //return 1 : ok, return 0 : error
{
    uint16_t check = 0;
    for(int i = 0; i < 7; i++)
            check += data[i];
        if((check & 0xff) == data[7])
        {
            switch (data[1])
            {
                case 0x2B:
                    nowIndex = 0;
                    setDataNum(data);
                    status = 1;
                    break;
                case 0x25:
                    setOtherData(data, nowIndex);
                    status = 2;
                    break;
                case 0x26:
                    setTempData(data, nowIndex);
                    status = 3;
                    nowIndex++;
                    break;
                case 0x52:
                    //nowIndex = 0;
                    status = 4;
                    break;
                default:
                    break;
            }
        }
        else
            return 0;
    return 1;
}

uint8_t getForaCheckSum(const uint8_t *data)
{
    uint16_t ret = 0;
    for(uint8_t i = 0; i < 7; i++)
        ret += data[i];
    return (uint8_t)(ret & 0xff);
}


uint16_t getForaMeteData(const uint8_t type, const uint16_t index)
{
    switch (type)
    {
    case 0:
        return myData[index].type;
        break;
    case 1:
        return myData[index].outUint;
        break;
    case 2:
        return myData[index].dateY;
        break;
    case 3:
        return myData[index].dateM;
        break;
    case 4:
        return myData[index].dateD;
        break;
    case 5:
        return myData[index].hour;
        break;
    case 6:
        return myData[index].min;
        break;
    case 7:
        return myData[index].objectTemperature;
        break;
    case 8:
        return myData[index].backgroundTemperature;
        break;
    default:
        return 0;
    }
}

static void getTypeStr(uint8_t type, char *str)
{
    switch (type)
    {
    case 0:
        strcpy(str, "ear temperature");
        break;
    case 1:
        strcpy(str, "forehead temperature");
        break;
    case 2:
        strcpy(str, "rectal temperature");
        break;
    case 3:
        strcpy(str, "armpit temperature");
        break;
    case 4:
        strcpy(str, "object surface temperature");
        break;
    case 5:
        strcpy(str, "room temperature");
        break;
    case 6:
        strcpy(str, "children temperature");
        break;
    default:
        strcpy(str, "undefine type");
        break;
    }
}

void printAllForaMeterData()
{
    if(dataNum == 0)
        printf("\n<<no data>>\n");
    char typeStr[50];
    for(int i = 0; i < dataNum; i++)
    {
        getTypeStr(myData[i].type, typeStr);
        printf("\n========================== index : %2d ==========================\n", i + 1);
        printf("data type : %s\n", typeStr);
        printf("date & time : %02d-%02d-%02d", myData[i].dateY, myData[i].dateM, myData[i].dateD);
        printf("  %02d:%02d\n", myData[i].hour, myData[i].min);
        if(myData[i].outUint == 0)
        {
            printf("object temperature : %d.%d °C\n", myData[i].objectTemperature / 10, myData[i].objectTemperature % 10);
            printf("background temperature : %d.%d °C\n", myData[i].backgroundTemperature / 10, myData[i].backgroundTemperature % 10);
        }
        else
        {
            printf("object temperature : %d.%d °F\n", myData[i].objectTemperature / 10, myData[i].objectTemperature % 10);
            printf("background temperature : %d.%d °F\n", myData[i].backgroundTemperature / 10, myData[i].backgroundTemperature % 10);
        }

    }
}


uint8_t getForaMeterStatus()
{
    return status;
}

void waitForaMeterData()
{
    status = 0;
}

void getForaMeterCmd(uint8_t type, uint8_t *buf)
{
    memcpy(buf, cmds[type], 8);
}