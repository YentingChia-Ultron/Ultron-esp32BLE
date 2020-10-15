#include "stdint.h"
#include "stdbool.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "utility.h"
#include "esp_log.h"
#include <string.h>
#define ENABLE_UART0 1

#define MAX_UART_READ_HANDLER 5
#define MAX_UART_PORT 2

#define ENABLE_UART0 CONFIG_ENABLE_UART0
#if ENABLE_UART0
    #define UART0_PORT UART_NUM_0
    #define UART0_BAUD CONFIG_UART0_BAUD
    #define UART0_IS_HALF_DUPLEX CONFIG_UART0_IS_HALF_DUPLEX
    #define UART0_TX CONFIG_UART0_TX
    #define UART0_RX CONFIG_UART0_RX
    #define UART0_RTS CONFIG_UART0_RTS
    #define UART0_BUF CONFIG_UART0_BUF 
#endif

#define ENABLE_UART1 CONFIG_ENABLE_UART1
#if ENABLE_UART1
    #define UART1_PORT UART_NUM_1
    #define UART1_BAUD CONFIG_UART1_BAUD
    #define UART1_IS_HALF_DUPLEX CONFIG_UART1_IS_HALF_DUPLEX
    #define UART1_TX CONFIG_UART1_TX
    #define UART1_RX CONFIG_UART1_RX
    #define UART1_RTS CONFIG_UART1_RTS
    #define UART1_BUF CONFIG_UART1_BUF
#endif

static const char* TAG = "utility";

static UartReadBytesHandler handler_list[MAX_UART_PORT][MAX_UART_READ_HANDLER] = {NULL};
static uint8_t handler_num[MAX_UART_PORT] = {0};
static TaskHandle_t read_uart_data_task_handler = NULL;

/* uart  uart  uart  uart  uart  uart */
static bool IS_INIT = false;
static bool IS_UART_INIT[UART_NUM_MAX] = {false};
static uint8_t ack_cmd[6];
// static SemaphoreHandle_t uart_write_mutex = NULL;

