
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  *						Ethernet to CAN converter
  * @author			: ClownMik (b.poposhka.m@gmail.com)
  ******************************************************************************
  */
#pragma diag_warning 188

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "main.h"
#include "MK_Init.h"
#include "leds.h"
#include "dswitch.h"
#include "rs232.h"
#include "m95_eeprom.h"
#include "stm32_rtc.h"
#include "gtimer.h"
#include "can.h"
#include "spi.h"
#include "sdcard.h"
#include "i2c.h"
#include "uart.h"
//#include "flash.h"
#include "stm32f4xx_hal.h"
#include "lwip.h"
#include "api.h"
#include "httpserver_netconn.h"

#define DEBUG_MSG DEBUG_MAIN
#include "dbgmsg.h"

// Mutual debug print. Use in tasks!
#define m_dbgmsg(args...) do{ \
	xSemaphoreTake(USARTDebugMutex, portMAX_DELAY); \
	dbgmsg(args); \
	xSemaphoreGive(USARTDebugMutex); \
} while(0);

#define m_errmsg(args...) do{ \
	xSemaphoreTake(USARTDebugMutex, portMAX_DELAY); \
	errmsg(args); \
	xSemaphoreGive(USARTDebugMutex); \
} while(0);	

/* Private variables ---------------------------------------------------------*/ 
extern ETH_HandleTypeDef heth;
volatile uint32_t dhcp_timer = TMOUT_DHCP;
TDevStatus CNCU;							// Status of device

xTaskHandle HTTPServerTaskHandle = NULL;
xTaskHandle netTaskHandle = NULL;
xTaskHandle CANtoTCPTaskHandle = NULL;
xTaskHandle TCPtoCANTaskHandle = NULL;
xTaskHandle PingTaskHandle = NULL;
xSemaphoreHandle USARTDebugMutex;

CANframe_t CANFrame;

uint8_t FILE_LIST_BUF[1024];

/* Private function prototypes -----------------------------------------------*/
// Tasks
void MainTask (void *param);				// Initializes LWip and creates
void HTTPServerTask (void *param);			
void CheckLinkTask(void *param);
void PingTask(void *param);

