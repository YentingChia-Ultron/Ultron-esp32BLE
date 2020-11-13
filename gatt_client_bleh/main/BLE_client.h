#ifndef BLE_CLIENT_H_
#define BLE_CLIENT_H_

#include <stdint.h>
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"

#define MAX_SERVICE_NUM      5
#define MAX_CHAR_NUM      5
#define MAX_PROFILE_NUM      CONFIG_BTDM_CTRL_BLE_MAX_CONN

struct Uuids
{
    esp_bt_uuid_t service_uuid;
    uint8_t char_num;
    esp_bt_uuid_t *char_uuid;
};
typedef struct Uuids UuidsT;

struct BleChar {
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    uint8_t notify_value[32];
    uint8_t notify_len;
    bool have_data;
};
typedef struct BleChar BleCharT;

struct BleService {
    esp_bt_uuid_t service_uuid;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint8_t char_num;
    BleCharT *chars;
};
typedef struct BleService BleServiceT;

struct BleProfile{
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    esp_bd_addr_t remote_bda;
    uint8_t adv_data[64];
    uint8_t adv_data_len;
    int rssi;
    uint8_t service_num;
    BleServiceT *services;
};
typedef struct BleProfile BleProfileT;

struct ProfileNode
{
    uint8_t dev_id;
    uint8_t profile_id;
	BleProfileT profile;
	struct ProfileNode *next;
};
typedef struct ProfileNode ProfileNodeT;

struct ConnectInfo{
    uint8_t dev_id;
    uint8_t bda[6];
    bool need_to_connect;
    uint8_t status;
    int rssi;
    uint8_t adv_data[64];
    uint8_t adv_data_len;
};
typedef struct ConnectInfo ConnectInfoT;

void setConnectInfo(ConnectInfoT* infos, uint8_t size);

void addProfile(uint8_t dev_id, UuidsT *myUUIDs, uint8_t service_num);
void deleteProfile(uint8_t profile_id);
ProfileNodeT *findProfile(uint8_t profile_id);

void sendCommand(uint8_t profile_id, uint8_t s_id, uint8_t ch_id, const uint8_t *cmd, int len);
void initBle();
void startScan(int duration);
void stopScan();
uint8_t getAdvLen(uint8_t dev_id);
void getAdvData(uint8_t dev_id, uint8_t *buff);
int getRssi(uint8_t dev_id);
uint8_t getBleStatus(uint8_t dev_id);
void disconnectBle(uint8_t profile_id);
void requestRead(uint8_t profile_id, uint8_t s_id, uint8_t ch_id);
void openProfile(uint8_t profile_id);
bool getDataStatus(uint8_t profile_id, uint8_t s_id, uint8_t ch_id);
uint8_t getNotifyLen(uint8_t profile_id, uint8_t s_id, uint8_t ch_id);
void getNotifyVlaue(uint8_t profile_id, uint8_t s_id, uint8_t ch_id, uint8_t *target);
void setBleBda(uint8_t profile_id, uint8_t *bda);

#endif /* BLE_CLIENT_H_ */
