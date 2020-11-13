#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ble_clients.h"

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

uint8_t is_send[2] = {0};

uint8_t dev_num = 2;
ConnectInfoT conn_info[2] = {
    {
        .dev_id = 0,
        .bda = {0xC0, 0x26, 0xDA, 0x03, 0x98, 0x71},
        .need_to_connect = true,
    },
    {
        .dev_id = 1,
        .bda = {0xBC, 0xDD, 0xC2, 0xDF, 0x8D, 0xAA},
        .need_to_connect = true,
    }
};

void test(void *arg)
{
    initBle();
    setConnectInfo(conn_info, dev_num);
    startScan(-1);
    uint8_t profile_num = 0;
    uint8_t status[2] = {0};
    uint8_t last_status[2] = {0};
    while (1)
    {
        printf("profile num : %d\n", profile_num);
        for(uint8_t dev_id = 0; dev_id < dev_num; dev_id++)
            printf("state[%d] = %d\n", dev_id, getBleStatus(dev_id));
        printf("\n");
        for(uint8_t dev_id = 0; dev_id < dev_num; dev_id++)
        {
            last_status[dev_id] = status[dev_id];
            status[dev_id] = getBleStatus(dev_id);
            int rssi = getRssi(dev_id);
            // printf("rssi[%d] : %d\n", dev_id, rssi);
            if(status[dev_id] == 0 && last_status[dev_id] == 2)
            {
                printf("delete profile because disconnect\n");
            }
            else if(status[dev_id] == 1 && status[dev_id] != last_status[dev_id] && conn_info[dev_id].need_to_connect)
            {
                ProfileNodeT *temp = findProfile(0);
                uint8_t pro_id = 0;
                while(temp != NULL)
                {
                    if(temp->dev_id == dev_id)
                    {
                        openProfile(pro_id);
                        break;
                    }
                    temp = temp->next;
                    pro_id++;
                }
                if(temp == NULL)
                {
                    if(dev_id == 0)
                        addProfile(dev_id, myUUID1, 1);
                    else
                        addProfile(dev_id, myUUID2, 2);
                    openProfile(profile_num);
                    profile_num++;
                }
                is_send[dev_id] = false;
                vTaskDelay(pdMS_TO_TICKS(150));
            }
            else if(status[dev_id] == 1)
            {
                uint8_t adv_data[64];
                getAdvData(dev_id, adv_data);
                // printf("----- adv data from device %d -----\n", dev_id);
                // for(uint8_t i = 0; i < getAdvLen(dev_id); i++)
                //     printf("%2X ", adv_data[i]);
                // printf("\n------------\n");
            }
            else if(status[dev_id] == 2)
            {
                uint8_t pro_id = 0;
                ProfileNodeT *temp = findProfile(0);
                while(temp != NULL)
                {
                    if(temp->dev_id == dev_id)
                    {
                        if(!is_send[dev_id])
                        {
                            printf("send comm : %d\n", dev_id);
                            if(dev_id == 1)
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
                            is_send[dev_id] = true;
                        }
                        else
                        {
                            if(dev_id == 1)
                            {
                                if(!getDataStatus(pro_id, 0, 0) || !getDataStatus(pro_id, 1, 0))
                                {
                                    printf("not get data yet : %d\n", dev_id);
                                    break;
                                }
                                is_send[dev_id] = false;
                                printf("notify[%d] : \n", dev_id);
                                for(uint8_t i = 0; i < 2; i++)
                                {
                                    uint8_t notify_data[32];
                                    uint8_t len = getNotifyLen(pro_id, i, 0);
                                    if(len > 32)
                                        printf("over size\n");
                                    else
                                    {
                                        getNotifyVlaue(pro_id, i, 0, notify_data);
                                        for(int j = 0; j < len; j++)
                                            printf("%02X ", notify_data[j]);
                                        printf("\n");
                                    }
                                }
                                printf("\n\n");
                            }
                            else
                            {
                                if(!getDataStatus(pro_id, 0, 0))
                                {
                                    printf("not get data yet : %d\n", dev_id);
                                    break;
                                }
                                is_send[dev_id] = false;
                                uint8_t notify_data[32];
                                uint8_t len = getNotifyLen(pro_id, 0, 0);
                                if(len > 32)
                                printf("over size\n");
                                else
                                {
                                    printf("notify[%d] : \n", dev_id);
                                    getNotifyVlaue(pro_id, 0, 0, notify_data);
                                    for(int i = 0; i < len; i++)
                                    {
                                        printf("%02X ", notify_data[i]);
                                    }
                                    printf("\n\n");
                                }
                            }
                        }
                        break;
                    }
                    temp = temp->next;
                    pro_id++;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1500));
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

