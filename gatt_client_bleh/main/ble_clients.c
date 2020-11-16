#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "nvs.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ble_clients.h"


#define GATTC_TAG "GATTC_DEMO_BLEH"
#define INVALID_HANDLE   0

static esp_gattc_char_elem_t *char_elem_result   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result = NULL;
bool scan_forever = false;
ProfileNodeT *head_profile = NULL;
uint8_t connect_info_num = 0;
ConnectInfoT *connect_infos = NULL;

/* Declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

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

void setConnectInfo(ConnectInfoT* infos, uint8_t info_num)
{
    connect_infos = calloc(info_num, sizeof(ConnectInfoT));
    for(uint8_t i = 0; i < info_num; i++)
        connect_infos[i] = infos[i];
    connect_info_num = info_num;
}

void addProfile(uint8_t dev_id, UuidsT *myUUIDs, uint8_t service_num)
{
    BleProfileT profile;
    if(service_num > MAX_SERVICE_NUM)
    {
        ESP_LOGE(GATTC_TAG, "service_num > MAX_SERVICE_NUM");
        profile.service_num = 0;
        return;
    }
    profile.service_num = service_num;
    profile.services = calloc(service_num, sizeof(BleServiceT));
    if(profile.services == NULL)
    {
        ESP_LOGE(GATTC_TAG, "services calloc fail");
        return;
    }
    for(uint8_t i = 0; i < service_num; i++)
    {
        profile.services[i].service_uuid = myUUIDs[i].service_uuid;
        if(myUUIDs[i].char_num > MAX_CHAR_NUM)
        {
            ESP_LOGE(GATTC_TAG, "char_num[%d] > MAX_CHAR_NUM", i);
            profile.services[i].char_num = 0;
            continue;
        }
        profile.services[i].char_num = myUUIDs[i].char_num;
        profile.services[i].chars = calloc(myUUIDs[i].char_num, sizeof(BleCharT));
        if(profile.services[i].chars == NULL)
        {
            ESP_LOGE(GATTC_TAG, "chars calloc fail");
            return;
        }
        for(uint8_t j = 0; j < myUUIDs[i].char_num; j++)
            profile.services[i].chars[j].char_uuid = myUUIDs[i].char_uuid[j];
    }
    profile.gattc_cb = gattc_profile_event_handler;
    profile.gattc_if = ESP_GATT_IF_NONE;
    memcpy(profile.remote_bda, connect_infos[dev_id].bda, 6);

    ProfileNodeT *new_node = (ProfileNodeT*)calloc(1, sizeof(ProfileNodeT));
    if(new_node == NULL)
    {
        ESP_LOGE(GATTC_TAG, "new_node calloc fail");
            return;
    }
    new_node->profile = profile;
    new_node->next = NULL;
    
    if(head_profile == NULL) {
        new_node->profile_id = 0;
        head_profile = new_node;
    }
    else {
        ProfileNodeT *current;    
        current = head_profile;
        uint8_t id = 1;
        while(current->next != NULL) {
            current = current->next;
            id++;
        }
        new_node->profile_id = id;
        current->next = new_node;
    }
    esp_err_t ret = esp_ble_gattc_app_register(new_node->profile_id);
    if (ret)
        ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);
    new_node->dev_id = dev_id;
}

void deleteProfile(uint8_t profile_id)
{
    ProfileNodeT *current = head_profile;
    ProfileNodeT *temp = NULL;

    if(profile_id == 0) {
        temp = head_profile;
        head_profile = current->next;
        current = current->next;
    }
    else{
        while(current->next != NULL) {
            if(current->next->profile_id == profile_id) {
                temp = current->next;
                current->next = current->next->next;
                current = current->next;
                break;
            }
            current = current->next;
        }
    }
    while(current != NULL)
    {
        current->profile_id--;
        current = current->next;
    }
    if(temp != NULL)
    {
        esp_ble_gattc_app_unregister(temp->profile.gattc_if);
        for(uint8_t i = 0; i < temp->profile.service_num; i++){
            free(temp->profile.services[i].chars);
            temp->profile.services[i].chars = NULL;
        }
        free(temp->profile.services);
        temp->profile.services = NULL;
        free(temp);
        temp = NULL;
        printf("delete sucess\n");
    }
}

ProfileNodeT *findProfile(uint8_t profile_id)
{
    ProfileNodeT *new_node = head_profile;
    while(new_node != NULL) 
    {
        if(new_node->profile_id == profile_id)
            return new_node;
        else
            new_node = new_node->next;
    }
    return NULL;
}

void sendCommand(uint8_t profile_id, uint8_t s_id, uint8_t ch_id, const uint8_t *cmd, int len) {
    ProfileNodeT *temp = findProfile(profile_id);
    if(temp == NULL)
    {
        ESP_LOGE(GATTC_TAG, "profile_id error");
        return;
    }
    temp->profile.services[s_id].chars[ch_id].have_data = false;
    printf("\nsend cmd : ");
    for(int i = 0 ; i < len ; i++) {
        printf("%02X ", cmd[i]);
    }
    printf("\n");
    esp_ble_gattc_write_char( temp->profile.gattc_if,
                              temp->profile.conn_id,
                              temp->profile.services[s_id].chars[ch_id].char_handle,
                              len,
                              cmd,
                              ESP_GATT_WRITE_TYPE_RSP,
                              ESP_GATT_AUTH_REQ_NONE);
}

void requestRead(uint8_t profile_id, uint8_t s_id, uint8_t ch_id) {
    ProfileNodeT *temp = findProfile(profile_id);
    if(temp == NULL)
    {
        ESP_LOGE(GATTC_TAG, "profile_id error");
        return;
    }
    esp_ble_gattc_read_char( temp->profile.gattc_if,
                              temp->profile.conn_id,
                              temp->profile.services[s_id].chars[ch_id].char_handle,
                              ESP_GATT_AUTH_REQ_NONE);
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
    case ESP_GATTC_CONNECT_EVT:
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CONNECT_EVT");
        break;
    case ESP_GATTC_OPEN_EVT:
        if (param->open.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "open failed, status %02X", p_data->open.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "open success");
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_OPEN_EVT conn_id %d, if %d", p_data->open.conn_id, gattc_if);
        ProfileNodeT *temp = head_profile;
        while(temp != NULL)
        {
            if(temp->profile.gattc_if == gattc_if)
                break;
            temp = temp->next;
        }
        memcpy(temp->profile.remote_bda, p_data->open.remote_bda, 6);
        temp->profile.conn_id = p_data->open.conn_id;

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
        ESP_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        ProfileNodeT *temp = head_profile;
        while(temp != NULL)
        {
            if(temp->profile.gattc_if == gattc_if)
                break;
            temp = temp->next;
        }
        for(uint8_t i = 0; i < temp->profile.service_num; i++)
        {
            bool get_server = false;
            if (temp->profile.services[i].service_uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == temp->profile.services[i].service_uuid.uuid.uuid16)
                get_server = true;
            if (temp->profile.services[i].service_uuid.len == ESP_UUID_LEN_32 && p_data->search_res.srvc_id.uuid.uuid.uuid32 == temp->profile.services[i].service_uuid.uuid.uuid32)
                get_server = true;
            else if (temp->profile.services[i].service_uuid.len == ESP_UUID_LEN_128 && memcmp(p_data->search_res.srvc_id.uuid.uuid.uuid128, temp->profile.services[i].service_uuid.uuid.uuid128, ESP_UUID_LEN_128) == 0)
                get_server = true;
            if (get_server) 
            {
                ESP_LOGI(GATTC_TAG, "service found");
                temp->profile.services[i].service_start_handle = p_data->search_res.start_handle;
                temp->profile.services[i].service_end_handle = p_data->search_res.end_handle;
                if(temp->profile.services[i].service_uuid.len == ESP_UUID_LEN_16)
                    ESP_LOGI(GATTC_TAG, "UUID16: %04X", p_data->search_res.srvc_id.uuid.uuid.uuid16);
                else if(temp->profile.services[i].service_uuid.len == ESP_UUID_LEN_32)
                    ESP_LOGI(GATTC_TAG, "UUID32: %04X", p_data->search_res.srvc_id.uuid.uuid.uuid32);
                else
                    esp_log_buffer_hex(GATTC_TAG, p_data->search_res.srvc_id.uuid.uuid.uuid128, temp->profile.services[i].service_uuid.len);

                //find char
                for(uint8_t j = 0; j < MAX_CHAR_NUM; j++)
                {
                    uint16_t count = 0;
                    esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                            p_data->search_res.conn_id,
                                                                            ESP_GATT_DB_CHARACTERISTIC,
                                                                            temp->profile.services[i].service_start_handle,
                                                                            temp->profile.services[i].service_end_handle,
                                                                            INVALID_HANDLE,
                                                                            &count);
                    if (status != ESP_GATT_OK)
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
                    if (count > 0){
                        char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                        if (!char_elem_result)
                            ESP_LOGE(GATTC_TAG, "gattc no mem");
                        else
                        {
                            status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                                    p_data->search_res.conn_id,
                                                                    temp->profile.services[i].service_start_handle,
                                                                    temp->profile.services[i].service_end_handle,
                                                                    temp->profile.services[i].chars[j].char_uuid,
                                                                    char_elem_result,
                                                                    &count);
                            if (status != ESP_GATT_OK)
                                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error : %d", status);
                            ESP_LOGI(GATTC_TAG, "char_elem_result count = %d   ", count);
                            /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                            if (count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
                                temp->profile.services[i].chars[j].char_handle = char_elem_result[0].char_handle;
                                esp_ble_gattc_register_for_notify(gattc_if, temp->profile.remote_bda, char_elem_result[0].char_handle);
                            }
                        }
                        /* free char_elem_result */
                        free(char_elem_result);
                        char_elem_result = NULL;
                    }
                    else
                        ESP_LOGE(GATTC_TAG, "no char found");
                }
            }
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
         break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
        ProfileNodeT *temp = head_profile;
        while(temp != NULL)
        {
            if(temp->profile.gattc_if == gattc_if)
                break;
            temp = temp->next;
        }
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "REG FOR NOTIFY failed: error status = %d", p_data->reg_for_notify.status);
        }else{
            uint16_t notify_en = 1;
            uint8_t count = 0;
            descr_elem_result = malloc(sizeof(esp_gattc_descr_elem_t) * MAX_CHAR_NUM);
            if (!descr_elem_result){
                ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
            }else{
                esp_gatt_status_t ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                     temp->profile.conn_id,
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
                                                                 temp->profile.conn_id,
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
                descr_elem_result = NULL;
            }
        }
        break;
    }
    case ESP_GATTC_NOTIFY_EVT:
        ESP_LOGE(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT");
        esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
        temp = head_profile;
        while(temp != NULL)
        {
            if(temp->profile.gattc_if == gattc_if)
                break;
            temp = temp->next;
        }
        bool find = false;
        for(int i = 0; i < temp->profile.service_num; i++)
        {
            for(int j = 0; j < MAX_CHAR_NUM; j++)
            {
                if(p_data->notify.handle == temp->profile.services[i].chars[j].char_handle)
                {
                    temp->profile.services[i].chars[j].notify_len = p_data->notify.value_len;
                    memcpy(temp->profile.services[i].chars[j].notify_value, p_data->notify.value, temp->profile.services[i].chars[j].notify_len);
                    temp->profile.services[i].chars[j].have_data = true;
                }
            }
            if(find)
                break;
        }
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        temp = head_profile;
        while(temp != NULL)
        {
            if(temp->profile.gattc_if == gattc_if)
                break;
            temp = temp->next;
        }
        if(connect_infos[temp->dev_id].status != 2){
            connect_infos[temp->dev_id].status = 2;
            startScan(-1);
        }
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
        temp = head_profile;
        while(temp != NULL)
        {
            if(temp->profile.gattc_if == gattc_if)
                break;
            temp = temp->next;
        }
        if(memcmp(p_data->disconnect.remote_bda, temp->profile.remote_bda, 6) == 0)
        {
            if(connect_infos[temp->dev_id].status == 2)
                connect_infos[temp->dev_id].status = 0;
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_DISCONNECT_EVT, profile_id = %d, reason = %d", temp->profile_id, p_data->disconnect.reason);
            printf("DISCONNECT!\n");
            startScan(-1); 
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
    ProfileNodeT *temp = NULL;
    switch (event) {
    case ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT:
        for(uint8_t i = 0; i < connect_info_num; i++)
        {
            if(memcmp(param->read_rssi_cmpl.remote_addr, connect_infos[i].bda, 6) == 0)
            {
                connect_infos[i].rssi = param->read_rssi_cmpl.rssi;
                break;
            }
        }
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
            for(uint8_t i = 0; i < connect_info_num; i++)
            {
                if (connect_infos[i].status == 0 && memcmp(scan_result->scan_rst.bda, connect_infos[i].bda, 6) == 0)
                {
                    connect_infos[i].status = 1;
                    connect_infos[i].rssi = scan_result->scan_rst.rssi;
                    connect_infos[i].adv_data_len = scan_result->scan_rst.adv_data_len + scan_result->scan_rst.scan_rsp_len;
                    if(connect_infos[i].adv_data_len > 64)
                        ESP_LOGE(GATTC_TAG, "adv_data_len over 64 : %d", connect_infos[i].adv_data_len);
                    memcpy(connect_infos[i].adv_data, scan_result->scan_rst.ble_adv, connect_infos[i].adv_data_len);
                    // esp_log_buffer_hex(GATTC_TAG, connect_infos[i].adv_data, connect_infos[i].adv_data_len);
                }
            }
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            printf("ESP_GAP_SEARCH_INQ_CMPL_EVT\n");
            if(scan_forever)
                startScan(-1);
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

            ProfileNodeT *temp = findProfile(param->reg.app_id);
            if(temp != NULL)
            {
                temp->profile.gattc_if = gattc_if;
                temp->profile.app_id = param->reg.app_id;
                temp = NULL;
            }
            else
                ESP_LOGE(GATTC_TAG, "app_id error : %04x", param->reg.app_id);

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
        ProfileNodeT *temp = head_profile;
        while(temp != NULL)
        {
            if (gattc_if == ESP_GATT_IF_NONE || gattc_if == temp->profile.gattc_if) {
                if (temp->profile.gattc_cb) {
                    temp->profile.gattc_cb(event, gattc_if, param);
                }
            }
            temp = temp->next;
        }
    } while (0);

}

void startScan(int duration)
{
    if(duration == -1){
        scan_forever = true;
        duration = 600;
    }
    esp_ble_gap_start_scanning(duration);
}
void stopScan()
{
    esp_ble_gap_stop_scanning();
}

uint8_t getBleStatus(uint8_t dev_id)
{
    if(connect_infos != NULL)
        return connect_infos[dev_id].status;
    else
        return 0;
}
uint8_t getAdvLen(uint8_t dev_id)
{
    return connect_infos[dev_id].adv_data_len;
}

int getRssi(uint8_t dev_id)
{
    if(connect_infos[dev_id].status == 2)
    {
        int last_value = connect_infos[dev_id].rssi;
        uint8_t i = 0;
        esp_ble_gap_read_rssi(connect_infos[dev_id].bda);
        while(last_value == connect_infos[dev_id].rssi && i < 20)
        {
            i++;
            vTaskDelay(10 / portTICK_RATE_MS);
        }
    }
    return connect_infos[dev_id].rssi;
}

void getAdvData(uint8_t dev_id, uint8_t *buff)
{
    memcpy(buff, connect_infos[dev_id].adv_data, connect_infos[dev_id].adv_data_len);
}

void disconnectBle(uint8_t profile_id) {
    ProfileNodeT *temp = findProfile(profile_id);
    if(temp == NULL)
    {
        ESP_LOGE(GATTC_TAG, "profile_id error");
        return;
    }
    if(connect_infos[temp->dev_id].status == 2) {
        ESP_LOGE(GATTC_TAG, "disconnect BLE");
        esp_ble_gap_disconnect(temp->profile.remote_bda);
    }
}

bool getDataStatus(uint8_t profile_id, uint8_t s_id, uint8_t ch_id) {
    ProfileNodeT *temp = findProfile(profile_id);
    if(temp == NULL)
    {
        ESP_LOGE(GATTC_TAG, "profile_id error");
        return;
    }
    return temp->profile.services[s_id].chars[ch_id].have_data;
}

uint8_t getNotifyLen(uint8_t profile_id, uint8_t s_id, uint8_t ch_id) 
{
    ProfileNodeT *temp = findProfile(profile_id);
    if(temp == NULL)
    {
        ESP_LOGE(GATTC_TAG, "profile_id error");
        return;
    }
    return temp->profile.services[s_id].chars[ch_id].notify_len;
}

void getNotifyVlaue(uint8_t profile_id, uint8_t s_id, uint8_t ch_id, uint8_t *target) {
    ProfileNodeT *temp = findProfile(profile_id);
    if(temp == NULL)
    {
        ESP_LOGE(GATTC_TAG, "profile_id error");
        return;
    }
    memcpy(target, temp->profile.services[s_id].chars[ch_id].notify_value, temp->profile.services[s_id].chars[ch_id].notify_len);
    temp->profile.services[s_id].chars[ch_id].have_data = false;
}

void initBle()
{
    printf("initBle\n");
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
}

void openProfile(uint8_t profile_id)
{
    stopScan();
    vTaskDelay(pdMS_TO_TICKS(50));

    ProfileNodeT *temp = findProfile(profile_id);
    if(temp != NULL) 
        esp_ble_gattc_open(temp->profile.gattc_if, temp->profile.remote_bda, 0, true);  
    else
        ESP_LOGE(GATTC_TAG, "openProfile error");
}