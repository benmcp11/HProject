/* Standard C Libraries */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

/* XDCtools Header files */
#include <xdc/runtime/Assert.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <xdc/std.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Clock.h>

/* TI-RTOS Drivers */
#include <ti/drivers/PIN.h>
#include <ti/drivers/UART.h>
#include <ti/display/Display.h>
#include <ti/display/DisplayExt.h>
#include <ti/drivers/GPIO.h>

/* Board Header files */
#include "Board.h"
#include <ti/drivers/Board.h>

/* EasyLink API Header files */
#include "easylink/EasyLink.h"
#include "rxTask.h"

#include "rxDataStructs.h"
#include "DataStructs.h"



/***** Defines *****/

/* Undefine to remove async mode */
//#define RFEASYLINKRX_ASYNC

#define RFEASYLINKRX_TASK_STACK_SIZE    2048
#define RFEASYLINKRX_TASK_PRIORITY      2
#define MAX_PAYLOAD_SIZE                10
#define RX_TIMEOUT                      500000

#define RFEASYLINKTX_TASK_STACK_SIZE 1024
#define RFEASYLINKTX_TASK_PRIORITY 2

#define RFEASYLINKTX_BURST_SIZE 10
#define RFEASYLINKTXPAYLOAD_LENGTH 127

#define RFEASYLINKTX_TASK_STACK_SIZE 1024
#define RFEASYLINKTX_TASK_PRIORITY 2

#define USED_PAYLOAD_SPACE 7

/***** Variable declarations *****/
static Task_Params rxTaskParams;
Task_Struct rxTask;    /* not static so you can see in ROV */
static uint8_t rxTaskStack[RFEASYLINKRX_TASK_STACK_SIZE];
int_fast32_t error;

/* LED pin handle */
PIN_Handle pinHandle;

/* UART lock */
Semaphore_Handle UART_sem;

/* Uart Handle */
UART_Handle uart;

/* Frequency list */
rxFrequencies freqInfo;
rxPacketInfo rxInfo1;

rxPacketInfo rxInfo;

TX_Info txData;

/***** Function definitions *****/


void processPacket(rxPacketInfo* rxInfo, EasyLink_RxPacket* rxPacket, int8_t* latestRssi){

        int packetNum = rxPacket->payload[6];
        if (packetNum < rxInfo->prevPacketNumber){
            rxInfo->numberOfPacketOverflows++;
        }
        rxInfo->prevPacketNumber = packetNum;

        // construct the frequency value from the payload
        int32_t first_quarter = ((uint32_t)rxPacket->payload[2]) << 24;
        int32_t second_quarter = ((uint32_t)rxPacket->payload[3]) << 16;
        int32_t third_quarter = ((uint32_t)rxPacket->payload[4]) << 8;
        int32_t fourth_quarter = ((uint32_t)rxPacket->payload[5]);
        rxInfo->txFrequency = first_quarter + second_quarter + third_quarter + fourth_quarter;

        // get the rest of the data from packet
        rxInfo->nodeID = rxPacket->payload[0];
        rxInfo->refFlag = rxPacket->payload[1];
        rxInfo->packetNum = packetNum + (rxInfo->numberOfPacketOverflows * 255);
        rxInfo->abs_time = rxPacket->absTime;

        *latestRssi = (int8_t)rxPacket->rssi;
}

void transmitReturn(EasyLink_TxPacket txPacket, uint32_t packetNo) {

    txPacket.payload[0] = 'R';
    txPacket.payload[1] = packetNo;

    txPacket.len = 2;

    EasyLink_Status result = EasyLink_transmit(&txPacket);

    if (result == EasyLink_Status_Success) {

            // Toggle DIO1 to indicate TX end (timing purposes):
        GPIO_write(Board_DIO1, 0);


        /* Toggle LED1 to indicate TX */
        PIN_setOutputValue(pinHandle, Board_PIN_LED1, !PIN_getOutputValue(Board_PIN_LED1));
        Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
        char message[50];
        memset(message, 0, sizeof message);
        snprintf(message, sizeof message, "Tx Success: %d\n\r", packetNo);
        UART_write(uart, &message, sizeof message);

       Semaphore_post(UART_sem);

    } else {
        /* Toggle LED1 and LED2 to indicate error */
            PIN_setOutputValue(pinHandle, Board_PIN_LED1, !PIN_getOutputValue(Board_PIN_LED1));
            PIN_setOutputValue(pinHandle, Board_PIN_LED2, !PIN_getOutputValue(Board_PIN_LED2));


    }

}

