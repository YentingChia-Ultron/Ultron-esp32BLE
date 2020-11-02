#ifndef BLE_CLIENT_H_
#define BLE_CLIENT_H_

#include <stdint.h>
#include "esp_gap_ble_api.h"

struct uuids
{
    uint8_t service_uuid_type;   //0 : 16, 1 : 32, 2 : 128
    uint8_t service_uuid[ESP_UUID_LEN_128];
    uint8_t char_uuid_type;
    uint8_t char_uuid[ESP_UUID_LEN_128];
};

typedef struct uuids uuid_t;

void send_command(const uint8_t *cmd, int len);
void init_BLE(char *name, uint8_t *bda,  uuid_t myUUIDs);
void start_scan();
void stop_scan();
void disconnect_BLE();
bool get_ble_status();
bool get_data_status();
uint8_t get_notify_len();
void get_notify_vlaue(uint8_t *target);

#endif /* BLE_CLIENT_H_ */
