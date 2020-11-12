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
bool need_to_open[2] = {true, true};
// bool have_opened[2] = {0};  // need to delete later
void test(void *arg)
{
    initBle();
    for(uint8_t i = 0; i < 2; i++)
        setBleBda(i, bda[i]);
    
    initUuid(0, myUUID1, 1);
    initUuid(1, myUUID2, 2);
    startScan(-1);
    while (1)
    {
        for(uint8_t i = 0; i < 2; i++)
            printf("status[%d] = %d\n", i, getBleStatus(i));
        printf("\n");
        for(uint8_t i = 0; i < 2; i++)
        {
            uint8_t status = getBleStatus(i);
            if(status == 1)
            {
                
                openProfile(i);
                vTaskDelay(pdMS_TO_TICKS(150));
            }
            if(!is_send[i] && status == 2)
            {
                uint8_t cmd[] = {0x51, 0x2B, 0x01, 0x00, 0x00, 0x00, 0xA3, 0x20};
                sendCommand(i, 0, 0, cmd, 8);
                if(i == 1)
                    sendCommand(i, 1, 0, cmd, 8);
                is_send[i] = 1;
            }
            if(is_send[i] > 0)
                is_send[i] = (is_send[i] >= 2)? 0 : is_send[i] + 1;
            if(getDataStatus(i, 0, 0))
            {
                printf("notify[%d] : ", i);
                is_send[i] = 0;
                uint8_t rec[32];
                uint8_t len = getNotifyLen(i, 0, 0);
                if(len > 32)
                    printf("over size\n");
                else
                {
                    getNotifyVlaue(i, 0, 0, rec);
                    for(int i = 0; i < len; i++)
                    {
                        printf("%02X ", rec[i]);
                    }
                    printf("\n\n");
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    
    // BleProfileT temp;
    // temp.rssi = 1000;
    // addProfile(temp);
    // temp.rssi = 1001;
    // addProfile(temp);
    // temp.rssi = 1002;
    // insertProfile(1, temp);
    // deleteProfile(0);

    // ProfileNodeT *trip = findProfile(0);
    // while(trip != NULL)
    // {
    //     printf("trival : id : %d, rssi : %d\n", trip->profile_id, trip->profile.rssi);
    //     trip = trip->next;
    // }

    xTaskCreate(test, "test", 0x10000, NULL, 5, NULL);
}