static void rfEasyLinkRxFnx(UArg arg0, UArg arg1)
{

    Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
    char m[] = "Rx Task Started\n\r";
    UART_write(uart, &m, sizeof m);
    Semaphore_post(UART_sem);

    EasyLink_RxPacket rxPacket1 = {0};

    // Initialize the EasyLink parameters to their default values
    EasyLink_Params easyLink_params;
    EasyLink_Params_init(&easyLink_params);

    /*
     * Initialize EasyLink with the settings found in easylink_config.h
     * Modify EASYLINK_PARAM_CONFIG in easylink_config.h to change the default
     * PHY
     */
    if(EasyLink_init(&easyLink_params) != EasyLink_Status_Success)
    {
        System_abort("EasyLink_init failed");
    }

    /*
     * If you wish to use a frequency other than the default, use
     * the following API:
     * EasyLink_setFrequency(868000000);
     */
    int frequencyIndex = 0;
    EasyLink_setFrequency(freqInfo.freqList[frequencyIndex]);
    int8_t latestRssi1;
    rxInfo1.numberOfPacketOverflows = 0;
    rxInfo1.prevPacketNumber = 0;
    int packetNo = 0;
    while(1) {

        rxPacket1.absTime = 0;
        rxPacket1.rxTimeout = RX_TIMEOUT;

        if (freqInfo.debug == 1){
            // Toggle DIO1 to indicate RX:
            GPIO_write(Board_DIO1, 1);
        }

        // receive two packets on each frequency:
        EasyLink_Status packetStatus1 = EasyLink_receive(&rxPacket1);

        if (freqInfo.debug == 1){
            GPIO_write(Board_DIO1, 0);
            PIN_setOutputValue(pinHandle, Board_PIN_LED1, 0);
            PIN_setOutputValue(pinHandle, Board_PIN_LED2, 0);
        }



        if (packetStatus1 == EasyLink_Status_Success) {

            /* Toggle LED2 to indicate RX */
            if (freqInfo.debug == 1){
                PIN_setOutputValue(pinHandle, Board_PIN_LED2, !PIN_getOutputValue(Board_PIN_LED2));
            }

            processPacket(&rxInfo1, &rxPacket1, &latestRssi1);

            // check if the two packets are on the same frequency and the frequency is the one we are currently listening to:
            if (rxInfo1.txFrequency != freqInfo.freqList[frequencyIndex]) {
                frequencyIndex++;
                if (freqInfo.debug == 1){
                    PIN_setOutputValue(pinHandle, Board_PIN_LED1, 0);
                    PIN_setOutputValue(pinHandle, Board_PIN_LED2, 1);
                }
                continue;
            }

            // Turn green LED on to show success of reading both packets on same frequency
            if (freqInfo.debug == 1){
                PIN_setOutputValue(pinHandle, Board_PIN_LED1, 1);
                PIN_setOutputValue(pinHandle, Board_PIN_LED2, 0);
            }

            // print format: "node_number refFlag rssi transmitterFreq transmitterPower"
            Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
            char message[50];
            memset(message, 0, sizeof message);
            snprintf(message, sizeof message, "%d_%d_%d_%d_%d_%07u\n\r", rxInfo1.nodeID, rxInfo1.refFlag, latestRssi1, rxInfo1.txFrequency, rxInfo1.packetNum,  EasyLink_RadioTime_To_ms(rxInfo1.abs_time));
            UART_write(uart, &message, sizeof message);

            Semaphore_post(UART_sem);

        } else {
            /* Turn red LED on to indicate error */
            if (freqInfo.debug == 1){
                PIN_setOutputValue(pinHandle, Board_PIN_LED1, 0);
                PIN_setOutputValue(pinHandle, Board_PIN_LED2, 1);
            }
        }
        EasyLink_TxPacket txPacket = {{0}, 0, 0, {0}};


        frequencyIndex++;
        if (frequencyIndex >= freqInfo.noFreqUsed) {
            frequencyIndex = 0;
        }
        EasyLink_setFrequency(freqInfo.freqList[frequencyIndex]);

        transmitReturn(txPacket,packetNo);
        packetNo++;
    }
}

