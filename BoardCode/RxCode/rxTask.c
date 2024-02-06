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

#include <math.h>
#include <string.h>
#include <stdlib.h>

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
#define N 5

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

static void txPhase(EasyLink_TxPacket txPacket, int iteration){

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
    Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
    char message[50];
    memset(message, 0, sizeof message);
    snprintf(message, sizeof message, "IN PS\n\r");
    UART_write(uart, &message, sizeof message);
    Semaphore_post(UART_sem);


    if (rxPacket->payload[0] == 1){return -1;}
    return (int) rxPacket->payload[1];

}

static void setPinStatus(EasyLink_Status packetStatus){

    if(packetStatus == EasyLink_Status_Success){
               PIN_setOutputValue(pinHandle, Board_PIN_LED1, !PIN_getOutputValue(Board_PIN_LED1));
    } else {
           PIN_setOutputValue(pinHandle, Board_PIN_LED1, !PIN_getOutputValue(Board_PIN_LED1));
           PIN_setOutputValue(pinHandle, Board_PIN_LED2, !PIN_getOutputValue(Board_PIN_LED2));
       }
}

static void txPhaseMulti(EasyLink_TxPacket *txPacket, int iteration){
    uint8_t packetNo = 0;
    int i = 0;

    int time = 0;
    EasyLink_RxPacket rxPacket = {0};
    int rxNo = -1;

    while(1){
        i = 0;
        txPacket->payload[3] = 0;

        while(i<N){

            txPacket->payload[2] = packetNo; // PacketNo
            if(i==N-1){txPacket->payload[3] = 1;} // Set end flag

            EasyLink_Status result = EasyLink_transmit(txPacket);

            setPinStatus(result);

            i++;
            packetNo++;

        }
        int count = 0;
        char message[50];

        while(count<100){
            count+=1;
//            Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
//            memset(message, 0, sizeof message);
//            snprintf(message, sizeof message, "Count: %d\n\r", count);
//            UART_write(uart, &message, sizeof message);
//            Semaphore_post(UART_sem);
        }
        //Rx section

        rxPacket.absTime = 0;

        if (freqInfo.debug == 1){GPIO_write(Board_DIO1, 1);};

        rxNo = -1;
        time = 0;

        Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
        memset(message, 0, sizeof message);
        snprintf(message, sizeof message, "rxNo: %d PL: %d\n\r",rxNo,rxPacket.payload[1]);
        UART_write(uart, &message, sizeof message);
        Semaphore_post(UART_sem);


        while(rxNo!= 0 && time < 5000){
            EasyLink_Status packetStatus = EasyLink_receive(&rxPacket);
            setPinStatus(packetStatus);

           if(packetStatus == 0){
               rxNo = rxPacket.payload[1];

               Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
               memset(message, 0, sizeof message);
               snprintf(message, sizeof message, "rxNo: %d PL: %d\n\r",rxNo,rxPacket.payload[1]);
               UART_write(uart, &message, sizeof message);
               Semaphore_post(UART_sem);



           }
            time ++;
        }

    }
}

static void txCreatePacketMultiNode(EasyLink_TxPacket * txPacket){

    txPacket->payload[0] = 1; //Tx Flag
    txPacket->payload[1] = freqInfo.nodeID; // Node ID
    txPacket->payload[2] = 0; // Packet No
    txPacket->payload[3] = 0; // EndFlag

    txPacket->len = 4;
    txPacket->dstAddr[0] = 0xaa;
}

typedef struct receivedPacketStruct{
    int packetType;
    int nodeNo;
    int8_t rssi;
    uint8_t packetNo;

}receivedPacketStruct;

typedef struct rxReceivedPacketsStruct{
    receivedPacketStruct txPackets[N];
    int8_t rssiArray[10];


}rxReceivedPacketsStruct;



static void createRxReturnPacket(EasyLink_TxPacket * returnPacket,rxReceivedPacketsStruct * packetsToSend){

    returnPacket->payload[0] = 0; //Tx Flag
    returnPacket->payload[1] = freqInfo.nodeID; // Node ID

    Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
    char message[50];
    memset(message, 0, sizeof message);
    snprintf(message, sizeof message, "IN CREATE RETURN PACKET: %d\n\r",returnPacket->payload[1]);
    UART_write(uart, &message, sizeof message);
    Semaphore_post(UART_sem);


    int i = 0;

    while(i< N){
        returnPacket->payload[2+i*3] = packetsToSend->txPackets[i].nodeNo; //TxNodeID[i]
        returnPacket->payload[3+i*3] = packetsToSend->txPackets[i].rssi; //TxRssi[i]
        returnPacket->payload[4+i*3] = packetsToSend->txPackets[i].packetNo; //TxPacketNo[i]
        i+=1;
    }

    int j = 0;

    while(j<freqInfo.numberOfRx){
        returnPacket->payload[N * 3 + 2 +j] = packetsToSend->rssiArray[j];
        j++;
    }

    returnPacket->len = N * 3 + 2 + freqInfo.numberOfRx;
    returnPacket->dstAddr[0] = 0xaa;
}

