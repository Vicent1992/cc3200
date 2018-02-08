#include <string.h>

// SimpleLink includes
#include "simplelink.h"

// driverlib includes
#include "hw_ints.h"
#include "hw_types.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "utils.h"
#include "interrupt.h"

// common interface includes
#include "uart_if.h"
#include "gpio_if.h"
#include "common.h"
#include "pinmux.h"


// JSON Parser
#include "jsmn.h"

#include "httpclient.h"

//#define HOST_NAME       	"2.vilink.applinzi.com"
#define HOST_NAME       	"785d63df.ngrok.io"
#define HOST_PORT           (80)
#define GET_REQUEST_URI 	"/downup.php?token=doubleq\&data=1"

#define PROXY_IP       	    <proxy_ip>
#define PROXY_PORT          <proxy_port>

#define MAX_BUFF_SIZE       2048

#define LED_CTRL_TYPE  "ledswitch"
#define TEMP_CTRL_TYPE "sensor"
#define NO_CTRL_TYPE   "vilink no use data"

unsigned long  g_ulDestinationIP; // IP address of destination server
char getBuff[MAX_BUFF_SIZE+1];
bool g_ledStatus = 0;

// Application specific status/error codes
typedef enum{
 /* Choosing this number to avoid overlap with host-driver's error codes */
    DEVICE_NOT_IN_STATION_MODE = -0x7D0,
    DEVICE_START_FAILED = DEVICE_NOT_IN_STATION_MODE - 1,
    INVALID_HEX_STRING = DEVICE_START_FAILED - 1,
    TCP_RECV_ERROR = INVALID_HEX_STRING - 1,
    TCP_SEND_ERROR = TCP_RECV_ERROR - 1,
    FILE_NOT_FOUND_ERROR = TCP_SEND_ERROR - 1,
    INVALID_SERVER_RESPONSE = FILE_NOT_FOUND_ERROR - 1,
    FORMAT_NOT_SUPPORTED = INVALID_SERVER_RESPONSE - 1,
    FILE_OPEN_FAILED = FORMAT_NOT_SUPPORTED - 1,
    FILE_WRITE_ERROR = FILE_OPEN_FAILED - 1,
    INVALID_FILE = FILE_WRITE_ERROR - 1,
    SERVER_CONNECTION_FAILED = INVALID_FILE - 1,
    GET_HOST_IP_FAILED = SERVER_CONNECTION_FAILED  - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

//*****************************************************************************
//
//! \brief Flush response body.
//!
//! \param[in]  httpClient - Pointer to HTTP Client instance
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
static int FlushHTTPResponse(HTTPCli_Handle httpClient)
{
    const char *ids[2] = {
                            HTTPCli_FIELD_NAME_CONNECTION, /* App will get connection header value. all others will skip by lib */
                            NULL
                         };
    char buf[128];
    int id;
    int len = 1;
    bool moreFlag = 0;
    char ** prevRespFilelds = NULL;


    /* Store previosly store array if any */
    prevRespFilelds = HTTPCli_setResponseFields(httpClient, ids);

    /* Read response headers */
    while ((id = HTTPCli_getResponseField(httpClient, buf, sizeof(buf), &moreFlag))
            != HTTPCli_FIELD_ID_END)
    {

        if(id == 0)
        {
            if(!strncmp(buf, "close", sizeof("close")))
            {
                UART_PRINT("Connection terminated by server\n\r");
            }
        }

    }

    /* Restore previosuly store array if any */
    HTTPCli_setResponseFields(httpClient, (const char **)prevRespFilelds);

    while(1)
    {
        /* Read response data/body */
        /* Note:
                moreFlag will be set to 1 by HTTPCli_readResponseBody() call, if more
                data is available Or in other words content length > length of buffer.
                The remaining data will be read in subsequent call to HTTPCli_readResponseBody().
                Please refer HTTP Client Libary API documenation @ref HTTPCli_readResponseBody
                for more information.
        */
        HTTPCli_readResponseBody(httpClient, buf, sizeof(buf) - 1, &moreFlag);
        ASSERT_ON_ERROR(len);

        if ((len - 2) >= 0 && buf[len - 2] == '\r' && buf [len - 1] == '\n'){
            break;
        }

        if(!moreFlag)
        {
            /* There no more data. break the loop. */
            break;
        }
    }
    return 0;
}

/*!
    \brief This function read respose from server and dump on console

    \param[in]      httpClient - HTTP Client object

    \return         0 on success else -ve

    \note

    \warning
*/
static int readResponse(HTTPCli_Handle httpClient)
{
	long lRetVal = 0;
	int bytesRead = 0;
	int id = 0;
	int json = 0;
	bool moreFlags = 0;

	const char *ids[2] = {
			                HTTPCli_FIELD_NAME_CONNECTION,
			                NULL
	                     };
	/* Read HTTP POST request status code */
	lRetVal = HTTPCli_getResponseStatus(httpClient);
	if(lRetVal > 0)
	{
		switch(lRetVal)
		{
		case 200:
		{
			//UART_PRINT("HTTP Status 200\n\r");
			HTTPCli_setResponseFields(httpClient, (const char **)ids);
			MAP_UtilsDelay(10000);

			while((id = HTTPCli_getResponseField(httpClient, getBuff, sizeof(getBuff), &moreFlags))
					!= HTTPCli_FIELD_ID_END)
			{
				switch(id)
				{
					case 0: /* HTTPCli_FIELD_NAME_CONNECTION */
					{
					}
					break;
					default:
					{
						UART_PRINT("Wrong filter id\n\r");
						lRetVal = -1;
						return lRetVal;;
					}
				}
			}

			while (1)
			{
				bytesRead = HTTPCli_readResponseBody(httpClient, getBuff, sizeof(getBuff)-1, &moreFlags);
				if(bytesRead < 0)
				{
					UART_PRINT("Failed to received response body\n\r");
					lRetVal = bytesRead;
					return lRetVal;;
				}
				getBuff[bytesRead] = '\0';

				//UART_PRINT("readResponseBody bytesRead = %d\n\r", bytesRead);
				//UART_PRINT("bytesRead = %d readResponseBody:\n\r%s\n\r", bytesRead, getBuff);

				if(strstr((const char *)getBuff, LED_CTRL_TYPE) != NULL)
				{
					UART_PRINT("readResponseBody getBuff:\n\r%s\n\r", getBuff);
					if(g_ledStatus == 0 && NULL != strstr((const char *)getBuff, "ledswitch={1}"))
					{
						g_ledStatus = 1;
						GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
					}else if(g_ledStatus == 1 && NULL != strstr((const char *)getBuff, "ledswitch={0}"))
					{
						g_ledStatus = 0;
						GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
					}
				}

		        if(!moreFlags)
		            break;
			}
		}
		break;

		case 404:
			UART_PRINT("File not found. \r\n");
		default:
			UART_PRINT("default: FlushHTTPResponse. \r\n");
			FlushHTTPResponse(httpClient);
			break;
		}
	}
	else
	{
		UART_PRINT("Failed to receive data from server.\r\n");
		return lRetVal;;
	}

	lRetVal = 0;
    return lRetVal;
}

int HTTPGetMethod(HTTPCli_Handle httpClient)
{

    long lRetVal = 0;
    bool moreFlags = 0;
    HTTPCli_Field fields[4] = {
                                {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                {HTTPCli_FIELD_NAME_CONTENT_LENGTH, "0"},
                                {NULL, NULL}
                            };

    HTTPCli_setRequestFields(httpClient, fields);

    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_GET, GET_REQUEST_URI, moreFlags);
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP GET request.\n\r");
        return lRetVal;
    }

    lRetVal = readResponse(httpClient);

    return lRetVal;
}

