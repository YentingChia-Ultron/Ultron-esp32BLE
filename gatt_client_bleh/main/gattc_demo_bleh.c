#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "BLE_client.h"


#define REMOTE_SERVICE_UUID        {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x23, 0x15, 0, 0}
#define REMOTE_NOTIFY_CHAR_UUID    {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x24, 0x15, 0, 0}

static const char remote_device_name[] = "FORA D40";
static const uint8_t remote_device_bda[6] = {0xC0, 0x26, 0xDA, 0x03, 0x98, 0x71};

uuid_t myUUID = {
    .service_uuid_type = 2,
    .service_uuid = REMOTE_SERVICE_UUID,
    .char_uuid_type = 2,
    .char_uuid = REMOTE_NOTIFY_CHAR_UUID
};

void app_main(void)
{
    init_BLE(remote_device_name, remote_device_bda, myUUID);
    start_scan();
    while (!get_ble_status());
    printf("connect!!!!\n");
    uint8_t cmd[] = {0x51, 0x2B, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x1F};
    send_command(cmd, 8);
    while (!get_data_status());
    uint8_t *rec = NULL;
    uint8_t len = get_notify_len();
    rec = malloc(len * sizeof(uint8_t));
    get_notify_vlaue(rec);
    for(int i = 0; i < len; i++)
    {
        printf("%d : %02X\n", i , rec[i]);
    }
    free(rec);
    
    
}

