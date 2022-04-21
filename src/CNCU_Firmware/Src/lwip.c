/**
 ******************************************************************************
  * File Name          : LWIP.c
  * Description        : This file provides initialization code for LWIP
  *                      middleWare.
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2018 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
  
/* Includes ------------------------------------------------------------------*/
#include "lwip.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#if defined ( __CC_ARM )  /* MDK ARM Compiler */
#include "lwip/sio.h"
#endif /* MDK ARM Compiler */

#include "main.h"
#define DEBUG_MSG DEBUG_LWIP
#include "dbgmsg.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* ETH Variables initialization ----------------------------------------------*/
void _Error_Handler(char * file, int line);

/* Variables Initialization */
struct netif gnetif;
ip4_addr_t ipaddr;
ip4_addr_t netmask;
ip4_addr_t gw;


void link_changed (struct netif *netif)
{
	dbgmsg("PHY Link is changed\r\n");
}
/**
  * LwIP initialization function
  */
uint8_t MX_LWIP_Init(init_t type)
{
	/* Initilialize the LwIP stack with RTOS */
	tcpip_init( NULL, NULL );
	
	/* IP addresses initialization without DHCP (IPv4) */
	if (type == _static)
	{	
		/* IP addresses initialization */ 
		uint8_t IP_ADDRESS[4];
		uint8_t NETMASK_ADDRESS[4];
		uint8_t GATEWAY_ADDRESS[4];
		memcpy(IP_ADDRESS, (uint8_t*)&CNCU.Settings.myIP, 4);
		NETMASK_ADDRESS[0] = 255;
		NETMASK_ADDRESS[1] = 255;
		NETMASK_ADDRESS[2] = 255;
		NETMASK_ADDRESS[3] = 0;
		GATEWAY_ADDRESS[0] = 192;
		GATEWAY_ADDRESS[1] = 168;
		GATEWAY_ADDRESS[2] = 0;
		GATEWAY_ADDRESS[3] = 1; 
		
		IP4_ADDR(&ipaddr, IP_ADDRESS[0], IP_ADDRESS[1], IP_ADDRESS[2], IP_ADDRESS[3]);
		IP4_ADDR(&netmask, NETMASK_ADDRESS[0], NETMASK_ADDRESS[1] , NETMASK_ADDRESS[2], NETMASK_ADDRESS[3]);
		IP4_ADDR(&gw, GATEWAY_ADDRESS[0], GATEWAY_ADDRESS[1], GATEWAY_ADDRESS[2], GATEWAY_ADDRESS[3]);
	}
	else
	{
		/* IP addresses initialization with DHCP (IPv4) */
		ipaddr.addr = 0;
		netmask.addr = 0;
		gw.addr = 0;
	}

  /* add the network interface (IPv4/IPv6) with RTOS */
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

  /* Registers the default network interface */
  netif_set_default(&gnetif);

	netif_set_link_callback(&gnetif, link_changed);
	
  if (netif_is_link_up(&gnetif))
  {
    /* When the netif is fully configured this function must be called */
    netif_set_up(&gnetif);
		if (type == _static) return 0;
  }
  else
  {
		errmsg("PHY Link is Down\r\n");
    /* When the netif link is down this function must be called */
    netif_set_down(&gnetif);
		return 1;
  }
	
  /* Start DHCP negotiation for a network interface (IPv4) */
	if (type == _dhcp) dhcp_start(&gnetif);
	return 0;
}

void MX_LWIP_SetIP (uint32_t newIP)
{
	/* IP addresses initialization */ 
	uint8_t IP_ADDRESS[4];
	uint8_t NETMASK_ADDRESS[4];
	uint8_t GATEWAY_ADDRESS[4];
	memcpy(IP_ADDRESS, (uint8_t*)&newIP, 4);
	NETMASK_ADDRESS[0] = 255;
	NETMASK_ADDRESS[1] = 255;
	NETMASK_ADDRESS[2] = 255;
	NETMASK_ADDRESS[3] = 0;
	GATEWAY_ADDRESS[0] = 192;
	GATEWAY_ADDRESS[1] = 168;
	GATEWAY_ADDRESS[2] = 0;
	GATEWAY_ADDRESS[3] = 1; 
	
	IP4_ADDR(&ipaddr, IP_ADDRESS[0], IP_ADDRESS[1], IP_ADDRESS[2], IP_ADDRESS[3]);
	IP4_ADDR(&netmask, NETMASK_ADDRESS[0], NETMASK_ADDRESS[1] , NETMASK_ADDRESS[2], NETMASK_ADDRESS[3]);
	IP4_ADDR(&gw, GATEWAY_ADDRESS[0], GATEWAY_ADDRESS[1], GATEWAY_ADDRESS[2], GATEWAY_ADDRESS[3]);
	
	netif_set_down(&gnetif);
	
	netif_set_addr(&gnetif, &ipaddr, &netmask,&gw );
	
	if (netif_is_link_up(&gnetif))
  {
		dbgmsg("PHY Link is Up\r\n");
    /* When the netif is fully configured this function must be called */
    netif_set_up(&gnetif);
  }
}

#ifdef USE_OBSOLETE_USER_CODE_SECTION_4
/* Kept to help code migration. (See new 4_1, 4_2... sections) */
/* Avoid to use this user section which will become obsolete. */
/* USER CODE BEGIN 4 */
/* USER CODE END 4 */
#endif

#if defined ( __CC_ARM )  /* MDK ARM Compiler */
/**
 * Opens a serial device for communication.
 *
 * @param devnum device number
 * @return handle to serial device if successful, NULL otherwise
 */
sio_fd_t sio_open(u8_t devnum)
{
  sio_fd_t sd;

  sd = 0; // dummy code
	
  return sd;
}

/**
 * Sends a single character to the serial device.
 *
 * @param c character to send
 * @param fd serial device handle
 *
 * @note This function will block until the character can be sent.
 */
void sio_send(u8_t c, sio_fd_t fd)
{

}

/**
 * Reads from the serial device.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received - may be 0 if aborted by sio_read_abort
 *
 * @note This function will block until data can be received. The blocking
 * can be cancelled by calling sio_read_abort().
 */
u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t recved_bytes;

  recved_bytes = 0; // dummy code

  return recved_bytes;
}

/**
 * Tries to read from the serial device. Same as sio_read but returns
 * immediately if no data is available and never blocks.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received
 */
u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t recved_bytes;

  recved_bytes = 0; // dummy code
	
  return recved_bytes;
}
#endif /* MDK ARM Compiler */

/*************************** END OF FILE **************************************/
