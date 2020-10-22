#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "foraBPG.h"

static uint16_t readNum = 0;
static uint16_t allDataNum = 0;
struct foraBPGData myData[100];
static uint16_t nowIndex = 0;

uint16_t getForaBPGDataNum()
{
    return allDataNum;
}
uint16_t getForaBPGReadNum()
{
    return readNum;
}
static void setDataNum(const uint8_t *data)
{
    readNum = (data[2] + (data[3] << 8));
    allDataNum += (data[2] + (data[3] << 8));
}

static void setBPGData(const uint8_t *data, const uint16_t index)
{
    myData[index].glucose = (data[3] << 8) + data[2]; 
    myData[index].gType = data[5] >> 7;
    myData[index].gCode = data[5] & 0x1f;
    myData[index].ambientTemp = data[4];

    myData[index].systolic = (data[3] << 8) + data[2]; 
    myData[index].pulse = data[5];
    myData[index].diastolic = data[4];
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
    myData[index].IHB = (data[5] >> 5) & 3;
    myData[index].pg = data[4] >> 7;
    myData[index].arrhy = (data[4] >> 6) & 1;
}

static void setMAPData(const uint8_t *data, const uint16_t index)
{
    myData[index].MAP = (data[3] << 8) + data[2];
}

uint8_t setAllForaBPGData(const uint8_t *data) //return 1 : ok, return 0 : error
{
    uint16_t check = 0;
    for(uint8_t i = 0; i < 7; i++)
            check += data[i];
    if((check & 0xff) == data[7])
    {
        switch (data[1])
        {
            case 0x2B:
                setDataNum(data);
                break;
            case 0x25:
                setOtherData(data, nowIndex);
                break;
            case 0x26:
                setBPGData(data, nowIndex);
                nowIndex++;
                break;
            case 0x52:
                nowIndex = 0;
                allDataNum = 0;
                break;
            case 0x49:
                setMAPData(data, nowIndex);
                break;
            default:
                break;
        }
    }
    else
        return 0;
    return 1;
}

uint16_t getForaBPGData(const uint8_t type, const uint16_t index)
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
    case 11:
        return myData[index].pg;
        break;
    case 12:
        return myData[index].arrhy;
        break;
    case 13:
        return myData[index].pulse;
        break;
    case 14:
        return myData[index].glucose;
        break;
    case 15:
        return myData[index].gType;
        break;
    case 16:
        return myData[index].gCode;
        break;
    case 17:
        return myData[index].ambientTemp;
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
static void getGT(uint8_t type, char *str)
{
    switch (type)
    {
    case 0:
        strcpy(str, "Gen");
        break;
    case 1:
        strcpy(str, "AC (before meal)");
        break;
    case 2:
        strcpy(str, "PC (after meal)");
        break;
    case 3:
        strcpy(str, "QC (quality control)");
        break;
    default:
        strcpy(str, "undefine type");
        break;
    }
}

uint8_t printAllForaBPGData()
{
    
    char tmpStr[50];
    if(allDataNum == 0)
    {
        printf("\n<<no data>>\n");
        return 0;
    }
    for(int i = 0; i < allDataNum; i++)
    {
        printf("\n========================== index : %2d ==========================\n", i + 1);
        if(myData[i].pg)
        {
            printf("mode : blood pressure\n");
            printf("date & time : %02d-%02d-%02d", myData[i].dateY, myData[i].dateM, myData[i].dateD);
            printf("  %02d:%02d\n", myData[i].hour, myData[i].min);
            printf("-------  Blood Pressure Info  -------\n");
            if(myData[i].AVG)
                printf("Average measurement reading\n");
            else
                printf("Single measurement reading\n");
            if(myData[i].arrhy)
                printf("arrhy : arrhythmia !\n");
            else
                printf("arrhy : normal heart beats\n");
            getIHBStr(myData[i].IHB, tmpStr);
            printf("IHB : %s\n", tmpStr);
            printf("systolic : %d mmHG \n", myData[i].systolic);
            printf("diastolic : %d mmHG\n", myData[i].diastolic);
            printf("MAP : %d mmHG\n", myData[i].MAP);
            printf("pulse : %d pul/min\n", myData[i].pulse);
        }
        else
        {
            printf("mode : blood glucose\n");
            printf("date & time : %02d-%02d-%02d", myData[i].dateY, myData[i].dateM, myData[i].dateD);
            printf("  %02d:%02d\n", myData[i].hour, myData[i].min);
            printf("-------  Blood Glucose Info  -------\n");
            getGT(myData[i].gType, tmpStr);
            printf("Glucose Type : %s\n", tmpStr);
            printf("glucose : %d mg/dL \n", myData[i].glucose);
            printf("glucose code : %d  \n", myData[i].gCode);
            printf("ambient temperature : %d °C\n", myData[i].ambientTemp);
        }
    }
    return 1;
}

uint8_t printSingleForaBPGData(const uint16_t index)
{
    if(index > allDataNum)
    {
        printf("index error, data num : %d\n", allDataNum);
        return 0;
    }
    char tmpStr[50];
    printf("\n========================== index : %2d ==========================\n", index + 1);
    if(myData[index - 1].pg)
    {
        printf("date & time : %02d-%02d-%02d", myData[index - 1].dateY, myData[index - 1].dateM, myData[index - 1].dateD);
        printf("  %02d:%02d\n", myData[index - 1].hour, myData[index - 1].min);
        printf("mode : blood pressure\n");
        printf("-------  Blood Pressure Info  -------\n");
        if(myData[index - 1].AVG)
            printf("Average measurement reading\n");
        else
            printf("Single measurement reading\n");
        if(myData[index - 1].arrhy)
            printf("arrhy : arrhythmia !\n");
        else
            printf("arrhy : normal heart beats\n");
        getIHBStr(myData[index - 1].IHB, tmpStr);
        printf("IHB : %s\n", tmpStr);
        printf("systolic : %d mmHG \n", myData[index - 1].systolic);
        printf("diastolic : %d mmHG\n", myData[index - 1].diastolic);
        printf("MAP : %d mmHG\n", myData[index - 1].MAP);
        printf("pulse : %d pul/min\n", myData[index - 1].pulse);
    }
    else
    {
        printf("date & time : %02d-%02d-%02d", myData[index - 1].dateY, myData[index - 1].dateM, myData[index - 1].dateD);
        printf("  %02d:%02d\n", myData[index - 1].hour, myData[index - 1].min);
        printf("mode : blood glucose\n");
        printf("-------  Blood Glucose Info  -------\n");
        getGT(myData[index - 1].gType, tmpStr);
        printf("Glucose Type : %s\n", tmpStr);
        printf("glucose : %d mg/dL \n", myData[index - 1].glucose);
        printf("glucose code : %d  \n", myData[index - 1].gCode);
        printf("ambient temperature : %d °C\n", myData[index - 1].ambientTemp);
    }
    return 1;
}

uint8_t restartSetBPG()
{
    nowIndex = 0;
    allDataNum = 0;
    return 1; 
}