void processPacketNew(rxPacketInfo* rxInfo, EasyLink_RxPacket* rxPacket, int8_t* latestRssi){

        int packetNum = rxPacket->payload[1];
        if (packetNum < rxInfo->prevPacketNumber){
            rxInfo->numberOfPacketOverflows++;
        }
        rxInfo->prevPacketNumber = packetNum;

        // get the rest of the data from packet
        rxInfo->nodeID = rxPacket->payload[0];
        rxInfo->packetNum = packetNum + (rxInfo->numberOfPacketOverflows * 255);
        //rxInfo->packetNum = packetNum;
        rxInfo->iteration = rxPacket->payload[2];
        rxInfo->abs_time = rxPacket->absTime;
        rxInfo->timeTaken = (uint8_t)rxPacket->payload[4];
        rxInfo->rcvDistance = (float)rxPacket->payload[5];

        *latestRssi = (int8_t)rxPacket->rssi;
}

static void txCreatePacket(EasyLink_TxPacket * txPacket){
    txPacket->payload[0] = txData.nodeID = 1;
    txPacket->payload[1] = 0; //Packet number
    txPacket->payload[2] = 0; //Iteration
    txPacket->payload[3] = 0; //Timeout Y/N
    txPacket->payload[4] = 0; //Absolute Time
    txPacket->payload[5] = freqInfo.vDistance; //vDistance

    txPacket->len = USED_PAYLOAD_SPACE;
    txPacket->dstAddr[0] = 0xaa;
}

static void txPhase(EasyLink_TxPacket txPacket, int iteration, ){

    clock_t start, end;

    uint8_t i =0;

    while(1){
        start = clock();
        txPacket.absTime = 0;
        txPacket.payload[1] = i;
        txPacket.payload[2] = iteration;

        // Toggle DIO1 to indicate TX start (timing purposes):
        if (txData.debug) {
            GPIO_write(Board_DIO1, 1);
        }
        end = clock();
        txPacket.payload[4] = (int8_t) ((end-start));
        
        EasyLink_Status result = EasyLink_transmit(&txPacket);

        if (result == EasyLink_Status_Success) {
            PIN_setOutputValue(pinHandle, Board_PIN_LED1, !PIN_getOutputValue(Board_PIN_LED1));
        } else {
            /* Toggle LED1 and LED2 to indicate error */

                PIN_setOutputValue(pinHandle, Board_PIN_LED1, !PIN_getOutputValue(Board_PIN_LED1));
                PIN_setOutputValue(pinHandle, Board_PIN_LED2, !PIN_getOutputValue(Board_PIN_LED2));
        }
        i++;
    }
}

static int txReceiveCheck(EasyLink_RxPacket* rxPacket){
    if (rxPacket->payload[0] == 1){return -1;}


    return (int) rxPacket->payload[1];



}

