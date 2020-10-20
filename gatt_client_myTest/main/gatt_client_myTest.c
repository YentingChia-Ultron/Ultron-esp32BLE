/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/



/****************************************************************************
*
* This demo showcases BLE GATT client. It can scan BLE devices and connect to one device.
* Run the gatt_server demo, the client demo will automatically connect to the gatt_server demo.
* Client demo will enable gatt_server's notify after connection. The two devices will then exchange
* data.
*
****************************************************************************/

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
#include "driver/gpio.h"

#include "cli.h"
#include "foraThermometer.h"



#define GATTC_TAG "GATTC_DEMO"
// #define REMOTE_SERVICE_UUID        0xFFF0
// #define REMOTE_NOTIFY_CHAR_UUID    0xFFF1
#define REMOTE_SERVICE_UUID        {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x23, 0x15, 0, 0}
#define REMOTE_NOTIFY_CHAR_UUID    {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x24, 0x15, 0, 0}
#define PROFILE_NUM      1
#define PROFILE_A_APP_ID 0
#define INVALID_HANDLE   0

#define BUTTON0   2

// static const char remote_device_name[] = "UTXXXX-XXXXXXX";
static const char remote_device_name[] = "FORA IR42";
// static const uint8_t remote_device_bda[6] = {0xC0, 0x26, 0xDA, 0x0C, 0x16, 0x36};  //fora th
static const uint8_t remote_device_bda[6] = {0xC0, 0x26, 0xDA, 0x03, 0x98, 0x71};  //fora blood
static bool connect    = false;
static bool get_server = false;
static esp_gattc_char_elem_t *char_elem_result   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result = NULL;
static bool isActivating = 0; 
static uint16_t myTime = 0;
static uint16_t myTick = 0;


bool getFora = false;

static const uint8_t remote_service_uuid[ESP_UUID_LEN_128] = REMOTE_SERVICE_UUID;

/* Declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);


static esp_bt_uuid_t remote_filter_service_uuid = {
    .len = ESP_UUID_LEN_128,
    .uuid = {.uuid128 = REMOTE_SERVICE_UUID,},
};

static esp_bt_uuid_t remote_filter_char_uuid = {
    .len = ESP_UUID_LEN_128,
    .uuid = {.uuid128 = REMOTE_NOTIFY_CHAR_UUID,},
};

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
static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};


static void send_command(const uint8_t *cmd, int len) {
    printf("\nsend cmd : ");
    for(int i = 0 ; i < len ; i++) {
        printf("%02X ", cmd[i]);
    }
    printf("\n");
    esp_ble_gattc_write_char( gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
                              gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                              gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                              len,
                              cmd,
                              ESP_GATT_WRITE_TYPE_RSP,
                              ESP_GATT_AUTH_REQ_NONE);
}

static void request_read() {
    esp_ble_gattc_read_char( gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
                              gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                              gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                              ESP_GATT_AUTH_REQ_NONE);
}
static uint16_t crc16(uint8_t *pD, int len)
{
    static uint16_t poly[2] = {0, 0xa001};
    uint16_t crc = 0xffff;
    int i, j;
    for(j = len; j > 0; j--)
    {
        uint8_t ds = *pD++;
        for(i = 0; i < 8; i++)
        {
            crc = (crc >> 1) ^ poly[(crc ^ ds) & 1];
            ds = ds >> 1;
        }
    }
    return crc;
}

static uint8_t err_cnt = 0;
static uint8_t ultron_activate_data[5120] = {0};
static uint16_t lastInd = 0;
static uint16_t activate_len = 0;
const uint8_t activate_cmd[2][10] = {"start_act", "trans_act"}; //start_act, trans_act
static void ultron_activate(uint8_t *data)
{
    if(err_cnt > 10)
    {
        ESP_LOGE(GATTC_TAG, "ultron activate fail");
        err_cnt = 0;
        isActivating = 0;
        return;
    }
    if((data[0] + data[1]) == 0)//first 
    {
        ESP_LOGW(GATTC_TAG, "ultron activate start !!\n");
        uint16_t crc_16 = (data[4] << 8) | data[5];
        if(crc16(data, 4) != crc_16)
        {
            ESP_LOGE(GATTC_TAG, "ultron active crc different");
            err_cnt++;
            send_command(activate_cmd[0], sizeof(activate_cmd[0]));
            return;
        }
        err_cnt = 0;
        activate_len = (data[2] << 8) | data[3];
        lastInd = 0;
        send_command(activate_cmd[1], sizeof(activate_cmd[1]));
        request_read();
    }
    else
    {
        uint16_t crc_16 = (data[252] << 8) | data[253];
        if(crc16(data, 252) != crc_16)
        {
            ESP_LOGE(GATTC_TAG, "ultron active crc different, \nrecive crc : %04X, here count : %04X", crc_16, crc16(data, 252));
            err_cnt++;
            send_command(activate_cmd[0], sizeof(activate_cmd[0]));
            request_read();
            return;
        }
        if((lastInd + 1)!= ((data[0] << 8) | data[1]))
        {
            ESP_LOGE(GATTC_TAG, "ultron active index different");
            ESP_LOGE(GATTC_TAG,"last : %04X, now : %04X\n", lastInd, ((data[0] << 8) | data[1]));
            err_cnt++;
            send_command(activate_cmd[0], sizeof(activate_cmd[0]));
            return;
        }
        uint16_t now_len = lastInd * 250;
        err_cnt = 0;
        for(uint8_t i = 0; i < 250; i++, now_len++)
        {
            if(now_len >= activate_len)
            {
                printf("\n------ activate data set done ------\ni : %d\nactivate_len : %d\nnow_len : %d \ntime : %d.%d s\n", i, activate_len, now_len, (myTime - myTick)/10, (myTime - myTick)%10);
                // for(int j = 0; j < activate_len; j++)
                //     printf("%02X ", ultron_activate_data[j]);
                // printf("\n------------------\n");
                isActivating = 0;
                return;
            }
            ultron_activate_data[now_len] = data[2 + i];
        }
        lastInd++;
        send_command(activate_cmd[1], sizeof(activate_cmd[1]));
        request_read();
    }
}

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
    case ESP_GATTC_CONNECT_EVT:{
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CONNECT_EVT conn_id %d, if %d", p_data->connect.conn_id, gattc_if);
        gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->connect.conn_id;
        memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATTC_TAG, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->connect.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }
        break;
    }
    case ESP_GATTC_OPEN_EVT:
        if (param->open.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "open failed, status %d", p_data->open.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "open success");
        break;
    case ESP_GATTC_DIS_SRVC_CMPL_EVT:
        if (param->dis_srvc_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "discover service failed, status %d", param->dis_srvc_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "discover service complete conn_id %d", param->dis_srvc_cmpl.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, NULL);
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: {


        printf("\n--------------------------\n");
        printf("\nsearch_res: ");
        printf("%X", p_data->search_res.srvc_id.uuid.uuid.uuid16);
        printf("\n...\n");
        for(int i =0 ; i<ESP_UUID_LEN_128;i++)
        printf("%X ",p_data->search_res.srvc_id.uuid.uuid.uuid128[i]);
        printf("\n");
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
                for(int i = 0; i < mycount; i++){
                printf("CHAR[%d] UUID : %2X\n", i, myresult[i].uuid.uuid.uuid16);
                printf("\n.....\n");
                    for(int j =0 ; j<ESP_UUID_LEN_128;j++)
                        printf("%X ",myresult[i].uuid.uuid.uuid128[j]);
                }
                printf("\n");
            }
        free(myresult);
        printf("\n--------------------------\n");

        ESP_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        if (memcmp(p_data->search_res.srvc_id.uuid.uuid.uuid128, remote_service_uuid, ESP_UUID_LEN_128) == 0) {
            ESP_LOGI(GATTC_TAG, "service found");
            get_server = true;
            gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
            gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
            ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
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
        if (get_server){
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                                     gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
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
                                                             gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                             gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                             remote_filter_char_uuid,
                                                             char_elem_result,
                                                             &count);
                    if (status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
                    }
                    ESP_LOGI(GATTC_TAG, "char_elem_result count = %d   ", count);
                    /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                    if (count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
                        gl_profile_tab[PROFILE_A_APP_ID].char_handle = char_elem_result[0].char_handle;
                        esp_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, char_elem_result[0].char_handle);
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
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "REG FOR NOTIFY failed: error status = %d", p_data->reg_for_notify.status);
        }else{
            uint16_t count = 0;
            uint16_t notify_en = 1;
            esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                         ESP_GATT_DB_DESCRIPTOR,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].char_handle,
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
                                                                         gl_profile_tab[PROFILE_A_APP_ID].conn_id,
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
                                                                     gl_profile_tab[PROFILE_A_APP_ID].conn_id,
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
    ESP_LOGE(GATTC_TAG, "in ESP_GATTC_NOTIFY_EVT");
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive value_len: %d", p_data->notify.value_len);
        if (p_data->notify.is_notify){
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive notify value:");
        }else{
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive indicate value:");
        }
        esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
        if(p_data->notify.value_len == 8 && p_data->notify.value[0] == 0x51 && p_data->notify.value[6] == 0xA5)
        {
            if(setAllData(p_data->notify.value))
                getFora = true;
            else
                printf("FORA data error\n");
            
        }
        printf("notify: ");
        for(int i = 0; i < p_data->notify.value_len; i++)
            printf("%02X ", p_data->notify.value[i]);
        printf("\n");
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write descr success ");
        uint8_t write_char_data[35];
        for (int i = 0; i < sizeof(write_char_data); ++i)
        {
            write_char_data[i] = i % 256;
        }
        esp_ble_gattc_write_char( gattc_if,
                                  gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                  gl_profile_tab[PROFILE_A_APP_ID].char_handle,
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
        ESP_LOGI(GATTC_TAG, "write char success ");
        break;
    case ESP_GATTC_DISCONNECT_EVT:
        connect = false;
        get_server = false;
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_DISCONNECT_EVT, reason = %d", p_data->disconnect.reason);
        printf("DISCONNECT!\n");
        uint32_t duration = 30;
        esp_ble_gap_start_scanning(duration);
        break;
    case ESP_GATTC_READ_CHAR_EVT:
        if (p_data->read.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "read char failed, error status = %x", p_data->read.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "read char success : %d", p_data->read.value_len);
        if(isActivating == 1)
        {
            ultron_activate(p_data->read.value);            
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
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        //the unit of the duration is second
        uint32_t duration = 30;
        esp_ble_gap_start_scanning(duration);
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
            // ESP_LOGI(GATTC_TAG, "\n");

            if (adv_name != NULL) {
                if (memcmp(scan_result->scan_rst.bda, remote_device_bda, 6) == 0) {
                    printf("find!\n");
                    ESP_LOGI(GATTC_TAG, "searched device %s\n", remote_device_name);
                    if (connect == false) {
                        connect = true;
                        ESP_LOGI(GATTC_TAG, "connect to the remote device.");
                        esp_ble_gap_stop_scanning();
                        esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
                    }
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
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "reg app failed, app_id %04x, status %d",
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

static bool lastState = 1;
static void sendActivateButtCmd(void *pvParameters)
{
    while(1)
    {
        bool buttState = gpio_get_level(BUTTON0);
        if(isActivating == 0 && buttState == 0 && lastState != buttState)
        { 
            printf("send activate butt cmd\n");
            ESP_LOGW(GATTC_TAG, "send activate butt cmd");
            send_command(activate_cmd[0], sizeof(activate_cmd[0]));
            isActivating = 1;
            myTick = myTime;
            request_read();
        }
        lastState = buttState;
        myTime++;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

uint8_t sendCmdByKeyboard( char** args, uint8_t numArgs ){
    if(numArgs == 1)
        return;
    uint8_t cmd[20] = {0};
    for(int i = 1; i < numArgs; i++)
    {
        cmd[i - 1] = (char)strtol(args[i], NULL, 16); 
    }
    send_command(cmd, numArgs - 1);
    return 0;
}

unsigned int countTick = 0;
static void countTime(void *pvParameters)
{
    while(1)
    {
        countTick++;
        if(countTick == INT32_MAX - 1)
            countTick = 0;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
}

unsigned int foraTime = 0;
uint16_t nowForaNum;
uint8_t sf( char** args, uint8_t numArgs ){ 
    printf("\nsend FORA TH cmds.\n");
    uint16_t index = 0; 
    int count = 0;
    uint8_t cmds[4][8] = {
        {0x51, 0x2B, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x1F},         //read data num
        {0x51, 0x25, 0, 0, 0x00, 0x00, 0xA3, 0x19},  //read time[index]
        {0x51, 0x26, 0, 0, 0x00, 0x00, 0xA3, 0x1A},  //read time[index]
        {0x51, 0x52, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x46}};        //clear all
    

    foraTime = countTick;
    send_command(cmds[0], 8);
    while(countTick < (foraTime + 1)); //wait 100ms
    nowForaNum = getDataNum();
    printf("num : %d\n", nowForaNum);
    foraTime = countTick;
    for(uint16_t i = 0; i < nowForaNum; i++)
    {
        uint8_t check;
        cmds[1][2] = i & 0xff;
        cmds[1][3] = i >> 8;
        check = getCheckSum(cmds[1]);
        cmds[1][7] = check;

        cmds[2][2] = i & 0xff;
        cmds[2][3] = i >> 8;
        check = getCheckSum(cmds[2]);
        cmds[2][7] = check;

        send_command(cmds[1], 8);
        send_command(cmds[2], 8);
    }
    send_command(cmds[3], 8);
    return 0;
}

uint8_t bp( char** args, uint8_t numArgs ){ 
    printf("\nsend FORA BP cmds.\n");
    uint16_t index = 0; 
    int count = 0;
    uint8_t cmds[4][8] = {
        {0x51, 0x2B, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x1F},         //read data num
        {0x51, 0x25, 0, 0, 0x00, 0x00, 0xA3, 0x19},  //read time[index]
        {0x51, 0x26, 0, 0, 0x00, 0x00, 0xA3, 0x1A},  //read time[index]
        {0x51, 0x52, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x46}};        //clear all
    send_command(cmds[0], 8);

    return 0;
}

uint8_t printFora( char** args, uint8_t numArgs ){ 
    printAllForaData();
    return 0;
}

void app_main(void)
{

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

    ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);
    }
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }


    gpio_set_pull_mode(BUTTON0, GPIO_PULLUP_ONLY);
    gpio_set_direction(BUTTON0, GPIO_MODE_INPUT);

    //xTaskCreate(&sendActivateButtCmd, "sendActivateButtCmd", 4096, NULL, 15, NULL);
    xTaskCreate(&countTime, "countTime", 4096, NULL, 15, NULL);
    init_cli();
    char sendCmd[] = "sc";
    cmd_register(sendCmd, sendCmdByKeyboard);
    char foraCmd[] = "ft";
    cmd_register(foraCmd, sf);
    char foraPrintCmd[] = "ftp";
    cmd_register(foraPrintCmd, printFora);
    char foraCmd2[] = "fb";
    cmd_register(foraCmd2, bp);

}

