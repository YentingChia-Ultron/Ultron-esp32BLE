#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "BLE_client.h"


#define REMOTE_SERVICE_UUID        {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x23, 0x15, 0, 0}
#define REMOTE_NOTIFY_CHAR_UUID    {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x24, 0x15, 0, 0}

#define GATTS_SERVICE_UUID_TEST_A   {0xFFF0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define GATTS_CHAR_UUID_TEST_A      {0xFFF6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

uuid_t myUUID[2] = {
    {
        .service_uuid = {
            .len = ESP_UUID_LEN_128,
            .uuid = {.uuid128 = REMOTE_SERVICE_UUID},
        },
        .char_uuid = {
            .len = ESP_UUID_LEN_128,
            .uuid = {.uuid128 = REMOTE_NOTIFY_CHAR_UUID},
        }
    },
    {
        .service_uuid = {
            .len = ESP_UUID_LEN_16,
            .uuid = {.uuid16 = GATTS_SERVICE_UUID_TEST_A},
        },
        .char_uuid = {
            .len = ESP_UUID_LEN_16,
            .uuid = {.uuid16 = GATTS_CHAR_UUID_TEST_A},
        }
    }
};

uint8_t bda[2][6] = {{0xC0, 0x26, 0xDA, 0x03, 0x98, 0x71},
                    {0xBC, 0xDD, 0xC2, 0xDF, 0x8D, 0xAA}};

uint8_t is_send[2] = {0};
void test(void *arg)
{
    init_BLE();
    for(uint8_t i = 0; i < 2; i++)
    {
        set_ble_bda(i, bda[i]);
        init_profile(i, myUUID[i]);
    }
    start_scan(-1);
    while (1)
    {
        for(uint8_t i = 0; i < 2; i++)
            printf("status[%d] = %d\n", i, get_ble_status(i));
        printf("\n");
        for(uint8_t i = 0; i < 2; i++)
        {
            uint8_t status = get_ble_status(i);
            if(status == 1)
            {
                open_profile(i);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            if(!is_send[i] && status == 2)
            {
                uint8_t cmd[] = {0x51, 0x2B, 0x01, 0x00, 0x00, 0x00, 0xA3, 0x20};
                send_command(i, cmd, 8);
                is_send[i] = 1;
            }
            if(is_send[i] > 0)
                is_send[i] = (is_send[i] >= 2)? 0 : is_send[i] + 1;
            if(get_data_status(i))
            {
                printf("notify[%d] : ", i);
                is_send[i] = 0;
                uint8_t rec[32];
                uint8_t len = get_notify_len(i);
                if(len > 32)
                    printf("over size\n");
                else
                {
                    get_notify_vlaue(i, rec);
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
    xTaskCreate(test, "test", 0x10000, NULL, 5, NULL);
}