static void txPhase(EasyLink_TxPacket txPacket, int iteration){
    uint8_t packetNo = 0;
    int i = 0;
    int timer = 0;
    EasyLink_RxPacket rxPacket = {0};
    int8_t latestRssi;
    int rxNo = -1;
    while(1){
        i = 0;
        txPacket->payload[3] = 0;

        while(i<10){
            txPacket->payload[2] = packetNo; // PacketNo
            if(i==9){
                txPacket->payload[3] = 1; // Set end flag
            }
            EasyLink_Status result = EasyLink_transmit(&txPacket);


            if (result == EasyLink_Status_Success) {
                PIN_setOutputValue(pinHandle, Board_PIN_LED1, !PIN_getOutputValue(Board_PIN_LED1));
            } else {
                /* Toggle LED1 and LED2 to indicate error */

                    PIN_setOutputValue(pinHandle, Board_PIN_LED1, !PIN_getOutputValue(Board_PIN_LED1));
                    PIN_setOutputValue(pinHandle, Board_PIN_LED2, !PIN_getOutputValue(Board_PIN_LED2));
            }
            i++;
            packetNo++;



        }
        rxPacket.absTime = 0;
        rxPacket.rxTimeout = RX_TIMEOUT;

        if (freqInfo.debug == 1){
            // Toggle DIO1 to indicate RX:
            GPIO_write(Board_DIO1, 1);
        }

        // receive two packets on each frequency:
        EasyLink_Status packetStatus = EasyLink_receive(&rxPacket);

        if (freqInfo.debug == 1){
            GPIO_write(Board_DIO1, 0);
            PIN_setOutputValue(pinHandle, Board_PIN_LED1, 0);
            PIN_setOutputValue(pinHandle, Board_PIN_LED2, 0);
        }


        if(packetStatus == EasyLink_Status_Success){
            PIN_setOutputValue(pinHandle, Board_PIN_LED1, !PIN_getOutputValue(Board_PIN_LED1));
            rxNo = txReceiveCheck(&rxPacket)
        } else {
            PIN_setOutputValue(pinHandle, Board_PIN_LED1, !PIN_getOutputValue(Board_PIN_LED1));
            PIN_setOutputValue(pinHandle, Board_PIN_LED2, !PIN_getOutputValue(Board_PIN_LED2));
        }
        while(rxNo!= 0 && timer < 100000){
            EasyLink_Status packetStatus = EasyLink_receive(&rxPacket);
            if (freqInfo.debug == 1){
                GPIO_write(Board_DIO1, 0);
                PIN_setOutputValue(pinHandle, Board_PIN_LED1, 0);
                PIN_setOutputValue(pinHandle, Board_PIN_LED2, 0);
            }


            if(packetStatus == EasyLink_Status_Success){
                PIN_setOutputValue(pinHandle, Board_PIN_LED1, !PIN_getOutputValue(Board_PIN_LED1));
                rxNo = txReceiveCheck(&rxPacket)
            } else {
                PIN_setOutputValue(pinHandle, Board_PIN_LED1, !PIN_getOutputValue(Board_PIN_LED1));
                PIN_setOutputValue(pinHandle, Board_PIN_LED2, !PIN_getOutputValue(Board_PIN_LED2));
            }

            timer ++;
        }

    }



}

static void txCreatePacketMultiNode(EasyLink_TxPacket * txPacket){

    txPacket->payload[0] = 1; //Tx Flag
    txPacket->payload[1] = txData.nodeID; // Node ID
    txPacket->payload[2] = 0; // Packet No
    txPacket->payload[3] = 0; // EndFlag

    txPacket->len = 4;
    txPacket->dstAddr[0] = 0xaa;
}





