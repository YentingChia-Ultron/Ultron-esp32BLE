/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/



/****************************************************************************
*
* This file is for gatt client. It can scan ble device, connect multiple devices,
* The gattc_multi_connect demo can connect three ble slaves at the same time.
* Modify the name of gatt_server demo named ESP_GATTS_DEMO_a,
* ESP_GATTS_DEMO_b and ESP_GATTS_DEMO_c,then run three demos,the gattc_multi_connect demo will connect
* the three gatt_server demos, and then exchange data.
* Of course you can also modify the code to connect more devices, we default to connect
* up to 4 devices, more than 4 you need to modify menuconfig.
*
****************************************************************************/

//ver.06

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define GATTC_TAG "GATTC_MULTIPLE_DEMO"
#define REMOTE_SERVICE_UUID        0xFFF0
#define REMOTE_NOTIFY_CHAR_UUID    0xFFF6
#define REMOTE_SERVICE_UUID2        0xC050
#define REMOTE_NOTIFY_CHAR_UUID2    0xC05A

/* register three profiles, each profile corresponds to one connection,
   which makes it easy to handle each connection event */
#define CONN_DEVICE_NUM  3
#define PROFILE_NUM 3
#define INVALID_HANDLE   0

#define BUTTON0   2
#define BUTTON1   4

/* Declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gattc_profile_0_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

static esp_bt_uuid_t remote_filter_char_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = REMOTE_NOTIFY_CHAR_UUID,},
};
static esp_bt_uuid_t remote_filter_char_uuid2 = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = REMOTE_NOTIFY_CHAR_UUID2,},
};

static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};

static int connectedNum = 0;
static bool get_service[CONN_DEVICE_NUM] = {0};
static bool Isconnecting    = false;
static bool stop_scan_done  = false;

static esp_gattc_char_elem_t  *char_elem_result_a   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result_a  = NULL;
static esp_gattc_char_elem_t  *char_elem_result_b   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result_b  = NULL;
static esp_gattc_char_elem_t  *char_elem_result_c   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result_c  = NULL;

static const char remote_device_name[3][50] = {"B4", "ESP_GATTS_DEMO_a", "iWEECARE Temp Pal"};
static const uint8_t remote_device_bda[3][6] = {
    {0xBC, 0xDD, 0xC2, 0xDF, 0x8D, 0xAA},
    {0xB4, 0xE6, 0x2D, 0xE9, 0x3A, 0xCF},
    {0x64, 0x69, 0x4E, 0x43, 0x54, 0x19}
};
struct remote_device {
    char name[50];
    uint8_t bda[6];
    uint16_t serviceUUID;
    uint16_t charUUID;
};
static struct remote_device my_remote_device[CONN_DEVICE_NUM];

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    int conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle;
    esp_bd_addr_t remote_bda;
    int connectTo;
    uint16_t serviceUUID;
    esp_bt_uuid_t filter_charUUID;
};
static struct gattc_profile_inst gl_profile_tab[CONN_DEVICE_NUM];

static void start_scan(void)
{
    stop_scan_done = false;
    Isconnecting = false;
    uint32_t duration = 30;
    esp_ble_gap_start_scanning(duration);
}
static void gattc_profile_0_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    int connetID = 0;
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
    case ESP_GATTC_REG_EVT:
        ESP_LOGI(GATTC_TAG, "REG_EVT");
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (scan_ret){
            ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
        }
        break;
    /* one device connect successfully, all profiles callback function will get the ESP_GATTC_CONNECT_EVT,
     so must compare the mac address to check which device is connected, so it is a good choice to use ESP_GATTC_OPEN_EVT. */
    case ESP_GATTC_CONNECT_EVT:
        break;
    case ESP_GATTC_OPEN_EVT:
        if (p_data->open.status != ESP_GATT_OK){
            //open failed, ignore the first device, connect the second device
            ESP_LOGE(GATTC_TAG, "connect device failed, status %d", p_data->open.status);
            connectedNum--;
            gl_profile_tab[connetID].connectTo = -1;
            gl_profile_tab[connetID].conn_id = -1;
            start_scan();
            break;
        }
        connetID = p_data->open.conn_id;
        //printf("p_data->reg.app_id : %d    p_data->open.conn_id : %d\n",(int)p_data->reg.app_id ,(int)p_data->open.conn_id);
        memcpy(gl_profile_tab[connetID].remote_bda, p_data->open.remote_bda, 6);
        gl_profile_tab[connetID].conn_id = p_data->open.conn_id;
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_OPEN_EVT conn_id %d, if %d, status %d, mtu %d", p_data->open.conn_id, gattc_if, p_data->open.status, p_data->open.mtu);
        ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATTC_TAG, p_data->open.remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"Config mtu failed");
        }
        ESP_LOGI(GATTC_TAG, "Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, NULL);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: {
        connetID = p_data->search_res.conn_id;
        //printf("p_data->reg.app_id : %d \n",(int)p_data->search_res.conn_id);
        ESP_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        printf("--------------------------\n");
        printf("SERVICE UUID : %2X\n", p_data->search_res.srvc_id.uuid.uuid.uuid16);
        uint16_t mycount = 0;
        esp_gattc_char_elem_t *myresult = NULL;
        uint16_t char_offset = 0;
        esp_ble_gattc_get_attr_count( gattc_if,
                                      p_data->search_res.conn_id,
                                      ESP_GATT_DB_ALL,
                                      p_data->search_res.start_handle,
                                      p_data->search_res.end_handle,
                                      INVALID_HANDLE,
                                      &mycount);
        printf("count : %d\n",mycount);
        myresult = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * mycount);
        esp_gatt_status_t status = esp_ble_gattc_get_all_char( gattc_if,
                                    p_data->search_res.conn_id,
                                    p_data->search_res.start_handle,
                                    p_data->search_res.end_handle,
                                    myresult,
                                    &mycount, char_offset);
        if (status != ESP_GATT_OK){
                printf("esp_ble_gattc_get_all_char error %2X\n",status);
            }
            else{
        for(int i = 0; i < mycount; i++)
            printf("CHAR[%d] UUID : %2X\n", i, myresult[i].uuid.uuid.uuid16);
            }
        free(myresult);
        printf("--------------------------\n");
        if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == gl_profile_tab[connetID].serviceUUID) {
            ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
            get_service[connetID] = true;
            gl_profile_tab[connetID].service_start_handle = p_data->search_res.start_handle;
            gl_profile_tab[connetID].service_end_handle = p_data->search_res.end_handle;
        }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
        connetID = p_data->search_cmpl.conn_id;
        //printf("p_data->reg.app_id : %d \n",(int)p_data->search_cmpl.conn_id);
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }
        if (get_service[connetID]){
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     gl_profile_tab[connetID].service_start_handle,
                                                                     gl_profile_tab[connetID].service_end_handle,
                                                                     INVALID_HANDLE,
                                                                     &count);
            if (status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }
            if (count > 0) {
                char_elem_result_a = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                if (!char_elem_result_a){
                    ESP_LOGE(GATTC_TAG, "gattc no mem");
                }else {
                    status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                             p_data->search_cmpl.conn_id,
                                                             gl_profile_tab[connetID].service_start_handle,
                                                             gl_profile_tab[connetID].service_end_handle,
                                                             gl_profile_tab[connetID].filter_charUUID,
                                                             char_elem_result_a,
                                                             &count);
                    if (status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
                    }

                    /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                    if (count > 0 && (char_elem_result_a[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
                        gl_profile_tab[connetID].char_handle = char_elem_result_a[0].char_handle;
                        esp_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[connetID].remote_bda, char_elem_result_a[0].char_handle);
                    }
                }
                /* free char_elem_result */
                free(char_elem_result_a);
            }else {
                ESP_LOGE(GATTC_TAG, "no char found");
            }
        }
        break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: 
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "reg notify failed, error status =%x", p_data->reg_for_notify.status);
            break;
        }
        for(int i = 0 ; i < CONN_DEVICE_NUM ; i++)
        {
        uint16_t count = 0;
        uint16_t notify_en = 1;
        esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     gl_profile_tab[i].conn_id,
                                                                     ESP_GATT_DB_DESCRIPTOR,
                                                                     gl_profile_tab[i].service_start_handle,
                                                                     gl_profile_tab[i].service_end_handle,
                                                                     gl_profile_tab[i].char_handle,
                                                                     &count);
        if (ret_status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
        }
        if (count > 0){
            descr_elem_result_a = (esp_gattc_descr_elem_t *)malloc(sizeof(esp_gattc_descr_elem_t) * count);
            if (!descr_elem_result_a){
                ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
            }else{
                ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                     gl_profile_tab[i].conn_id,
                                                                     p_data->reg_for_notify.handle,
                                                                     notify_descr_uuid,
                                                                     descr_elem_result_a,
                                                                     &count);
                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                }

                /* Every char has only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
                if (count > 0 && descr_elem_result_a[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result_a[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
                    ret_status = esp_ble_gattc_write_char_descr( gattc_if,
                                                                 gl_profile_tab[i].conn_id,
                                                                 descr_elem_result_a[0].handle,
                                                                 sizeof(notify_en),
                                                                 (uint8_t *)&notify_en,
                                                                 ESP_GATT_WRITE_TYPE_RSP,
                                                                 ESP_GATT_AUTH_REQ_NONE);
                }

                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                }

                /* free descr_elem_result */
                free(descr_elem_result_a);
            }
        }
        else{
            ESP_LOGE(GATTC_TAG, "decsr not found");
        }

        }
        break;
    
    case ESP_GATTC_NOTIFY_EVT:
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, Receive notify value:");
        esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
        printf("notify.value\n");
        for(int i = 0; i < p_data->notify.value_len; i++){
            printf("%2X ", p_data->notify.value[i]);}
        printf("\n");
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        connetID = p_data->write.conn_id;
        //printf("write : %d \n",(int)p_data->write.conn_id);
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write descr success");
        uint8_t write_char_data[35];
        for (int i = 0; i < sizeof(write_char_data); ++i)
        {
            write_char_data[i] = i % 256;
        }
        esp_ble_gattc_write_char( gattc_if,
                                  gl_profile_tab[connetID].conn_id,
                                  gl_profile_tab[connetID].char_handle,
                                  sizeof(write_char_data),
                                  write_char_data,
                                  ESP_GATT_WRITE_TYPE_RSP,
                                  ESP_GATT_AUTH_REQ_NONE);
        break;
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
        }else{
            ESP_LOGI(GATTC_TAG, "Write char success");
        }
        start_scan();
        break;
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:%08x%04x",(bda[0] << 24) + (bda[1] << 16) + (bda[2] << 8) + bda[3],
                 (bda[4] << 8) + bda[5]);
        break;
    }
    case ESP_GATTC_DISCONNECT_EVT:

        for (int i = 0; i < CONN_DEVICE_NUM; ++i)
        {
            if (gl_profile_tab[i].connectTo != -1 && memcmp(p_data->disconnect.remote_bda, gl_profile_tab[i].remote_bda, 6) == 0){
                printf("device a disconnect : %d \n", i);
                ESP_LOGI(GATTC_TAG, "device a disconnect");
                connectedNum--;
                gl_profile_tab[i].connectTo = -1;
                get_service[(int)gl_profile_tab[i].conn_id] = false;
                gl_profile_tab[i].conn_id = -1;
                start_scan();
                break;  
            }
        }
        break;
    default:
        break;
    }
}
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;
    switch (event) {
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(GATTC_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        //the unit of the duration is second
        uint32_t duration = 30;
        esp_ble_gap_start_scanning(duration);
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(GATTC_TAG, "Scan start success");
        }else{
            ESP_LOGE(GATTC_TAG, "Scan start failed");
        }
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
            ESP_LOGI(GATTC_TAG, "Searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
            ESP_LOGI(GATTC_TAG, "Searched Device Name Len %d", adv_name_len);
            esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);
            ESP_LOGI(GATTC_TAG, "\n");
            if (Isconnecting){
                break;
            }
            if (connectedNum == CONN_DEVICE_NUM && !stop_scan_done){
                stop_scan_done = true;
                esp_ble_gap_stop_scanning();
                ESP_LOGI(GATTC_TAG, "all devices are connected");
                break;
            }
            for(int i = 0 ; i < CONN_DEVICE_NUM; i++)
                {
                    if (memcmp(scan_result->scan_rst.bda, my_remote_device[i].bda, 6) == 0)
                    {
                        printf("scan_rst.bda from : %s\n", my_remote_device[i].name);
                        for(int k = 0; k < 6; k++){
                            printf("%2X ", scan_result->scan_rst.bda[k]);}
                        printf("\n");
                        connectedNum++;
                        printf("connectedNum : %d \n",connectedNum);
                        ESP_LOGI(GATTC_TAG, "Searched device %s", my_remote_device[i].name);
                        esp_ble_gap_stop_scanning();
                        for(int j = 0; j < CONN_DEVICE_NUM; j++)
                        {
                            if(gl_profile_tab[j].conn_id == -1)
                            {
                                gl_profile_tab[j].connectTo = i;
                                gl_profile_tab[j].serviceUUID = my_remote_device[i].serviceUUID;
                                gl_profile_tab[j].filter_charUUID.len = ESP_UUID_LEN_16;
                                gl_profile_tab[j].filter_charUUID.uuid.uuid16 = my_remote_device[i].charUUID;
                                break;
                            }
                        }
                        esp_ble_gattc_open(gl_profile_tab[i].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
                        Isconnecting = true;
                        break;
                    }
                }
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "Scan stop failed");
            break;
        }
        ESP_LOGI(GATTC_TAG, "Stop scan successfully");

        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "Adv stop failed");
            break;
        }
        ESP_LOGI(GATTC_TAG, "Stop adv successfully");
        break;

    default:
        break;
    }
}