static EasyLink_Status sendRxReturnPacket(rxReceivedPacketsStruct * packetsToSend){

    EasyLink_TxPacket returnPacket = {{0}, 0, 0, {0}};
    createRxReturnPacket(&returnPacket, packetsToSend);
    EasyLink_Status result = EasyLink_transmit(&returnPacket);

    Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
    char message[50];
    memset(message, 0, sizeof message);
    snprintf(message, sizeof message, "IN SEND RETURN PACKET: %d\n\r", result);
    UART_write(uart, &message, sizeof message);
    Semaphore_post(UART_sem);


    setPinStatus(result);
    return result;

};


static void processRxPacket(EasyLink_RxPacket * rxPacket){

    int i = 0;
    int nWrites = 0;
    // RxNodeID = payload[1]
    // TxNodeID[i] = payload[2+i*nWrites]
    // TxRssi[i] = payload[3+i*nWrites]
    // TxPacketNo[i] = payload[4+i*nWrites]

    int j = 0;


    while(i<1){
        Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
        char message[300];
        memset(message, 0, sizeof message);

        //RxNodeID,TxNodeID[1]_TxRssi[1]_TxPacketNo[1],TxNodeID[2]_TxRssi[2]_TxPacketNo[2],...TxNodeID[5]_TxRssi[5]_TxPacketNo[5],Rx->RxHost RSSI
        snprintf(message, sizeof message, "%d,%d_%d_%d,%d_%d_%d,%d_%d_%d,%d_%d_%d,%d_%d_%d,%d,",
                 rxPacket->payload[1],
                 rxPacket->payload[2+i*nWrites],(int8_t)rxPacket->payload[3+i*nWrites],rxPacket->payload[4+i*nWrites],
                 rxPacket->payload[5+i*nWrites],(int8_t)rxPacket->payload[6+i*nWrites],rxPacket->payload[7+i*nWrites],
                 rxPacket->payload[8+i*nWrites],(int8_t)rxPacket->payload[9+i*nWrites],rxPacket->payload[10+i*nWrites],
                 rxPacket->payload[11+i*nWrites],(int8_t)rxPacket->payload[12+i*nWrites],rxPacket->payload[13+i*nWrites],
                 rxPacket->payload[14+i*nWrites],(int8_t)rxPacket->payload[15+i*nWrites],rxPacket->payload[16+i*nWrites],
                 rxPacket->rssi);

        char rssiStr[12]; // Twice the size for numbers and underscores
        memset(rssiStr,0, sizeof(rssiStr));

        while(j<freqInfo.numberOfRx){
            if(j<freqInfo.numberOfRx-1){
                snprintf(rssiStr, sizeof rssiStr, "%d_",(int8_t)rxPacket->payload[17+j]);
            }else{
                snprintf(rssiStr, sizeof rssiStr, "%d\n\r",(int8_t)rxPacket->payload[17+j]);
            }

            strcat(message, rssiStr);
            j++;
        }



        UART_write(uart, &message, sizeof message);
        i+=1;
        Semaphore_post(UART_sem);
    };
}



static void processHostData(rxReceivedPacketsStruct * hostPackets){

    int i = 0;

//    hostPackets->txPackets[i].nodeNo; //TxNodeID[i]
//    hostPackets->txPackets[i].rssi; //TxRssi[i]
//    hostPackets->txPackets[i].packetNo; //TxPacketNo[i]

//    char deb[300];
//    Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
//    memset(deb, 0, sizeof deb);
//    snprintf(deb, sizeof deb, "IN PROCESS\n\r");
//    UART_write(uart, &deb, sizeof deb);
//    Semaphore_post(UART_sem);

    char message[300];
    memset(message, 0, sizeof message);

    snprintf(message, sizeof message, "%d,%d_%d_%d,%d_%d_%d,%d_%d_%d,%d_%d_%d,%d_%d_%d,%d\n\r",
             0,
             hostPackets->txPackets[0].nodeNo, hostPackets->txPackets[0].rssi,hostPackets->txPackets[0].packetNo,
             hostPackets->txPackets[1].nodeNo, hostPackets->txPackets[1].rssi,hostPackets->txPackets[1].packetNo,
             hostPackets->txPackets[2].nodeNo, hostPackets->txPackets[2].rssi,hostPackets->txPackets[2].packetNo,
             hostPackets->txPackets[3].nodeNo, hostPackets->txPackets[3].rssi,hostPackets->txPackets[3].packetNo,
             hostPackets->txPackets[4].nodeNo, hostPackets->txPackets[4].rssi,hostPackets->txPackets[4].packetNo,
             9);




    Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
    UART_write(uart, &message, sizeof message);
    Semaphore_post(UART_sem);

}

typedef struct RxNode{
    int nodeNo;
    int8_t lastRssi;
    int x;
    int y;
    int n;


}rxNode;