// Functions
static _Bool check_link_status(void);	// 1 - Link is UP, 0 - Down
void vApplicationIdleHook(void);

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();	
	memset(&CNCU, 0xFF, sizeof(TDevStatus));
	
	/* Configure the system clock */
	CNCU.Periph.SysClockstatus = MK_SysClock_Config();
	CNCU.Config.HCLK = HAL_RCC_GetHCLKFreq();
	CNCU.Config.PCLK1 = HAL_RCC_GetPCLK1Freq();
	CNCU.Config.PCLK2 = HAL_RCC_GetPCLK2Freq();
	
	/* Initialize all peripherals */
	MK_GPIO_Init();
	CNCU.Periph.LEDSstatus = Leds_Init();
	Leds_StartBlink(GEN_LED, 500);
	CNCU.Config.DSWTCH = Get_DSwitch();
	CNCU.Periph.USART1status = USART1_Init(921600);
	CNCU.Periph.USART2status = RS232_Init(115200);
	CNCU.Periph.USART3status = USART3_Init(921600);	
	CNCU.Periph.SPI1status = SPI_Init(eSPI1);	// TODO: add SPI CLK freq as param
	if(!CNCU.Periph.SPI1status)
	{
		M95EEPROM_Init();
		//FLASH_Init();
	}
	
	CNCU.Periph.RTCstatus = RTC_Init();
	CNCU.Periph.SDstatus = SD_CARD_Init();
	
	CNCU.Config.CAN_SPEED = CAN_Init(100000);
	
	if (CNCU.Config.CAN_SPEED <= 0) CNCU.Periph.CAN1status = 1;
	else CNCU.Periph.CAN1status = 0;
	
	//CNCU.Periph.I2C3status = I2C_Init();		// TODO: Board v.2 uses I2C1 instead of I2C3
	/* Timers */
	CNCU.Periph.GTIMstatus = Gtimer_Init(ms_resolution);
	Gtimer_Start();	

	MK_IWDG_Init(1000);	// 1 sec 			
	MK_IWDG_Reset();
	
	ip4addr_aton(MY_DEFAULT_IP, (ip_addr_t*)&CNCU.Settings.myIP);
	ip4addr_aton(SERV_DEFAULT_IP, (ip_addr_t*)&CNCU.Settings.servIP);
	CNCU.Settings.servPort = SERV_DEFAULT_PORT;
	CNCU.States.proc = GET_SETTINGS;
	CNCU.States.link = DOWN;

	RTC_TimeTypeDef rtc_time;		
	RTC_DateTypeDef rtc_date;
	if (!CNCU.Periph.RTCstatus)
	{
		//RTC_Set_Time(12, 28, 6);			//set time (hr,min,sec)
		//RTC_Set_Date(8, RTC_WEEKDAY_THURSDAY, RTC_MONTH_AUGUST, 2019);
		
		RTC_Get_Time(&rtc_time);		
		RTC_Get_Date(&rtc_date);
	}
	
	dbgmsg("\r\n=============== CNCU DEBUG_MODE ===============\r\n");
	dbgmsg("HCLK_value:\t\t%d [Hz]\r\n", CNCU.Config.HCLK);
	dbgmsg("PCLK1_value:\t\t%d [Hz]\r\n", CNCU.Config.PCLK1);
	dbgmsg("PCLK2_value:\t\t%d [Hz]\r\n", CNCU.Config.PCLK2);
	dbgmsg("APB1 Prescaler:\t\t%d\r\n", APBPrescTable[(RCC->CFGR & RCC_CFGR_PPRE1)>>RCC_CFGR_PPRE1_Pos]);
	dbgmsg("APB2 Prescaler:\t\t%d\r\n", APBPrescTable[(RCC->CFGR & RCC_CFGR_PPRE2)>>RCC_CFGR_PPRE2_Pos]);
	dbgmsg("DIPSWITCH_value:\t%d\r\n", CNCU.Config.DSWTCH);
	resmsg(CNCU.Periph.SysClockstatus, "SysClock status:\t");
	resmsg(CNCU.Periph.USART1status, "USART1 status:\t\t");
	resmsg(CNCU.Periph.LEDSstatus, "LEDS status:\t\t");
	resmsg(CNCU.Periph.RTCstatus, "RTC status:\t\t");
	dbgmsg("Date: %02d.%02d.%04d, Time: %02d.%02d.%02d\r\n", rtc_date.Date, rtc_date.Month, 1980+rtc_date.Year, 																									rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds);
	resmsg(CNCU.Periph.SDstatus, "microSD status:\t\t");
	if(!CNCU.Periph.SDstatus) sd_print_info();
	resmsg(CNCU.Periph.USART2status, "RS232 status:\t\t");
	resmsg(CNCU.Periph.USART3status, "UART status:\t\t");
	resmsg(CNCU.Periph.SPI1status, "SPI status:\t\t");
	resmsg(CNCU.Periph.SPI1status, "EEPROM status:\t\t");
	dbgmsg("CAN baudrate:\t\t%d [bps]\r\n", CNCU.Config.CAN_SPEED);
	resmsg(CNCU.Periph.CAN1status, "CAN status:\t\t");
	resmsg(CNCU.Periph.I2C3status, "I2C status:\t\t");
	resmsg(CNCU.Periph.GTIMstatus, "GTIM status:\t\t");
	dbgmsg("\r\n===============================================\r\n");
	
	/* Create the thread(s) */
	xTaskCreate(MainTask, "MainTask", 1024, NULL, 2, NULL); 
	xTaskCreate(HTTPServerTask, "HTTPServerTask", 512, NULL, 2, &HTTPServerTaskHandle);
	xTaskCreate(CheckLinkTask, "CheckLinkTask", 128, NULL, 1, NULL);

	USARTDebugMutex = xSemaphoreCreateMutex();
	if (!USARTDebugMutex) errmsg("Unable to create USARTDebugMutex. Check FreeRTOS heap size.\r\n");
	
	/* Start scheduler */	
	vTaskStartScheduler();

	/* We should never get here as control is now taken by the scheduler */
	errmsg("RTOS: Scheduler can't start\r\n");
	NVIC_SystemReset();	 	
}

