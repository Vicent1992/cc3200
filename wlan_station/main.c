//*****************************************************************************
//
//  Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//  Redistributions of source code must retain the above copyright 
//  notice, this list of conditions and the following disclaimer.
//
//  Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the 
//  documentation and/or other materials provided with the   
//  distribution.
//
//  Neither the name of Texas Instruments Incorporated nor the names of
//  its contributors may be used to endorse or promote products derived
//  from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// Standard includes
#include <stdlib.h>
#include <string.h>

// Simplelink includes
#include "simplelink.h"

//Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"

//Free_rtos/ti-rtos includes
#include "osi.h"

//Common interface includes
#include "gpio_if.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "common.h"
#include "pinmux.h"
#include "sta_configure.h"
#include "httpclient.h"

#define APPLICATION_NAME        "vilink station"
#define APPLICATION_VERSION     "1.0.0"

#define OSI_STACK_SIZE      2048

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

static void DisplayBanner(char * AppName)
{

    UART_PRINT("\n\n\n\r");
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\t\t           CC3200 %s Application       \n\r", AppName);
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\n\n\n\r");
}

static void BoardInit(void)
{
#ifndef USE_TIRTOS
#if defined(ccs) || defined(gcc)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif

    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);
    PRCMCC3200MCUInit();
}

void LedLoopCtrl()
{
	UART_PRINT("LedLoopCtrl looping \n\r");
	while(1){
		GPIO_IF_LedOff(MCU_ALL_LED_IND);
		MAP_UtilsDelay(1000000);
		GPIO_IF_LedOn(MCU_ALL_LED_IND);
		MAP_UtilsDelay(1000000);
	}
}

void ViLinkStationMode( void *pvParameters )
{
    long lRetVal = -1;
    int retryTimes;
    HTTPCli_Struct httpClient;

    while(1){
		lRetVal = ViLinkStartStation();
		if (lRetVal < 0)
		{
			UART_PRINT("lRetVal [%d]:Failed to start the vilink station and connect ap\n\r", lRetVal);
		}
		UART_PRINT("ViLinkStartStation ok....\n\r\r\n");

		// Turn on GREEN LED when device gets PING response from AP
		GPIO_IF_LedOn(MCU_EXECUTE_SUCCESS_IND);

		MAP_UtilsDelay(10000000);
		ConnectToHTTPServer(&httpClient);
		if (lRetVal < 0)
		{
			UART_PRINT("lRetVal [%d]:Failed to Connect HTTPServer\n\r", lRetVal);
		}

		retryTimes = 0;
		while(retryTimes <= 20){
			MAP_UtilsDelay(10000000);
			lRetVal = HTTPGetMethod(&httpClient);
			if (lRetVal < 0)
			{
				retryTimes++;
				UART_PRINT("lRetVal [%d]:Failed to Get HTTPServer Data\n\r", lRetVal);
				if((retryTimes % 5) == 0){
					UART_PRINT("Try to restart http server connect...\n\r", lRetVal);
					DisConnectToHTTPServer(&httpClient);
					MAP_UtilsDelay(10000000);
					ConnectToHTTPServer(&httpClient);
				}
			}else{
				retryTimes = 0;
			}
		}

		DisConnectToHTTPServer(&httpClient);

		lRetVal = ViLinkStopStation();
		if (lRetVal < 0)
		{
			UART_PRINT("lRetVal [%d]:Failed to stop the vilink station \n\r", lRetVal);
			continue;
		}
		GPIO_IF_LedOff(MCU_EXECUTE_SUCCESS_IND);
		UART_PRINT("ViLinkStopStation end....\n\r\n\r");
    }
}

void main()
{
    long lRetVal = -1;

    BoardInit();
    PinMuxConfig();
#ifndef NOTERM
    InitTerm();
#endif  //NOTERM
    GPIO_IF_LedConfigure(LED1|LED2|LED3);
    GPIO_IF_LedOff(MCU_ALL_LED_IND);

    DisplayBanner(APPLICATION_NAME);


	lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
	if(lRetVal < 0)
		ERR_PRINT(lRetVal);

	lRetVal = osi_TaskCreate( ViLinkStationMode, \
							  (const signed char*)"ViLink Station Task", \
							  OSI_STACK_SIZE, NULL, 1, NULL );
	if(lRetVal < 0)
		ERR_PRINT(lRetVal);

	osi_start();

	UART_PRINT("vilink main exit....\n\r");
  }