static void read_uart_data_task(void *args){
    int buf_size = *(int *)args;
    ESP_LOGI(TAG, "start read_uart_data_task");
    uint8_t *data = (uint8_t *) malloc(buf_size);
    int len = 0;
    while(1){
        for(uint8_t i = 0; i < MAX_UART_PORT; i++){
            if(IS_UART_INIT[i]){
                len = uart_read_bytes(i, data, buf_size, 10 / portTICK_RATE_MS);
                if (len > 0) {
                    if(len == 6) {
                        memcpy(ack_cmd, data, len);
                    }
                    for(uint8_t j = 0; j < handler_num[i]; j++){
                        handler_list[i][j](data, &len);
                    }
                }
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

esp_err_t uart_task_init() {
    if(IS_INIT) return ESP_FAIL;
    IS_INIT = true;
    int *buf_size = malloc(sizeof(int));
    *buf_size = 0;
    // taskCreateWithCheck(read_uart_data_task, "read_uart_data_task", 1024*2, buf_size, 6, &read_uart_data_task_handler);

    if (IS_UART_INIT[0]) {
        ESP_LOGE(TAG, "Already init uart %d", 0);
        return ESP_FAIL;
    }
    uart_config_t uart0_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(0, &uart0_config);

    ESP_ERROR_CHECK(uart_driver_install(0, 256 * 2, 0, 0, NULL, 0));
    uart_set_pin(0, 1, 3, 22, UART_PIN_NO_CHANGE);

    IS_UART_INIT[0] = true;
    *buf_size = 256;


    // uart_write_mutex = xSemaphoreCreateMutex();
    if (read_uart_data_task_handler == NULL) {
        taskCreateWithCheck(read_uart_data_task, "read_uart_data_task", 1024*3, buf_size, 6, &read_uart_data_task_handler);
    }

    return ESP_OK;
}

esp_err_t uart_write_bytes_with_retry(uart_port_t uart_num, uint8_t *data, uint8_t len){
    if (!IS_UART_INIT[uart_num]) {
        return ESP_ERR_NOT_FOUND;
    }
    // xSemaphoreTake(uart_write_mutex, portMAX_DELAY);
    int uard_write_result = uart_write_bytes(uart_num, (const char *)data, len);
    // xSemaphoreGive(uart_write_mutex);
    uint8_t retry = 0;
    while (uard_write_result < 0 && retry < 5) {
        retry++;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        ESP_LOGW(TAG, "retry write uart");
        // xSemaphoreTake(uart_write_mutex, portMAX_DELAY);
        uard_write_result = uart_write_bytes(uart_num, (const char *)data, len);
        // xSemaphoreGive(uart_write_mutex);
    }
    ESP_LOGW(TAG, "success write uart!!!!!!!!");
    return ESP_OK;
}

esp_err_t uart_write_bytes_with_ack(uart_port_t uart_num, uint8_t *data, uint8_t len){
    if (!IS_UART_INIT[uart_num]) {
        return ESP_ERR_NOT_FOUND;
    }
    // xSemaphoreTake(uart_write_mutex, portMAX_DELAY);
    int uard_write_result = uart_write_bytes(uart_num, (const char *)data, len);
    // xSemaphoreGive(uart_write_mutex);
    uint8_t retry = 0;
    while ((uard_write_result < 0 || memcmp(ack_cmd, data, len) != 0) && retry < 5) {
        retry++;
        ESP_LOGW(TAG, "retry write uart");
        // xSemaphoreTake(uart_write_mutex, portMAX_DELAY);
        uard_write_result = uart_write_bytes(uart_num, (const char *)data, len);
        // xSemaphoreGive(uart_write_mutex);
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
    memset(ack_cmd, 0x00, 6);
    ESP_LOGW(TAG, "success write uart!!!!!!!!");
    return ESP_OK;
}

int uart_get_buffer_size(uart_port_t uart_num) {
    if (!IS_UART_INIT[uart_num]) {
        return -1;
    }
#if ENABLE_UART0
    if (uart_num == 0){
        return UART0_BUF;
    }
#endif
#if ENABLE_UART1
    if (uart_num == 1){
        return UART1_BUF;
    }
#endif
    return -1;
}

/**
 * @brief create task, and if fail to create task twice, restart ESP
 * 
 * @param pxTaskCode 
 * @param pcName 
 * @param usStackDepth 
 * @param pvParameters 
 * @param uxPriority 
 * @param pxCreatedTask 
 * @return BaseType_t pdPASS if task was successfully created
 */
BaseType_t taskCreateWithCheck( TaskFunction_t pxTaskCode, const char * const pcName, const uint16_t usStackDepth,
                                void * const pvParameters, UBaseType_t uxPriority, TaskHandle_t * const pxCreatedTask)
{
    BaseType_t result = xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask);
    if (result != pdPASS) {
        /* 記錄錯誤的log */
        ESP_LOGE(TAG, "Create task failed: %s", pcName);
        result = xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask);
        if (result != pdPASS) {
            ESP_LOGE(TAG, "Create task failed 2 times: %s", pcName);
            /* 重試一次還是失敗，就重開機 */
            esp_restart();
        }
    }
    ESP_LOGI(TAG, "Create task success: %s", pcName);
    return result;
}

esp_err_t set_uart_read_handler(uart_port_t uart_num, UartReadBytesHandler handler) {
    if (!IS_UART_INIT[uart_num] || handler == NULL) {
        return ESP_FAIL;
    }

    if (handler_num[uart_num] < MAX_UART_READ_HANDLER) {
        handler_list[uart_num][handler_num[uart_num]] = handler;
    }
    handler_num[uart_num]++;
    return ESP_OK;
}

#if defined(CONFIG_ULTRON_ESP8266) || defined(CONFIG_ULTRON_ESP8285)
#include "esp_system.h"
static bool cpuTurbo = false;

void enter_cpu_turbo_mode() {
    if (!cpuTurbo) {
        rtc_clk_cpu_freq_set(RTC_CPU_FREQ_160M);
        ESP_LOGE(TAG, "!!!!!!!!!!!!! SET CPU TO 160 MHz ");
        cpuTurbo = true;
    }
}

void exit_cpu_turbo_mode() {
    if (cpuTurbo) {
        cpuTurbo = false;
        rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M);
        ESP_LOGE(TAG, "!!!!!!!!!!!!! SET CPU TO 80 MHz ");
    }
}
#endif