// CNCU Work Process
void MainTask (void *param)
{
	struct netconn *nc;
	volatile err_t res;
	
	// Inits tcpip stack, creates main TCPIP thread (st_size 1024, prio 6), queue and mutex 
	CNCU.Periph.ETHstatus = MX_LWIP_Init(_static);
	if (CNCU.Periph.ETHstatus)
	{
		m_errmsg("LWiP init failed. Rebooting...\r\n");
		NVIC_SystemReset();		 
	}
	m_dbgmsg("LWiP initialized.\r\nIP: %s\r\n", ip4addr_ntoa(&gnetif.ip_addr));
	if (heth.Init.DuplexMode == ETH_MODE_FULLDUPLEX) dbgmsg("ETH mode:\t\tFULLDUPLEX\r\n");
	else m_dbgmsg("ETH mode:\t\tHALFDUPLEX\r\n");
	if (heth.Init.Speed == ETH_SPEED_100M) dbgmsg("ETH speed:\t\t100Mbps\r\n");
	else m_dbgmsg("ETH speed:\t\t10Mbps\r\n");	
	uint32_t newIP;
	ip4addr_aton("192.168.0.11", (ip_addr_t*)&newIP);
	MX_LWIP_SetIP(newIP);
	m_dbgmsg("New IP set: %s\r\n", ip4addr_ntoa(&gnetif.ip_addr));
	
	CNCU.States.link = UP;
	CNCU.States.proc = CONNECTING;

	nc = netconn_new(NETCONN_TCP);
	if (nc == NULL)
	{
		m_errmsg("netconn_new error %d\r\n",res);
		while(1) osDelay(1);
	}	
	res = netconn_bind(nc, &gnetif.ip_addr, 0);
	if (res != 0)
	{
		m_errmsg("netconn_bind error %d\r\n",res);
		while(1) osDelay(1);
	}		
	
	// Main loop
	for (;;)
	{
		switch(CNCU.States.proc)
		{
			case CONNECTING: 
				m_dbgmsg("Connecting to Work Server...\r\n");
				Leds_StartRunningBlink();
				for(;;)
				{
					if (netconn_connect(nc, (ip_addr_t*)&CNCU.Settings.servIP, CNCU.Settings.servPort))
					{
						// If the connection could not be established, rebind the netconn-interface and try again
						netconn_close(nc);
						netconn_delete(nc);
						nc = netconn_new(NETCONN_TCP);
						if (nc == NULL)
						{
							m_errmsg("netconn_new error %d\r\n", res);
							while(1) osDelay(1);
						}	
						res = netconn_bind(nc, &gnetif.ip_addr, 0);
						if (res != 0)
						{
							m_errmsg("netconn_bind error %d\r\n", res);
							while(1) osDelay(1);
						}
					}
					else 
					{
						CNCU.States.proc = CONNECTED;
						break;
					}				
					osDelay(50);		// Try again every 50 milliseconds
				}
			break;
			
			case CONNECTED:
				Leds_StopRunningBlink();
				Leds_ON(GEN_LED);
				CNCU.States.serv = ONLINE;
				m_dbgmsg("> Connected! Starting TCP and CAN routine.\r\n");
			  if (!PingTaskHandle) xTaskCreate(PingTask, "PingTask", 128, nc, 1, &PingTaskHandle);
				if (!PingTaskHandle) m_errmsg("Unable to create PingTask\r\n");

				for (;;) 
				{
					// Polling PHY link and Server online status
					if (CNCU.States.link == DOWN || CNCU.States.serv == OFFLINE)
					{	
						CNCU.States.proc = CONNECTING;
						break;
					}
					
					// Main work
					
					// CAN frame came
					if (CAN_GetFrame(&CANFrame))
					{
						dbgmsg("Incoming CAN msg\r\n");
						dbgmsg("RTR:\t%d\r\n", CANFrame.RTR);
						dbgmsg("DLC:\t%d\r\n", CANFrame.DLC);
						dbgmsg("IDE:\t%d\r\n", CANFrame.IDE);
						dbgmsg("Header:\t0x%08X\r\n", CANFrame.Header);
						
						res = netconn_write(nc, &CANFrame, sizeof(CANframe_t), NETCONN_COPY);
						if (res != 0)
						{
							m_errmsg("netconn_write error %d\r\n",res);
							CNCU.States.serv = OFFLINE;
							CNCU.States.proc = CONNECTING;
							Leds_OFF(GEN_LED);
							break;
						}
					}
					// TCP packet came
					
					osDelay(100);
				}
			
			break;
			
			case WORK: break;

			default: break;
		}		
	}// Main loop
	
}

