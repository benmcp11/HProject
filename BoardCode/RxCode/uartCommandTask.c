/* Standard C Libraries */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* XDCtools Header files */
#include <xdc/std.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>

/* Board Header files */
#include "Board.h"
#include <ti/drivers/Board.h>

/* Drivers */
#include <ti/drivers/UART.h>
#include <ti/drivers/NVS.h>

/* Application Header Files */
#include "uartCommandTask.h"

/* EasyLink API Header files */
#include "easylink/EasyLink.h"

/* NVS Structure */
#include "rxDataStructs.h"

/* Defines */
#define UART_TASK_STACK_SIZE    2048
#define UART_TASK_PRIORITY      3

#define INPUT_BUFFER_SIZE       32

/* Task Object to be constructed */
Task_Struct uartCommandTask;

/* Task's stack */
uint8_t uartTaskStack[UART_TASK_STACK_SIZE];

/* Define nvs */
NVS_Handle nvsRegion;
uint_fast16_t status;
rxFrequencies freqInfo;

/* Setup UART */
UART_Handle uart;

/* UART Semaphore */
Semaphore_Handle UART_sem;

char input_buffer[INPUT_BUFFER_SIZE];

/* Parse user input */
int getUARTInput(UART_Handle uart, char *buffer, size_t size) {

    char input_char;
    
    /* Take in a command */
    memset(buffer, 0, size);
    
    char message[] = ">:";
    UART_write(uart, &message, sizeof message);

    int i;
    for (i = 0; i < INPUT_BUFFER_SIZE; i++) {
        /* Read the input 1 char at a time, displaying the input back to the user */
        UART_read(uart, &input_char, 1);
        UART_write(uart, &input_char, 1);

        if (input_char == '\r') {
            /* Enter Key */
            input_char = '\n';
            UART_write(uart, &input_char, 1);
            buffer[i] = '\0';
            return 1;
        }
        buffer[i] = input_char;
    }

    /* If we get this far, the input buffer has run out so return -1 so indicate failure */
    return -1;
}

