/* Dennis */

//#ifdef CONFIG_CLI

#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include "freertos/semphr.h"

#include <string.h>
#include "utility.h"
#include "cli.h"

static char inputBuffer[INPUT_BUFFER_LENGTH]       =   { 0 };
static char lazyBuffer[INPUT_BUFFER_LENGTH]        =   { 0 };

static const char* prompt       =   "\r\n$ CLI >";
static const char* b            =   "\b \b";
static const char* theHelp      =   "\r\nThis is help.\r\n";

static uint8_t currentIdx           =   0;
static SemaphoreHandle_t semaCli    =   NULL;

command help = {
    .next       =   NULL,
    .keyword    =   "help",
    .func       =   to_help
};

uint8_t to_help( char** args, uint8_t numArgs ){
    uart_write_bytes(UART_CHOOSE, theHelp, strlen(theHelp));
    return 0;
}

//- command line interface
void init_cli( void ){
    uart_task_init();
    if( !semaCli ) semaCli = xSemaphoreCreateBinary();
    set_uart_read_handler(UART_CHOOSE, cli_handler);

    //- registration

    //- registration ^

    taskCreateWithCheck( (TaskFunction_t)&cli_task, "cli_task", 1024 * 8, NULL, 10, NULL );
}

/* args : pointers to each arg
 * 
 * inputBuffer : user input string
 * 
 * return - counts for args 
*/
uint8_t get_args( char** args, char* inputBuffer ){
    static uint8_t idxArgs = 0;
    uint8_t numArgs = idxArgs;
    char* idx = inputBuffer;

    if( !*idx && !(idxArgs = 0) ) 
        return numArgs;
    *(args + idxArgs++) = idx;
    if( idxArgs >= MAX_NUM_ARGS && !(idxArgs = 0) ) 
        return numArgs;

    while( *idx ){
        if( *idx == ' ' && !(*idx++ = '\0') ) break; else idx++;
    }
    return get_args(args, idx);
}

void cmd_register( char* cmd_keyword, uint8_t (*func)(char**, uint8_t) ){
    uint8_t i = 0;
    command* idxCmd = &help;
    command* newCmd = (command*) malloc(sizeof(command));
    newCmd->next = NULL;
    newCmd->func = func;

    while( (*(newCmd->keyword + i) = *(cmd_keyword + i)) && ++i <= MAX_CMD_KEYWORD );
    while( idxCmd->next && (idxCmd = idxCmd->next) );
    idxCmd->next = newCmd;
}

void cli_task(void){
    for(;;){
        if( pdTRUE == xSemaphoreTake(semaCli, 100) ){
            char* args[MAX_NUM_ARGS] = { NULL };
            uint8_t numArgs = get_args(args, inputBuffer);

            for( command* idxCmd = &help; idxCmd; idxCmd = idxCmd->next ){
                if( !strcmp(args[0], idxCmd->keyword) ){
                    idxCmd->func(args,numArgs);
                    break;
                }
            }
            *inputBuffer = '\0';
        }
    }
}

int cli_handler(uint8_t *data, int *data_len){
    uint8_t i = 0;
    uint32_t count = INPUT_BUFFER_LENGTH;

    //- Enter
    if(*data == 0x0d){                                    
        if(!currentIdx){
            while( count-- &&  (*(inputBuffer + i) = *(lazyBuffer +i)) ) i++;
        }
        else{
            inputBuffer[currentIdx] = '\0';
            while( count-- &&  (*(lazyBuffer + i) = *(inputBuffer +i)) ) i++;
        }
        currentIdx = 0;
        xSemaphoreGive(semaCli);
        return uart_write_bytes(UART_CHOOSE, prompt, strlen(prompt));
    }

    //- BackSpace
    if((currentIdx && *data == 0x7f) || *data == 0x08) {    
        inputBuffer[--currentIdx] = '\0';
        return uart_write_bytes(UART_CHOOSE, (const char *)b, strlen(b));
    }

    if(*data >= ' ' && *data < 0x7f){                     
        uint8_t n = 0;
        while(currentIdx < INPUT_BUFFER_LENGTH && n < *data_len){
            inputBuffer[currentIdx++] = *data;
            n++;
        }
        return uart_write_bytes(UART_CHOOSE, (const char *)(inputBuffer + currentIdx - 1), n);
    }
    return true;
}

//#endif