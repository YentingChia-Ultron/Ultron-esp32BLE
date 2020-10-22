#include "foraBP.h"

static uint16_t dataNum = 0;
struct foraBPData myData[100];
static uint8_t status = 0;
static uint16_t nowIndex = 0;


static const uint8_t cmds[5][8] = {
    {0x51, 0x2B, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x1F},         //read data num
    {0x51, 0x25, 0, 0, 0x00, 0x00, 0xA3, 0x19},               //read time[index]
    {0x51, 0x26, 0, 0, 0x00, 0x00, 0xA3, 0x1A},               //read time[index]
    {0x51, 0x52, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x46},         //clear all
    {0x51, 0x24, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x18}};        //init (read mode)

uint16_t getForaBPDataNum()
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
    myData[index].hour = data[5] & 0x1f;
    myData[index].min = data[4] & 0x3f;
    myData[index].AVG = data[5] >> 7;
    myData[index].IHB = (data[4] >> 6) & 1;

}

static void setBPData(const uint8_t *data, const uint16_t index)
{
    myData[index].systolic = data[2];
    myData[index].MAP = data[3];
    myData[index].diastolic = data[4];
    myData[index].pulse = data[5];
}

uint8_t setAllForaBPData(const uint8_t *data) //return 1 : ok, return 0 : error
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
                    setBPData(data, nowIndex);
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

uint8_t getForaBPCheckSum(const uint8_t *data)
{
    uint16_t ret = 0;
    for(uint8_t i = 0; i < 7; i++)
        ret += data[i];
    return (uint8_t)(ret & 0xff);
}


uint16_t getForaBPData(const uint8_t type, const uint16_t index)
{
    switch (type)
    {
    case 0:
        return myData[index].dateY;
        break;
    case 1:
        return myData[index].dateM;
        break;
    case 2:
        return myData[index].dateD;
        break;
    case 3:
        return myData[index].hour;
        break;
    case 4:
        return myData[index].min;
        break;
    case 5:
        return myData[index].IHB;
        break;
    case 6:
        return myData[index].AVG;
        break;
    case 7:
        return myData[index].systolic;
        break;
    case 8:
        return myData[index].MAP;
        break;
    case 9:
        return myData[index].diastolic;
        break;
    case 10:
        return myData[index].pulse;
        break;
    default:
        return 0;
    }
}

static void getIHBStr(uint8_t type, char *str)
{
    switch (type)
    {
    case 0:
        strcpy(str, "Normal Heart Beats");
        break;
    case 1:
        strcpy(str, "Tachycardia(>110) or Bradycardia(<50)");
        break;
    case 2:
        strcpy(str, "Varied Heart Rate ( ±20%)");
        break;
    case 3:
        strcpy(str, "Atrail Fibrillation (AF)");
        break;
    default:
        strcpy(str, "undefine type");
        break;
    }
}

void printAllForaBPData()
{
    if(dataNum == 0)
        printf("\n<<no data>>\n");
    char tmpStr[50];
    for(int i = 0; i < dataNum; i++)
    {
        getIHBStr(myData[i].IHB, tmpStr);
        printf("\n========================== index : %2d ==========================\n", i + 1);
        if(myData[i].AVG)
            printf("Average measurement reading\n");
        else
            printf("Single measurement reading\n");
        printf("data IHB : %s\n", tmpStr);
        printf("date & time : %02d-%02d-%02d", myData[i].dateY, myData[i].dateM, myData[i].dateD);
        printf("  %02d:%02d\n", myData[i].hour, myData[i].min);
        printf("systolic : %d  \n", myData[i].systolic);
        printf("diastolic : %d\n", myData[i].diastolic);
        printf("MAP : %d\n", myData[i].MAP);
        printf("pulse : %d\n", myData[i].pulse);

    }
}


uint8_t getForaBPStatus()
{
    return status;
}

void waitForaBPData()
{
    status = 0;
}

void getForaBPCmd(uint8_t type, uint8_t *buf)
{
    memcpy(buf, cmds[type], 8);
}