#ifndef BLE_CLIENT_H_
#define BLE_CLIENT_H_

#include <stdint.h>
#include "esp_gap_ble_api.h"

struct uuids
{
    esp_bt_uuid_t service_uuid;
    esp_bt_uuid_t char_uuid;
};

typedef struct uuids uuid_t;

void send_command(uint8_t app_id, const uint8_t *cmd, int len);
void init_BLE();
void start_scan(int duration);
void stop_scan();
void disconnect_BLE(uint8_t app_id);
void request_read(uint8_t app_id);
void init_profile(uint8_t app_id, uuid_t myUUIDs);
void open_profile(uint8_t app_id);
uint8_t get_ble_status(uint8_t app_id);
bool get_data_status();
uint8_t get_notify_len();
void get_notify_vlaue(uint8_t *target);
uint8_t get_adv_data_len();
void get_adv_data(uint8_t *buff);
int get_rssi(uint8_t app_id);
void set_ble_bda(uint8_t app_id, uint8_t *bda);

#endif /* BLE_CLIENT_H_ */
