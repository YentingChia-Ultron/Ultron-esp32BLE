#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
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

#include "BLE_client.h"


#define GATTC_TAG "GATTC_DEMO_BLEH"
#define REMOTE_SERVICE_UUID        {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x23, 0x15, 0, 0}
#define REMOTE_NOTIFY_CHAR_UUID    {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x24, 0x15, 0, 0}
#define MAX_PROFILE_NUM      CONFIG_BTDM_CTRL_BLE_MAX_CONN
#define PROFILE_A_APP_ID 0
#define INVALID_HANDLE   0


static uint8_t remote_device_bda[2][6];
static uint8_t adv_data[MAX_PROFILE_NUM][64];
static uint8_t adv_data_len[MAX_PROFILE_NUM] = {0};
static int ble_rssi[MAX_PROFILE_NUM] = {0};

static uint8_t ble_status[MAX_PROFILE_NUM] = {0}; //1 : find, 2 : connect
static bool get_server[MAX_PROFILE_NUM] = {0};
static bool get_data[MAX_PROFILE_NUM]   = {0};
uint8_t notify_vlaue[MAX_PROFILE_NUM][32];
uint8_t notify_len[MAX_PROFILE_NUM] = {0};
bool scan_forever = false;
static esp_gattc_char_elem_t *char_elem_result   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result = NULL;




/* Declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);


static esp_bt_uuid_t remote_filter_service_uuid[MAX_PROFILE_NUM];

static esp_bt_uuid_t remote_filter_char_uuid[MAX_PROFILE_NUM];

static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};

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
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle;
    esp_bd_addr_t remote_bda;
};

/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
static struct gattc_profile_inst gl_profile_tab[MAX_PROFILE_NUM];


void send_command(uint8_t app_id, const uint8_t *cmd, int len) {
    get_data[app_id] = false;
    printf("\nsend cmd : ");
    for(int i = 0 ; i < len ; i++) {
        printf("%02X ", cmd[i]);
    }
    printf("\n");
    esp_ble_gattc_write_char( gl_profile_tab[app_id].gattc_if,
                              gl_profile_tab[app_id].conn_id,
                              gl_profile_tab[app_id].char_handle,
                              len,
                              cmd,
                              ESP_GATT_WRITE_TYPE_RSP,
                              ESP_GATT_AUTH_REQ_NONE);
}

void request_read(uint8_t app_id) {
    esp_ble_gattc_read_char( gl_profile_tab[app_id].gattc_if,
                              gl_profile_tab[app_id].conn_id,
                              gl_profile_tab[app_id].char_handle,
                              ESP_GATT_AUTH_REQ_NONE);
}

bool service_set[MAX_PROFILE_NUM] = {0};
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;
    switch (event) {
    case ESP_GATTC_REG_EVT:
        ESP_LOGI(GATTC_TAG, "REG_EVT");
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (scan_ret){
            ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
        }
        break;
    case ESP_GATTC_CONNECT_EVT:
        break;
    case ESP_GATTC_OPEN_EVT:
        if (param->open.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "open failed, status %02X", p_data->open.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "open success");
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_OPEN_EVT conn_id %d, if %d", p_data->open.conn_id, gattc_if);
        uint8_t app_id = 0;
        for (;app_id < MAX_PROFILE_NUM; app_id++)
        {
            if(gl_profile_tab[app_id].gattc_if == gattc_if)
                break;
        }
        memcpy(gl_profile_tab[app_id].remote_bda, p_data->open.remote_bda, 6);
        gl_profile_tab[app_id].conn_id = p_data->open.conn_id;
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }
        break;
    case ESP_GATTC_DIS_SRVC_CMPL_EVT:
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, NULL);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: {


        // printf("\n--------------------------\n");
        // printf("\nsearch_res: ");
        // printf("%X", p_data->search_res.srvc_id.uuid.uuid.uuid16);
        // printf("\n...\n");
        // for(int i =0 ; i<ESP_UUID_LEN_128;i++)
        // printf("%X ",p_data->search_res.srvc_id.uuid.uuid.uuid128[i]);
        // printf("\n");
        // uint16_t mycount = 0;
        // esp_gattc_char_elem_t *myresult = NULL;
        // uint16_t char_offset = 0;
        // esp_ble_gattc_get_attr_count( gattc_if,
        //                               p_data->search_res.conn_id,
        //                               ESP_GATT_DB_ALL,
        //                               p_data->search_res.start_handle,
        //                               p_data->search_res.end_handle,
        //                               INVALID_HANDLE,
        //                               &mycount);
        // printf("count : %d\n",mycount);
        // myresult = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * mycount);
        // esp_gatt_status_t status = esp_ble_gattc_get_all_char( gattc_if,
        //                             p_data->search_res.conn_id,
        //                             p_data->search_res.start_handle,
        //                             p_data->search_res.end_handle,
        //                             myresult,
        //                             &mycount, char_offset);
        // if (status != ESP_GATT_OK){
        //         printf("esp_ble_gattc_get_all_char error %2X\n",status);
        //     }
        //     else{
        //         for(int i = 0; i < mycount; i++){
        //         printf("CHAR[%d] UUID : %2X\n", i, myresult[i].uuid.uuid.uuid16);
        //         printf("\n.....\n");
        //             for(int j =0 ; j<ESP_UUID_LEN_128;j++)
        //                 printf("%X ",myresult[i].uuid.uuid.uuid128[j]);
        //         }
        //         printf("\n");
        //     }
        // free(myresult);
        // printf("\n--------------------------\n");

        ESP_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        uint8_t app_id = 0;
        for (;app_id < MAX_PROFILE_NUM; app_id++)
        {
            if(gl_profile_tab[app_id].gattc_if == gattc_if)
                break;
        }
        if (remote_filter_service_uuid[app_id].len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == remote_filter_service_uuid[app_id].uuid.uuid16)
            get_server[app_id] = true;
        if (remote_filter_service_uuid[app_id].len == ESP_UUID_LEN_32 && p_data->search_res.srvc_id.uuid.uuid.uuid32 == remote_filter_service_uuid[app_id].uuid.uuid32)
            get_server[app_id] = true;
        else if (remote_filter_service_uuid[app_id].len == ESP_UUID_LEN_128 && memcmp(p_data->search_res.srvc_id.uuid.uuid.uuid128, remote_filter_service_uuid[app_id].uuid.uuid128, ESP_UUID_LEN_128) == 0)
            get_server[app_id] = true;
        if (get_server[app_id] && !service_set[app_id]) {
            service_set[app_id] = true;
            ESP_LOGI(GATTC_TAG, "service found");
            gl_profile_tab[app_id].service_start_handle = p_data->search_res.start_handle;
            gl_profile_tab[app_id].service_end_handle = p_data->search_res.end_handle;
            if(remote_filter_service_uuid[app_id].len == ESP_UUID_LEN_16)
                ESP_LOGI(GATTC_TAG, "UUID16: %04X", p_data->search_res.srvc_id.uuid.uuid.uuid16);
            else if(remote_filter_service_uuid[app_id].len == ESP_UUID_LEN_32)
                ESP_LOGI(GATTC_TAG, "UUID32: %04X", p_data->search_res.srvc_id.uuid.uuid.uuid32);
            else
                esp_log_buffer_hex(GATTC_TAG, p_data->search_res.srvc_id.uuid.uuid.uuid128, remote_filter_service_uuid[app_id].len);
            
        }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }
        if(p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
            ESP_LOGI(GATTC_TAG, "Get service information from remote device");
        } else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH) {
            ESP_LOGI(GATTC_TAG, "Get service information from flash");
        } else {
            ESP_LOGI(GATTC_TAG, "unknown service source");
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SEARCH_CMPL_EVT");
        app_id = 0;
        for (;app_id < MAX_PROFILE_NUM; app_id++)
        {
            if(gl_profile_tab[app_id].gattc_if == gattc_if)
                break;
        }
        if (get_server[app_id]){
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     gl_profile_tab[app_id].service_start_handle,
                                                                     gl_profile_tab[app_id].service_end_handle,
                                                                     INVALID_HANDLE,
                                                                     &count);
            if (status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }

            if (count > 0){
                char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                if (!char_elem_result){
                    ESP_LOGE(GATTC_TAG, "gattc no mem");
                }else{
                    status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                             p_data->search_cmpl.conn_id,
                                                             gl_profile_tab[app_id].service_start_handle,
                                                             gl_profile_tab[app_id].service_end_handle,
                                                             remote_filter_char_uuid[app_id],
                                                             char_elem_result,
                                                             &count);
                    if (status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error : %d", status);
                    }
                    ESP_LOGI(GATTC_TAG, "char_elem_result count = %d   ", count);
                    /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                    if (count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
                        gl_profile_tab[app_id].char_handle = char_elem_result[0].char_handle;
                        esp_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[app_id].remote_bda, char_elem_result[0].char_handle);
                    }
                }
                /* free char_elem_result */
                free(char_elem_result);
            }else{
                ESP_LOGE(GATTC_TAG, "no char found");
            }
        }
         break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
        uint8_t app_id = 0;
        for (;app_id < MAX_PROFILE_NUM; app_id++)
        {
            if(gl_profile_tab[app_id].gattc_if == gattc_if)
                break;
        }
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "REG FOR NOTIFY failed: error status = %d", p_data->reg_for_notify.status);
        }else{
            uint16_t count = 0;
            uint16_t notify_en = 1;
            esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                         gl_profile_tab[app_id].conn_id,
                                                                         ESP_GATT_DB_DESCRIPTOR,
                                                                         gl_profile_tab[app_id].service_start_handle,
                                                                         gl_profile_tab[app_id].service_end_handle,
                                                                         gl_profile_tab[app_id].char_handle,
                                                                         &count);
            if (ret_status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }
            if (count > 0){
                descr_elem_result = malloc(sizeof(esp_gattc_descr_elem_t) * count);
                if (!descr_elem_result){
                    ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
                }else{
                    ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                         gl_profile_tab[app_id].conn_id,
                                                                         p_data->reg_for_notify.handle,
                                                                         notify_descr_uuid,
                                                                         descr_elem_result,
                                                                         &count);
                    if (ret_status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                    }
                    /* Every char has only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
                    if (count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
                        ret_status = esp_ble_gattc_write_char_descr( gattc_if,
                                                                     gl_profile_tab[app_id].conn_id,
                                                                     descr_elem_result[0].handle,
                                                                     sizeof(notify_en),
                                                                     (uint8_t *)&notify_en,
                                                                     ESP_GATT_WRITE_TYPE_RSP,
                                                                     ESP_GATT_AUTH_REQ_NONE);
                    }

                    if (ret_status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                    }

                    /* free descr_elem_result */
                    free(descr_elem_result);
                }
            }
            else{
                ESP_LOGE(GATTC_TAG, "decsr not found");
            }

        }
        break;
    }
    case ESP_GATTC_NOTIFY_EVT:
        ESP_LOGE(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT");
        app_id = 0;
        for (;app_id < MAX_PROFILE_NUM; app_id++)
        {
            if(gl_profile_tab[app_id].gattc_if == gattc_if)
                break;
        }
        esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
        notify_len[app_id] = p_data->notify.value_len;
        memcpy(notify_vlaue[app_id], p_data->notify.value, notify_len[app_id]);
        get_data[app_id] = true;
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        app_id = 0;
        for (;app_id < MAX_PROFILE_NUM; app_id++)
        {
            if(gl_profile_tab[app_id].gattc_if == gattc_if)
                break;
        }
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write descr success ");
        if(ble_status[app_id] != 2){
            ble_status[app_id] = 2;
            start_scan(-1);
        }
        uint8_t write_char_data[35];
        for (int i = 0; i < sizeof(write_char_data); ++i)
        {
            write_char_data[i] = i % 256;
        }
        esp_ble_gattc_write_char( gattc_if,
                                  gl_profile_tab[app_id].conn_id,
                                  gl_profile_tab[app_id].char_handle,
                                  sizeof(write_char_data),
                                  write_char_data,
                                  ESP_GATT_WRITE_TYPE_RSP,
                                  ESP_GATT_AUTH_REQ_NONE);
        break;
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:");
        esp_log_buffer_hex(GATTC_TAG, bda, sizeof(esp_bd_addr_t));
        break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
            break;
        }
        app_id = 0;
        for (;app_id < MAX_PROFILE_NUM; app_id++)
        {
            if(gl_profile_tab[app_id].gattc_if == gattc_if)
                break;
        }
        if(ble_status[app_id] != 2){
            ble_status[app_id] = 2;
            start_scan(-1);
        }
        ESP_LOGI(GATTC_TAG, "write char success ");
        break;
    case ESP_GATTC_DISCONNECT_EVT:
        app_id = 0;
        for (;app_id < MAX_PROFILE_NUM; app_id++)
        {
            if(gl_profile_tab[app_id].gattc_if == gattc_if)
                break;
        }
        if(memcmp(p_data->disconnect.remote_bda, gl_profile_tab[app_id].remote_bda, 6) == 0)
        {
            if(ble_status[app_id] == 2)
                ble_status[app_id] = 0;
            get_server[app_id] = false;
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_DISCONNECT_EVT, app_id = %d, reason = %d", app_id, p_data->disconnect.reason);
            printf("DISCONNECT!\n");
            start_scan(-1); 
        }
        break;
    case ESP_GATTC_READ_CHAR_EVT:
        if (p_data->read.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "read char failed, error status = %x", p_data->read.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "read char success : %d", p_data->read.value_len);
        break;
    default:
        break;
    }
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;
    uint8_t app_id = 0;
    switch (event) {
    case ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT:
        for(;app_id < MAX_PROFILE_NUM; app_id++)
        {
            if(memcmp(param->read_rssi_cmpl.remote_addr, remote_device_bda[app_id], 6) == 0)
                break;
        }
        printf("ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT : %d\n", param->read_rssi_cmpl.rssi);
        ble_rssi[app_id] = param->read_rssi_cmpl.rssi;
        break;
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTC_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "scan start success");

        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            // esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
            // ESP_LOGI(GATTC_TAG, "searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
            // ESP_LOGI(GATTC_TAG, "searched Device Name Len %d", adv_name_len);
            // esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);