static void rxPhaseMulti(){
    rxReceivedPacketsStruct receivedPacketsArr;

//    rxNode rxNodeArr[freqInfo.numberOfRx];


    EasyLink_RxPacket rxPacket = {0};
    EasyLink_Status packetStatus;

    EasyLink_TxPacket returnPacket = {{0}, 0, 0, {0}};

    returnPacket.payload[0] = 0; //Tx Flag
    returnPacket.payload[1] = freqInfo.nodeID; // Node ID
    returnPacket.len = 2;
    returnPacket.dstAddr[0] = 0xaa;

    rxPacket.absTime = 0;
    rxPacket.rxTimeout = RX_TIMEOUT;
    int packetCount = 0;

    while(1){

        if (freqInfo.debug == 1){GPIO_write(Board_DIO1, 1);};
        packetStatus = EasyLink_receive(&rxPacket);

        if (freqInfo.debug == 1){
            GPIO_write(Board_DIO1, 0);
            PIN_setOutputValue(pinHandle, Board_PIN_LED1, 0);
            PIN_setOutputValue(pinHandle, Board_PIN_LED2, 0);
        }

        setPinStatus(packetStatus);
        if(packetStatus != EasyLink_Status_Success){continue;};

        // Tx Flag = payload[0]
        // NodeId = payload[1]
        // PacketNo = payload[2]
        // EndFlag = payload[3]

        receivedPacketStruct packetIn;

        // If a Tx Packet
        if(rxPacket.payload[0] == 1){
            packetIn.packetType = 1;
            packetIn.nodeNo = rxPacket.payload[1];
            packetIn.packetNo = rxPacket.payload[2];
            packetIn.rssi = rxPacket.rssi;

            receivedPacketsArr.txPackets[packetCount%N] = packetIn;

            packetCount++;

            //If EndFlag set

            if(freqInfo.nodeID == 1 && rxPacket.payload[3] == 1){
                sendRxReturnPacket(&receivedPacketsArr);
            }


        //If an RxPacket
        } else if(rxPacket.payload[0] == 0){
            char message[50];

            receivedPacketsArr.rssiArray[(int)rxPacket.payload[1]] = rxPacket.rssi;



            //If this node is the host
            if(freqInfo.nodeID == 0){
                processRxPacket(&rxPacket);

//                Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
//                memset(message, 0, sizeof message);
//                snprintf(message, sizeof message, "%d\n\r",rxPacket.payload[1]);
//                UART_write(uart, &message, sizeof message);
//                Semaphore_post(UART_sem);

                //If it is the last Rx send packet containing nodeID
                if((int)rxPacket.payload[1] == freqInfo.numberOfRx-1){

//                    char message[50];
//                    Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
//                    memset(message, 0, sizeof message);
//                    snprintf(message, sizeof message, "IN PAYLOAD == 2, %d\n\r",rxPacket.payload[1]);
//                    UART_write(uart, &message, sizeof message);
//                    Semaphore_post(UART_sem);
                    processHostData(&receivedPacketsArr);

                    EasyLink_Status result = EasyLink_transmit(&returnPacket);
                    setPinStatus(result);

//                    Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
//                    memset(message, 0, sizeof message);
//                    snprintf(message, sizeof message, "SENT: %d\n\r",result);
//                    UART_write(uart, &message, sizeof message);
//                    Semaphore_post(UART_sem);
                }
            }



            // If the node before this one AND this is not node 1

            if(freqInfo.nodeID > 1){
                char message[50];
                int rxNo = rxPacket.payload[1];

                if(rxNo == freqInfo.nodeID - 1){
//                    Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
//
//                    memset(message, 0, sizeof message);
//                    snprintf(message, sizeof message, "HERE from 1\n\r");
//                    UART_write(uart, &message, sizeof message);
//                    Semaphore_post(UART_sem);


                    // Delay Loop to allow Host Rx to display results of Rx1
                    int count = 0;

                    if(freqInfo.nodeID == 3){
                        while(count<4){

                            Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
                            memset(message, 0, sizeof message);
                            snprintf(message, sizeof message, "RxNodes: %d\n\r", receivedPacketsArr.rssiArray[count]);
                            UART_write(uart, &message, sizeof message);
                            Semaphore_post(UART_sem);
                            count+=1;

                        }
                    } else{
                        while(count<4){

                            Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
                            memset(message, 0, sizeof message);
                            snprintf(message, sizeof message, "COUNT: %d\n\r",count);
                            UART_write(uart, &message, sizeof message);
                            Semaphore_post(UART_sem);
                            count+=1;

                        }
                    }

                    sendRxReturnPacket(&receivedPacketsArr);
                }



//                if(rxPacket.payload[1] == 0){
//                    Semaphore_pend(UART_sem, BIOS_WAIT_FOREVER);
//                     memset(message, 0, sizeof message);
//                     snprintf(message, sizeof message, "HERE from HOST\n\r");
//                     UART_write(uart, &message, sizeof message);
//                     Semaphore_post(UART_sem);
//                }


            }
        }
    }
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
    txCreatePacketMultiNode(&txPacket);

    int i = 0;
    while(1) {
            if(freqInfo.type == 1){
                txPhaseMulti(&txPacket, i);
            }
            else if(freqInfo.type == 0){
                rxPhaseMulti();
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