// Ping Work Server  (TODO: Use KeepAlive of smth else instead of polling)
void PingTask(void *param)
{
	struct netconn* n_c = (struct netconn*) param;
	char buf[80];
	err_t res;
	sprintf(buf, "Helllo\r\n");
	m_dbgmsg("Starting PingTask\r\n");
	
	for (;;)
	{
		/*
		if(CNCU.States.proc == CONNECTED)
		{
			res = netconn_write(n_c, buf, strlen(buf), NETCONN_COPY);
			if (res != 0)
			{
				m_errmsg("netconn_write error %d\r\n",res);
				CNCU.States.serv = OFFLINE;
				Leds_OFF(GEN_LED);
			}
    }
		 */
		osDelay(PING_SERV_PERIOD);
	}
}

// Check PHY connection
void CheckLinkTask(void *param)
{	
	if (CNCU.Periph.ETHstatus == 0)
	{
		for (;;)
		{	
			if (!check_link_status())
			{
				m_dbgmsg(_RED"PHY Link is Down\r\n"_RESET);
				CNCU.States.link = DOWN;
				Leds_ON(ERR_LED);
			}
			else if (CNCU.States.link == DOWN)
			{
				CNCU.States.link = UP;
				Leds_OFF(ERR_LED);
			}
			
			osDelay(1000);
		}
	}
}

/** StartDefaultTask function 
  * @brief  .
  * @param  *pvParameters - parameter of Task.
  * @retval None
  */
void HTTPServerTask (void *pvParameters)
{
	struct netconn *nc;					// New connection structure 
	struct netconn *in_nc;			// New incomming connection structure
	volatile err_t res;					// for detecting errors
	uint16_t port = 80;					// Port number (HTTP port)
	uint16_t cnt = 0;
	
  m_dbgmsg("HTTP Server. Waiting link UP \r\n");
	while (CNCU.States.link != UP) osDelay(1);
	
	m_dbgmsg("Starting HTTP Server\r\n");
	
	/* Create a connection structure */
	nc = netconn_new(NETCONN_TCP);
	if (nc == NULL)
	{
		m_errmsg("netconn_new error %d\r\n",res);
		while(1) osDelay(1);
	}
	
	/* Bind the connection to port 80 (HTTP) on any local IP address */
	res = netconn_bind(nc, IP_ADDR_ANY, port);
	if (res != 0)
	{
		m_errmsg("netconn_bind error %d\r\n",res);
		while(1) osDelay(1);
	}	

	/* Put the connection into LISTEN state */
	res = netconn_listen(nc);
	if (res != 0)
	{
		m_errmsg("netconn_listen error %d\r\n",res);
		while(1) osDelay(1);
	}
	
	m_dbgmsg("Web Server listening..\r\n");
	
  for(;;)	
  {
	/* Block until we get an incoming connection */
    res = netconn_accept(nc, &in_nc);
		if (res != 0) 
		{
			m_errmsg("netconn_accept error %d\r\n",res);
		}
		else
		{	
			/* HTTP server connection */
			http_server_serve(in_nc);
				
			/* delete connection */
			netconn_delete(in_nc);	
		}
  }
}

static _Bool check_link_status(void)
{
	static _Bool prev = 1;	// Up
	_Bool linked = 1;
	uint32_t phyreg = 0U;
	
	HAL_ETH_ReadPHYRegister(&heth, PHY_BSR, &phyreg);
      
  if(((phyreg & PHY_LINKED_STATUS) != PHY_LINKED_STATUS))
	{
		netif_set_link_down(&gnetif);
		netif_set_down(&gnetif);
		linked = 0;
	}
	else
	{
		netif_set_link_up(&gnetif);
		netif_set_up(&gnetif);
		linked = 1;
	}

	if (prev != linked) ethernetif_update_config(&gnetif);	
	prev = linked;
	
	return linked;
}

/***************************** END OF FILE ************************************/
