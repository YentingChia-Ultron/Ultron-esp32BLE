/* Dennis */

#ifndef CLI_H_
#define CLI_H_

#define UART_CHOOSE                 UART_NUM_0
#define MODBUS_UART_PORT            UART_NUM_1

#define INPUT_BUFFER_LENGTH         100
#define OUTPUT_BUFFER_LENGTH        100
#define MAX_NUM_ARGS                10
#define MAX_CMD_KEYWORD             20

typedef struct command_struct{
    struct command_struct* next;
    uint8_t (*func)(char**, uint8_t);
    char keyword[MAX_CMD_KEYWORD];
} command;

int     cli_handler(uint8_t *data, int *data_len);
void    init_cli(void);
void    cli_task(void);
void    cmd_register(char* cmd_keyword, uint8_t(*func)(char**, uint8_t));
uint8_t get_args(char** args, char* inputBuffer);

uint8_t to_help( char** args, uint8_t num_args );

#endif /* CLI_H_ */