static void commandInterface() {

    char message[] = "\n\n\rUART Command Interface Started\n\r"
                    "--------------------------------\n\rPlease type commands as seen, and do not use backspaces.\n\r\n"
                    "Please reset the board after all changes have been implemented.\n\r--------------------------------\n\r\n"
                    "Available Commands:\n\r"
                    "- info \n\r- help \n\r- setnum \n\r- resetfreq \n\r- setfreq \n\r- setdebug \n\r- change\n\r- setdis \n\r- exit\n\r\n";
    UART_write(uart, message, sizeof message);

    /* Enter main command loop */
    while (1) {
        /* Enter main loop */

        /* Get user command */
        if (getUARTInput(uart, input_buffer, sizeof input_buffer) == -1) {
            /* Input failed due to buffer running out of space */
            char message[] = "Input buffer ran out.\r\n";
            UART_write(uart, &message, sizeof message);
            continue;
        }
        
        /* Command is good, read it and find out what to do next */
        if (strcmp(input_buffer, "exit") == 0) {
            /* Exit the command interface */
            char message[] = "Exiting command interface\n\r--------------------\n\n\r";
            UART_write(uart, &message, sizeof message);
            return;
        } 
        else if (strcmp(input_buffer, "help") == 0) {
            /* Exit the command interface */
            char message[] =    "\n\rHelp\n\r"
                                "------\n\r"
                                "- info: Displays the receiver's settings\n\r"
                                "- exit: Exits command interface and returns to sending packet data\n\r"
                                "- setnum: Set the number of frequencies the receiver should cycle through\n\r"
                                "- resetfreq: Reset the receiver's frequency list to defaults\n\r"
                                "- setfreq: Set one of the frequencies in the list to a custom value\n\r"
                                "- setdebug: Turn the Debugging LED's ON or OFF\n\r";
            UART_write(uart, &message, sizeof message);
        } 
        else if (strcmp(input_buffer, "setfreq") == 0) {
            /* Set board frequency */
            char message[100];
            memset(message, 0, sizeof message);
            snprintf(message, sizeof message, "Board is currently using %d frequencies.\n\r"
                                                "Enter the frequency index you would like to edit:\n\r", freqInfo.noFreqUsed);
            UART_write(uart, &message, sizeof message);

            if (getUARTInput(uart, input_buffer, sizeof input_buffer) == -1) {
                /* Input failed due to buffer running out of space */
                char message[] = "Input buffer ran out.\n\r\n";
                UART_write(uart, &message, sizeof message);
                continue;
            }
            
            long n;
            n = strtol(input_buffer, NULL, 10);

            if (n < 1 || n > MAX_N_FREQUENCIES) {
                char message[] = "Invalid number/input.\n\r\n";
                UART_write(uart, &message, sizeof message);
                continue;
            }

            char message2[] = "Enter desired frequency:\n\r";
            UART_write(uart, &message2, sizeof message2);

            if (getUARTInput(uart, input_buffer, sizeof input_buffer) == -1) {
                /* Input failed due to buffer running out of space */
                char message[] = "Input buffer ran out.\n\r\n";
                UART_write(uart, &message, sizeof message);
                continue;
            }
            
            long freq;
            freq = strtol(input_buffer, NULL, 10);

            if (freq < FREQ_MIN || freq > FREQ_MAX) {
                char message[] = "Invalid number/input.\n\r\n";
                UART_write(uart, &message, sizeof message);
                continue;
            }
            
            freqInfo.freqList[n-1] = freq;
            NVS_write(nvsRegion, 0, &freqInfo, sizeof freqInfo, NVS_WRITE_ERASE);
        }
        else if (strcmp(input_buffer, "resetfreq") == 0) {
            /* Reset the board's frequency list to defaults */
            char message[] = "RX frequecies have been reset to defaults\n\r";
            UART_write(uart, &message, sizeof message);
            
            setFreqDefaults(&freqInfo);
            NVS_write(nvsRegion, 0, &freqInfo, sizeof freqInfo, NVS_WRITE_ERASE);
        } 
        else if (strcmp(input_buffer, "setnum") == 0) {
            char message[] = "How many frequencies would you like to use?\n\r";
            UART_write(uart, &message, sizeof message);

            if (getUARTInput(uart, input_buffer, sizeof input_buffer) == -1) {
                /* Input failed due to buffer running out of space */
                char message[] = "Input buffer ran out.\n\r\n";
                UART_write(uart, &message, sizeof message);
                continue;
            }
            
            long n;
            n = strtol(input_buffer, NULL, 10);

            if (n < 1 || n > MAX_N_FREQUENCIES) {
                char message[] = "Invalid number/input.\n\r\n";
                UART_write(uart, &message, sizeof message);
                continue;
            }

            freqInfo.noFreqUsed = n;
            NVS_write(nvsRegion, 0, &freqInfo, sizeof freqInfo, NVS_WRITE_ERASE);
        }
        else if (strcmp(input_buffer, "info") == 0) {
            char message[200];
            memset(message, 0, sizeof message);
            snprintf(message, sizeof message,
                "Receiver Board Info\n\r"
                "-------------------\n\r"
                "\tNumber of frequencies in use:\t%d\n\r", freqInfo.noFreqUsed);
            UART_write(uart, &message, sizeof message);
            int i;
            for (i = 0; i < freqInfo.noFreqUsed; i++) {
                char message[100];
                memset(message, 0, sizeof message);
                snprintf(message, sizeof message, "\t%d:\t%dHz\n\r", (i+1), freqInfo.freqList[i]);
                UART_write(uart, &message, sizeof message);
            }

            char message2[100];
            memset(message2, 0, sizeof message2);
            snprintf(message2, sizeof message2, "\n\n\tType: %d\n\r\tHorizontal Distance: %.3f m\n\r", freqInfo.type, freqInfo.distanceMetres);
            UART_write(uart, &message2, sizeof message2);

        }
        else if (strcmp(input_buffer, "setdebug") == 0) {
                    /* Set board setconfig */
                    char message[] = "Enter 0 for OFF and 1 for ON:\r\n";
                    UART_write(uart, &message, sizeof message);

                    if (getUARTInput(uart, input_buffer, sizeof input_buffer) == -1) {
                        /* Input failed due to buffer running out of space */
                        char message[] = "Input buffer ran out.\n\r\n";
                        UART_write(uart, &message, sizeof message);
                        continue;
                    }

                    if (input_buffer[0] == '0') {
                        freqInfo.debug = 0;
                        NVS_write(nvsRegion, 0, &freqInfo, sizeof(freqInfo), NVS_WRITE_ERASE);
                    } else if (input_buffer[0] == '1') {
                        freqInfo.debug = 1;
                        NVS_write(nvsRegion, 0, &freqInfo, sizeof(freqInfo), NVS_WRITE_ERASE);
                    } else {
                        char message[] = "Invalid input\n\r\n";
                        UART_write(uart, &message, sizeof message);
                    }
                }
        else if(strcmp(input_buffer, "change") == 0){

            char message[] = "Enter 0 for Rx and 1 for Tx:\r\n";
            UART_write(uart, &message, sizeof message);

            if (getUARTInput(uart, input_buffer, sizeof input_buffer) == -1) {
                                    /* Input failed due to buffer running out of space */
                char message[] = "Input buffer ran out.\n\r\n";
                UART_write(uart, &message, sizeof message);
                continue;
            }
            if (input_buffer[0] == '0') {
               freqInfo.type = 0;
               NVS_write(nvsRegion, 0, &freqInfo, sizeof(freqInfo), NVS_WRITE_ERASE);
           } else if (input_buffer[0] == '1') {
               freqInfo.type = 1;
               NVS_write(nvsRegion, 0, &freqInfo, sizeof(freqInfo), NVS_WRITE_ERASE);
           } else {
               char message[] = "Invalid input\n\r\n";
               UART_write(uart, &message, sizeof message);
           }

        }
        else if(strcmp(input_buffer, "setdis") == 0){

            char message[] = "Enter distance:\r\n";
            UART_write(uart, &message, sizeof message);

            float dis;

            if (getUARTInput(uart, input_buffer, sizeof input_buffer) == -1) {
                /* Input failed due to buffer running out of space */
                char message[] = "Input buffer ran out.\n\r\n";
                UART_write(uart, &message, sizeof message);
                continue;
            }

            dis = strtof(input_buffer, NULL);

            if (dis < 0 || dis > 1000) {
                char message[] = "Invalid number/input.\n\r\n";
                UART_write(uart, &message, sizeof message);
                continue;
            }else{
                freqInfo.distanceMetres = dis;
                char message2[50];
                memset(message2, 0, sizeof message2);
                snprintf(message2, sizeof message2, "Distance: %.3f m\n\r",freqInfo.distanceMetres);
                UART_write(uart, &message2, sizeof message2);
            }
        }
        else if(strcmp(input_buffer, "setvdis") == 0){

           char message[] = "Enter vertical distance:\r\n";
           UART_write(uart, &message, sizeof message);

           float dis;

           if (getUARTInput(uart, input_buffer, sizeof input_buffer) == -1) {
               /* Input failed due to buffer running out of space */
               char message[] = "Input buffer ran out.\n\r\n";
               UART_write(uart, &message, sizeof message);
               continue;
           }

           dis = strtof(input_buffer, NULL);

           if (dis < 0 || dis > 100) {
               char message[] = "Invalid number/input.\n\r\n";
               UART_write(uart, &message, sizeof message);
               continue;
           }else{
               freqInfo.vDistance = dis;
               char message2[50];
               memset(message2, 0, sizeof message2);
               snprintf(message2, sizeof message2, "Distance: %.3f m\n\r",freqInfo.vDistance);
               UART_write(uart, &message2, sizeof message2);
           }
       }
        else {
            char message[] = "Command not Recognised\n\r\n";
            UART_write(uart, &message, sizeof message);
        }
    }
}

/* Uart Command Task */
static void uartCommandTaskFunction() {

    /* Enter main loop */
    while (1) {
        /* Wait for a key press to start the uart command interface */
        char input;
        UART_read(uart, &input, 1);

        /* Get access to UART */
        Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);

        /* Start the UART command interface */
        commandInterface();

        /* Release the UART mutex */
        Semaphore_post(UART_sem);
    }
}

/* Setup the uart task */
void uartCommandTask_init(NVS_Handle handle, rxFrequencies rxF, Semaphore_Handle sem, UART_Handle uart_handle) {
    nvsRegion = handle;
    freqInfo = rxF;

    UART_sem = sem;
    uart = uart_handle;

    Task_Params uartTaskParams;

    Task_Params_init(&uartTaskParams);
    uartTaskParams.stackSize = UART_TASK_STACK_SIZE;
    uartTaskParams.priority = UART_TASK_PRIORITY;
    uartTaskParams.stack = &uartTaskStack;

    Task_construct(&uartCommandTask, uartCommandTaskFunction, &uartTaskParams, NULL);
}

