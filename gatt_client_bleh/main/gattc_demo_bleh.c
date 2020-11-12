#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "BLE_client.h"

#define DEVICE_NUM 2

#define REMOTE_SERVICE_UUID        {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x23, 0x15, 0, 0}
#define REMOTE_NOTIFY_CHAR_UUID    {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x24, 0x15, 0, 0}

#define GATTS_SERVICE_UUID_TEST_A   0xFFF0
#define GATTS_CHAR_UUID_TEST_A      0xFFF6
#define GATTS_SERVICE_UUID_TEST_B   0xEEE0
#define GATTS_CHAR_UUID_TEST_B      0xEEE1

esp_bt_uuid_t char_uuid1[1]= {
    {
        .len = ESP_UUID_LEN_128,
        .uuid = {.uuid128 = REMOTE_NOTIFY_CHAR_UUID},
    }
};
esp_bt_uuid_t char_uuid2[1]= {
    {
        .len = ESP_UUID_LEN_16,
        .uuid = {.uuid16 = GATTS_CHAR_UUID_TEST_A},
    }
};
esp_bt_uuid_t char_uuid3[1]= {
    {
        .len = ESP_UUID_LEN_16,
        .uuid = {.uuid16 = GATTS_CHAR_UUID_TEST_B},
    }
};
UuidsT myUUID1[1] = {
    {
        .service_uuid = {
            .len = ESP_UUID_LEN_128,
            .uuid = {.uuid128 = REMOTE_SERVICE_UUID},
        },
        .char_num = 1,
        .char_uuid = char_uuid1
    }
};
UuidsT myUUID2[2] = {
    {
        .service_uuid = {
            .len = ESP_UUID_LEN_16,
            .uuid = {.uuid16 = GATTS_SERVICE_UUID_TEST_A},
        },
        .char_num = 1,
        .char_uuid = char_uuid2
    },
    {
        .service_uuid = {
            .len = ESP_UUID_LEN_16,
            .uuid = {.uuid16 = GATTS_SERVICE_UUID_TEST_B},
        },
        .char_num = 1,
        .char_uuid = char_uuid3
    }
};

uint8_t bda[2][6] = {{0xC0, 0x26, 0xDA, 0x03, 0x98, 0x71},
                    {0xBC, 0xDD, 0xC2, 0xDF, 0x8D, 0xAA}};

uint8_t is_send[2] = {0};
void test(void *arg)
{
    initBle();
    addProfile(myUUID1, 1, bda[0]);
    addProfile(myUUID2, 2, bda[1]);
    startScan(-1);
    while (1)
    {
        ProfileNodeT *temp = findProfile(0);
        uint8_t pro_id = 0;
        while(temp != NULL)
        {
            printf("state[%d] = %d\n", pro_id, getBleStatus(pro_id));
            pro_id++;
            temp = temp->next;
        }
        temp = findProfile(0);
        pro_id = 0;
        while(temp != NULL)
        {
            uint8_t status = getBleStatus(pro_id);
            if(status == 1)
            {
                openProfile(pro_id);
                vTaskDelay(pdMS_TO_TICKS(150));
            }
            if(!is_send[pro_id] && status == 2)
            {
                if(pro_id == 1)
                {
                    uint8_t cmd[2][2]= {{0xf0, 0xf1}, {0xf2, 0xf3}};
                    sendCommand(pro_id, 0, 0, cmd[0], 2);
                    sendCommand(pro_id, 1, 0, cmd[1], 2);
                }
                else
                {
                    uint8_t cmd[] = {0x51, 0x2B, 0x01, 0x00, 0x00, 0x00, 0xA3, 0x20};
                    sendCommand(pro_id, 0, 0, cmd, 8);
                }
                is_send[pro_id] = 1;
            }
            if(is_send[pro_id] > 0)
                is_send[pro_id] = (is_send[pro_id] >= 2)? 0 : is_send[pro_id] + 1;
            if(getDataStatus(pro_id, 0, 0))
            {
                printf("notify[%d] : \n", pro_id);
                is_send[pro_id] = 0;
                uint8_t rec[32];
                if(pro_id == 1)
                {
                    uint8_t len = getNotifyLen(pro_id, 0, 0);
                    if(len > 32)
                    printf("over size\n");
                    else
                    {
                        getNotifyVlaue(pro_id, 0, 0, rec);
                        for(int i = 0; i < len; i++)
                        {
                            printf("%02X ", rec[i]);
                        }
                        printf("\n");
                    }
                    len = getNotifyLen(pro_id, 1, 0);
                    if(len > 32)
                    printf("over size\n");
                    else
                    {
                        getNotifyVlaue(pro_id, 1, 0, rec);
                        for(int i = 0; i < len; i++)
                        {
                            printf("%02X ", rec[i]);
                        }
                        printf("\n\n");
                    }
                }
                else
                {
                    uint8_t len = getNotifyLen(pro_id, 0, 0);
                    if(len > 32)
                    printf("over size\n");
                    else
                    {
                        getNotifyVlaue(pro_id, 0, 0, rec);
                        for(int i = 0; i < len; i++)
                        {
                            printf("%02X ", rec[i]);
                        }
                        printf("\n\n");
                    }
                }
            }
            temp = temp->next;
            pro_id++;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    
    
    // addProfile(myUUID1, 1);
    // insertProfile(0, myUUID2, 2);
    // deleteProfile(1);

    // ProfileNodeT *trip = findProfile(0);
    // while(trip != NULL)
    // {
    //     printf("trival -> id : %d, service num : %d\n", trip->profile_id, trip->profile.service_num);
    //     trip = trip->next;
    // }

    xTaskCreate(test, "test", 0x10000, NULL, 5, NULL);
}