#if CONFIG_EXAMPLE_DUMP_ADV_DATA_AND_SCAN_RESP
            if (scan_result->scan_rst.adv_data_len > 0) {
                ESP_LOGI(GATTC_TAG, "adv data:");
                esp_log_buffer_hex(GATTC_TAG, &scan_result->scan_rst.ble_adv[0], scan_result->scan_rst.adv_data_len);
            }
            if (scan_result->scan_rst.scan_rsp_len > 0) {
                ESP_LOGI(GATTC_TAG, "scan resp:");
                esp_log_buffer_hex(GATTC_TAG, &scan_result->scan_rst.ble_adv[scan_result->scan_rst.adv_data_len], scan_result->scan_rst.scan_rsp_len);
            }
#endif
            app_id = 0;
            for(;app_id < MAX_PROFILE_NUM; app_id++)
            {
                if(ble_status[app_id] == 2)
                    continue;
                if (memcmp(scan_result->scan_rst.bda, remote_device_bda[app_id], 6) == 0)
                {
                    ble_rssi[app_id] = scan_result->scan_rst.rssi;
                    adv_data_len[app_id] = scan_result->scan_rst.adv_data_len + scan_result->scan_rst.scan_rsp_len;
                    if(adv_data_len[app_id] > 64)
                        ESP_LOGE(GATTC_TAG, "adv_data_len over 64 : %d", adv_data_len[app_id]);
                    memcpy(adv_data[app_id], scan_result->scan_rst.ble_adv, adv_data_len[app_id]);
                    // ESP_LOGI(GATTC_TAG, "searched device %s\n", adv_name);
                    // ESP_LOGE(GATTC_TAG, "scan adv data:");
                    // esp_log_buffer_hex(GATTC_TAG, adv_data, adv_data_len);
                    ble_status[app_id] = 1;
                    break;
                }
            }
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            printf("ESP_GAP_SEARCH_INQ_CMPL_EVT\n");
            if(scan_forever)
                start_scan(-1);
            break;
        default:
            printf("event : %d", event);
            break;
        }
        break;
    }
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "stop scan successfully");
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "adv stop failed, error status = %x", param->adv_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "stop adv successfully");
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(GATTC_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{    

    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            ESP_LOGW(GATTC_TAG, "EVT %d, gattc if %d, app_id %d", event, gattc_if, param->reg.app_id);
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
            gl_profile_tab[param->reg.app_id].app_id = param->reg.app_id;
        } else {
            ESP_LOGE(GATTC_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gattc_if equal to profile i, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < MAX_PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == gl_profile_tab[idx].gattc_if) {
                if (gl_profile_tab[idx].gattc_cb) {
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}

void start_scan(int duration)
{
    if(duration == -1){
        scan_forever = true;
        duration = 600;
    }
    esp_ble_gap_start_scanning(duration);
}
void stop_scan()
{
    esp_ble_gap_stop_scanning();
}

uint8_t get_ble_status(uint8_t app_id)
{
    return ble_status[app_id];
}
uint8_t get_adv_data_len(uint8_t app_id)
{
    return adv_data_len[app_id];
}

int get_rssi(uint8_t app_id)
{
    if(ble_status[app_id] == 2)
    {
        int last_value = ble_rssi[app_id];
        uint8_t i = 0;
        esp_ble_gap_read_rssi(remote_device_bda[app_id]);
        while(last_value == ble_rssi[app_id] && i < 20)
        {
            i++;
            vTaskDelay(10 / portTICK_RATE_MS);
        }
    }
    return ble_rssi[app_id];
}

void get_adv_data(uint8_t app_id, uint8_t *buff)
{
    memcpy(buff, adv_data[app_id], adv_data_len[app_id]);
}

void disconnect_BLE(uint8_t app_id) {
    if(ble_status[app_id] == 2) {
        ESP_LOGE(GATTC_TAG, "disconnect BLE");
        esp_ble_gap_disconnect(gl_profile_tab[app_id].remote_bda);
    }
}

bool get_data_status(uint8_t app_id) {
    return get_data[app_id];
}

uint8_t get_notify_len(uint8_t app_id) 
{
    return notify_len[app_id];
}

void get_notify_vlaue(uint8_t app_id, uint8_t *target) {
    memcpy(target, notify_vlaue[app_id], notify_len[app_id]);
    get_data[app_id] = false;
}

void set_ble_bda(uint8_t app_id, uint8_t *bda)
{
    memcpy(remote_device_bda[app_id], bda, 6);
}
void init_BLE()
{
    printf("init_BLE\n");
    // Initialize NVS.
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
        ESP_LOGE(GATTC_TAG, "%s gap register failed, error code = %x\n", __func__, ret);
        return;
    }

    //register the callback function to the gattc module
    ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    if(ret){
        ESP_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x\n", __func__, ret);
        return;
    }

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
    for(int i = 0; i < MAX_PROFILE_NUM; i++)
    {
        gl_profile_tab[i].gattc_cb = gattc_profile_event_handler;
        gl_profile_tab[i].gattc_if = ESP_GATT_IF_NONE;
        gl_profile_tab[i].app_id = i;
        esp_err_t ret = esp_ble_gattc_app_register(i);
        if (ret)
            ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);
    }
}

void init_uuid(uint8_t app_id, uuid_t myUUIDs)
{
    printf("init_uuid\n");
    if(myUUIDs.service_uuid.len == ESP_UUID_LEN_16)
    {
        remote_filter_service_uuid[app_id].len = ESP_UUID_LEN_16;
        remote_filter_service_uuid[app_id].uuid.uuid16 = myUUIDs.service_uuid.uuid.uuid16;
    }
    else if(myUUIDs.service_uuid.len  == ESP_UUID_LEN_32)
    {
        remote_filter_service_uuid[app_id].len = ESP_UUID_LEN_32;
        remote_filter_service_uuid[app_id].uuid.uuid32 = myUUIDs.service_uuid.uuid.uuid32;
    }
    else if(myUUIDs.service_uuid.len  == ESP_UUID_LEN_128)
    {
        remote_filter_service_uuid[app_id].len = ESP_UUID_LEN_128;
        memcpy(remote_filter_service_uuid[app_id].uuid.uuid128, myUUIDs.service_uuid.uuid.uuid128, ESP_UUID_LEN_128);
    }
    if(myUUIDs.char_uuid.len == ESP_UUID_LEN_16)
    {
        remote_filter_char_uuid[app_id].len = ESP_UUID_LEN_16;
        remote_filter_char_uuid[app_id].uuid.uuid16 = myUUIDs.char_uuid.uuid.uuid16;
    }
    else if(myUUIDs.char_uuid.len == ESP_UUID_LEN_32)
    {
        remote_filter_char_uuid[app_id].len = ESP_UUID_LEN_32;
        remote_filter_char_uuid[app_id].uuid.uuid32 = myUUIDs.char_uuid.uuid.uuid32;
    }
    else if(myUUIDs.char_uuid.len == ESP_UUID_LEN_128)
    {
        remote_filter_char_uuid[app_id].len = ESP_UUID_LEN_128;
        memcpy(remote_filter_char_uuid[app_id].uuid.uuid128, myUUIDs.char_uuid.uuid.uuid128, ESP_UUID_LEN_128);
    }
}

void open_profile(uint8_t app_id)
{
    printf("open_profile in client.c\n");
    stop_scan();
    vTaskDelay(pdMS_TO_TICKS(50));
    esp_ble_gattc_open(gl_profile_tab[app_id].gattc_if, remote_device_bda[app_id], 0, true);  
}