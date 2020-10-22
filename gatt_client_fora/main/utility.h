#ifndef __UTILITY_H__
#define __UTILITY_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

typedef int (*UartReadBytesHandler)(uint8_t *data, int *len);

/* uart write會發生錯誤，所以需要retry，建立一個共用的，有錯誤可以從這邊寫log */
esp_err_t uart_task_init();
esp_err_t uart_write_bytes_with_retry(uart_port_t uart_num, uint8_t *data, uint8_t len);
esp_err_t uart_write_bytes_with_ack(uart_port_t uart_num, uint8_t *data, uint8_t len);
int uart_get_buffer_size(uart_port_t uart_num);
/* create task，可能會allocate失敗，或是其他原因失敗，有錯誤統一在此紀錄 */
BaseType_t taskCreateWithCheck( TaskFunction_t pxTaskCode,
                                const char * const pcName,      /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
                                const uint16_t usStackDepth,
                                void * const pvParameters,
                                UBaseType_t uxPriority,
                                TaskHandle_t * const pxCreatedTask);

esp_err_t set_uart_read_handler(uart_port_t uart_num, UartReadBytesHandler handler);


#if defined(CONFIG_ULTRON_ESP8266) || defined(CONFIG_ULTRON_ESP8285)
void enter_cpu_turbo_mode();
void exit_cpu_turbo_mode();
#endif // defined(CONFIG_ULTRON_ESP8266) || defined(CONFIG_ULTRON_ESP8285)


#endif // __UTILITY_H__