void send_command(char *cmd, int len, int profileId) 
{
    uint8_t write_char_data[len];
    for(int i = 0 ; i < len ; i++) {
        write_char_data[i] = cmd[i];
    }
    esp_ble_gattc_write_char( gl_profile_tab[profileId].gattc_if,
                              gl_profile_tab[profileId].conn_id,
                              gl_profile_tab[profileId].char_handle,
                              sizeof(write_char_data),
                              write_char_data,
                              ESP_GATT_WRITE_TYPE_RSP,
                              ESP_GATT_AUTH_REQ_NONE);
}

static int cnt= 0;
static bool lastState[2] = {1, 1};
static void getConnState(void *pvParameters)
{
    while(1)
    {
        printf("\n----------\nsecond : %d\n", ++cnt);
        // printf("conn_devices: \n");
        // for(int i = 0 ; i < CONN_DEVICE_NUM ; i++){
        //     bool found = false;
        //     for(int j = 0 ; j < CONN_DEVICE_NUM ; j++)
        //     {
        //         if(gl_profile_tab[j].connectTo == i)
        //         {
        //             found = true;
        //             break;
        //         }
        //     }
        //     printf("%s : %d \n", my_remote_device[i].name, found);
        // }
        for(int j = 0 ; j < CONN_DEVICE_NUM ; j++)
        {
            printf("gl_profile_tab[%d].connid = %d  gl_profile_tab[%d].connTo = %d\n",j, gl_profile_tab[j].conn_id, j, gl_profile_tab[j].connectTo);
        }
        printf("connectedNum : %d \n",connectedNum);
        printf("----------\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
static void sendButtCmd(void *pvParameters)
{
    while(1)
    {
        printf("into sendButtCmd!!   \n");
        const char cmd[2][4] = {"GOUP", "GODN"};
        bool buttState[2] = {gpio_get_level(BUTTON0), gpio_get_level(BUTTON1)};
        printf("\nbuttState : %d   %d\n", buttState[0], buttState[1]);
        if(buttState[0] == 0 && lastState[0] != buttState[0])
        {     
            printf("GOUP!!\n");
            for(int i = 0 ; i < CONN_DEVICE_NUM -1; i++) {
                send_command(cmd[0], 4, i);
            }
        }
        if(buttState[1] == 0 && lastState[1] != buttState[1])
        {
            printf("GODN!!\n");
            for(int i = 0 ; i < CONN_DEVICE_NUM -1; i++) {
                send_command(cmd[1], 4, i);
            }
        }
        lastState[0] = buttState[0];
        lastState[1] = buttState[1];
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    //ESP_LOGI(GATTC_TAG, "EVT %d, gattc if %d, app_id %d", event, gattc_if, param->reg.app_id);

    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "Reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == gl_profile_tab[idx].gattc_if) {
                if (gl_profile_tab[idx].gattc_cb) {
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}

void app_main(void)
{
    //init
    for(int i = 0 ; i < CONN_DEVICE_NUM; i++) 
    {
        gl_profile_tab[i].gattc_cb = gattc_profile_0_event_handler;
        gl_profile_tab[i].gattc_if = ESP_GATT_IF_NONE;
        gl_profile_tab[i].connectTo = -1;
        gl_profile_tab[i].conn_id = -1;
        memcpy(my_remote_device[i].bda, remote_device_bda[i], 6);
        memcpy(my_remote_device[i].name , remote_device_name[i], 50);
        my_remote_device[i].serviceUUID = REMOTE_SERVICE_UUID;
        my_remote_device[i].charUUID = REMOTE_NOTIFY_CHAR_UUID;
    }
    my_remote_device[CONN_DEVICE_NUM - 1].serviceUUID = REMOTE_SERVICE_UUID2;
    my_remote_device[CONN_DEVICE_NUM - 1].charUUID = REMOTE_NOTIFY_CHAR_UUID2;

    gpio_set_pull_mode(BUTTON0, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(BUTTON1, GPIO_PULLUP_ONLY);
    gpio_set_direction(BUTTON0, GPIO_MODE_INPUT);
    gpio_set_direction(BUTTON1, GPIO_MODE_INPUT);
    //xTaskCreate(&sendButtCmd, "sendButtCmd", 4096, NULL, 15, NULL);
    xTaskCreate(&getConnState, "getConnState", 4096, NULL, 15, NULL);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    //register the  callback function to the gap module
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret){
        ESP_LOGE(GATTC_TAG, "gap register error, error code = %x", ret);
        return;
    }

    //register the callback function to the gattc module
    ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    if(ret){
        ESP_LOGE(GATTC_TAG, "gattc register error, error code = %x", ret);
        return;
    }

    for(int i = 0 ; i < CONN_DEVICE_NUM ; i++) {
        ret = esp_ble_gattc_app_register(i);
        if (ret){
            ESP_LOGE(GATTC_TAG, "gattc app register %d error, error code = %x", i, ret);
            return;
        }
    }
    ret = esp_ble_gatt_set_local_mtu(200);
    if (ret){
        ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", ret);
    }


}