//*****************************************************************************
//
//! Function to connect to HTTP server
//!
//! \param  httpClient - Pointer to HTTP Client instance
//!
//! \return Error-code or SUCCESS
//!
//*****************************************************************************
int ConnectToHTTPServer(HTTPCli_Handle httpClient)
{
    long lRetVal = -1;
    struct sockaddr_in addr;

#ifdef USE_PROXY
    struct sockaddr_in paddr;
    paddr.sin_family = AF_INET;
    paddr.sin_port = htons(PROXY_PORT);
    paddr.sin_addr.s_addr = sl_Htonl(PROXY_IP);
    HTTPCli_setProxy((struct sockaddr *)&paddr);
#endif

    /* Resolve HOST NAME/IP */
    lRetVal = sl_NetAppDnsGetHostByName((signed char *)HOST_NAME,
                                          strlen((const char *)HOST_NAME),
                                          &g_ulDestinationIP,SL_AF_INET);
    if(lRetVal < 0)
    {
        ASSERT_ON_ERROR(GET_HOST_IP_FAILED);
    }
    UART_PRINT("g_ulDestinationIP = %ld\r\n", g_ulDestinationIP);

    /* Set up the input parameters for HTTP Connection */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(HOST_PORT);
    addr.sin_addr.s_addr = sl_Htonl(g_ulDestinationIP);

    /* Testing HTTPCli open call: handle, address params only */
    HTTPCli_construct(httpClient);
    lRetVal = HTTPCli_connect(httpClient, (struct sockaddr *)&addr, 0, NULL);
    if (lRetVal < 0)
    {
        ASSERT_ON_ERROR(SERVER_CONNECTION_FAILED);
    }
    else
    {
        UART_PRINT("Connection to server created successfully\r\n");
    }

    return 0;
}

void DisConnectToHTTPServer(HTTPCli_Handle httpClient)
{
    UART_PRINT("DisConnectToHTTPServer --------- 1\r\n");
    HTTPCli_disconnect(httpClient);
    UART_PRINT("DisConnectToHTTPServer --------- 2\r\n");
    HTTPCli_destruct(httpClient);
    UART_PRINT("DisConnectToHTTPServer --------- 3\r\n");
}