static void rxPhase(){
    EasyLink_RxPacket rxPacket = {0};
    int8_t latestRssi;
    while(1){
        rxPacket.absTime = 0;
        rxPacket.rxTimeout = RX_TIMEOUT;

        if (freqInfo.debug == 1){
            // Toggle DIO1 to indicate RX:
            GPIO_write(Board_DIO1, 1);
        }

        // receive two packets on each frequency:
        EasyLink_Status packetStatus1 = EasyLink_receive(&rxPacket);

        if (freqInfo.debug == 1){
            GPIO_write(Board_DIO1, 0);
            PIN_setOutputValue(pinHandle, Board_PIN_LED1, 0);
            PIN_setOutputValue(pinHandle, Board_PIN_LED2, 0);
        }



        if (packetStatus1 == EasyLink_Status_Success) {

            /* Toggle LED2 to indicate RX */
            if (freqInfo.debug == 1){
                PIN_setOutputValue(pinHandle, Board_PIN_LED2, !PIN_getOutputValue(Board_PIN_LED2));
            }

            processPacketNew(&rxInfo, &rxPacket, &latestRssi);

            // check if the two packets are on the same frequency and the frequency is the one we are currently listening to:

            // Turn green LED on to show success of reading both packets on same frequency
            if (freqInfo.debug == 1){
                PIN_setOutputValue(pinHandle, Board_PIN_LED1, 1);
                PIN_setOutputValue(pinHandle, Board_PIN_LED2, 0);
            }

            // print format: "node_number refFlag rssi transmitterFreq transmitterPower"
            Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
            char message[50];
            memset(message, 0, sizeof message);
            snprintf(message, sizeof message, "%d_%d_%d_%d_%07u_%08u_%.3f_%.3f_%.3f\n\r", rxInfo.nodeID, rxInfo.packetNum, rxInfo.iteration, latestRssi,  EasyLink_RadioTime_To_ms(rxInfo.abs_time), rxInfo.timeTaken, freqInfo.distanceMetres,freqInfo.vDistance, rxInfo.rcvDistance);
            UART_write(uart, &message, sizeof message);

            Semaphore_post(UART_sem);

        } else {
            /* Turn red LED on to indicate error */
            if (freqInfo.debug == 1){
                PIN_setOutputValue(pinHandle, Board_PIN_LED1, 0);
                PIN_setOutputValue(pinHandle, Board_PIN_LED2, 1);
            }
        }
    }
}

static void rfEasyLinkRxFnxNew(UArg arg0, UArg arg1){

    Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
    char m[] = "Rx Task Started\n\r";
    UART_write(uart, &m, sizeof m);
    Semaphore_post(UART_sem);

    EasyLink_RxPacket rxPacket = {0};

    // Initialize the EasyLink parameters to their default values
    EasyLink_Params easyLink_params;
    EasyLink_Params_init(&easyLink_params);

    /*
     * Initialize EasyLink with the settings found in easylink_config.h
     * Modify EASYLINK_PARAM_CONFIG in easylink_config.h to change the default
     * PHY
     */
    if(EasyLink_init(&easyLink_params) != EasyLink_Status_Success)
    {
        System_abort("EasyLink_init failed");
    }

    /*
     * If you wish to use a frequency other than the default, use
     * the following API:
     * EasyLink_setFrequency(868000000);
     */
    int frequencyIndex = 0;
    EasyLink_setFrequency(freqInfo.freqList[frequencyIndex]);

    rxInfo.numberOfPacketOverflows = 0;
    rxInfo.prevPacketNumber = 0;

    EasyLink_TxPacket txPacket = {{0}, 0, 0, {0}};
    txCreatePacket(&txPacket);

    int i = 0;
    while(1) {
            if(freqInfo.type ==1){
                txPhase(txPacket, i);
            }
            else if(freqInfo.type == 0){
                rxPhase();
            }
        }
}



void rxTask_init(PIN_Handle ledPinHandle, rxFrequencies rxF, Semaphore_Handle sem, UART_Handle uart_handle) {
    pinHandle = ledPinHandle;
    freqInfo = rxF;
    UART_sem = sem;
    uart = uart_handle;

    Task_Params_init(&rxTaskParams);
    rxTaskParams.stackSize = RFEASYLINKRX_TASK_STACK_SIZE;
    rxTaskParams.priority = RFEASYLINKRX_TASK_PRIORITY;
    rxTaskParams.stack = &rxTaskStack;
    rxTaskParams.arg0 = (UInt)1000000;

    Task_construct(&rxTask, rfEasyLinkRxFnxNew, &rxTaskParams, NULL);
}
