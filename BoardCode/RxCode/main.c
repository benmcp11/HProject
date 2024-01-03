/*
 * Copyright (c) 2015-2018, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== rfEasyLinkRx.c ========
 */
/* Standard C Libraries */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
#include <ti/sysbios/knl/Semaphore.h>

/* TI-RTOS Drivers */
#include <ti/drivers/PIN.h>
#include <ti/drivers/UART.h>
#include <ti/display/Display.h>
#include <ti/display/DisplayExt.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/NVS.h>

/* NVS Structure */
#include "rxDataStructs.h"

/* Board Header files */
#include "Board.h"
#include <ti/drivers/Board.h>

/* EasyLink API Header files */
#include "easylink/EasyLink.h"
#include "smartrf_settings/smartrf_settings.h"
#include "rxTask.h"
#include "uartCommandTask.h"


/* Pin driver handle */
static PIN_Handle ledPinHandle;
static PIN_State ledPinState;

/* NVS shenangingangagsns */

#define Board_NVS0      0x1A000

NVS_Handle nvsRegion;
NVS_Attrs regionAttrs;

uint_fast16_t status;


/*
* Application LED pin configuration table:
*   - All LEDs board LEDs are off.
*/
PIN_Config pinTable[] = {
    Board_PIN_LED1               | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    Board_PIN_LED2               | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    CC1310_LAUNCHXL_DIO30_ANALOG | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    CC1310_LAUNCHXL_DIO28_ANALOG | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

/* The RX Output struct contains statistics about the RX operation of the radio */
PIN_Handle pinHandle;

/* UART mutex */
Semaphore_Struct semStruct;
Semaphore_Handle UART_sem;

/* UART Handle */
UART_Handle uart;

/*
 * ========= functions ===========
 */


/*
 *  ======== main ========
 */
int main(void)
{
    /* Call driver init functions */
    Board_initGeneral();

    /* Init the UART */
    UART_Params params;

    UART_init();

    UART_Params_init(&params);
    params.baudRate = 115200;
    params.writeDataMode = UART_DATA_TEXT;
    params.readDataMode = UART_DATA_TEXT;
    params.writeMode = UART_MODE_BLOCKING;
    params.readMode = UART_MODE_BLOCKING;
    params.readTimeout = UART_WAIT_FOREVER;
    params.writeTimeout = UART_WAIT_FOREVER;

    uart = UART_open(Board_UART0, &params);

    // initialize GPIO pins:
    GPIO_init();

    /* Init the NVS */
    NVS_init();
    nvsRegion = NVS_open(Board_NVS0, NULL);

    /* Load board data from nvs */
    rxFrequencies freqInfo;
    NVS_read(nvsRegion, 0, &freqInfo, sizeof(freqInfo));
    if (checkFreqData(&freqInfo) == DATA_REWRITE_FLAG) {
        NVS_write(nvsRegion, 0, &freqInfo, sizeof(freqInfo), NVS_WRITE_ERASE);
    }



    /* Open LED pins */
    ledPinHandle = PIN_open(&ledPinState, pinTable);
    Assert_isTrue(ledPinHandle != NULL, NULL);

    /* Clear LED pins */
    if (freqInfo.debug == 1){
        PIN_setOutputValue(ledPinHandle, Board_PIN_LED1, 0);
        PIN_setOutputValue(ledPinHandle, Board_PIN_LED2, 0);
    }

    // Set pin 28 to HI. Enables HGM
    PIN_setOutputValue(pinHandle, CC1310_LAUNCHXL_DIO28_ANALOG, 1);

    // Map RFC_GPO0 to DIO29 and RFC_GPO1 to DIO30.
    // This connects the PA_EN and LNA_EN signals to the amplifier.
    IOCPortConfigureSet(IOID_29, IOC_PORT_RFC_GPO0, IOC_IOMODE_NORMAL);
    IOCPortConfigureSet(IOID_30, IOC_PORT_RFC_GPO1, IOC_IOMODE_NORMAL);

    rxTask_init(ledPinHandle, freqInfo, UART_sem, uart);
    uartCommandTask_init(nvsRegion, freqInfo, UART_sem, uart);

    /* Create the Semaphore for UART */
    Semaphore_Params semParams;
    Semaphore_Params_init(&semParams);
    Semaphore_construct(&semStruct, 1, &semParams);
    UART_sem = Semaphore_handle(&semStruct);


    /* Start BIOS */
    BIOS_start();
    
    return (0);
}
