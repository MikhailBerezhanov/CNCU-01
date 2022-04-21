
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  *
  *
  *
  *
  *
  ******************************************************************************
  */
#pragma diag_warning 188

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "stm32f4xx_hal.h"
#include "lwip.h"
#include "api.h"
#include "httpserver.h"
#include "MK_Init.h"
#include "leds.h"
#include "dswitch.h"
#include "rs232.h"
#include "m95_eeprom.h"
#include "rtc.h"
#include "gtimer.h"
#include "can.h"
#include "spi.h"
#include "sdcard.h"
#include "i2c.h"
#include "uart.h"
#include "flash.h"
#include "buzz.h"
#include "main.h"
#include "cncu_conf.h"

#define DEBUG_MSG DEBUG_MAIN
#include "dbgmsg.h"

#define TEST_FILE_PTH "style.css"

static char file_buf[1];

/* Private variables ---------------------------------------------------------*/ 
extern ETH_HandleTypeDef heth;
volatile uint32_t dhcp_timer = TMOUT_DHCP;
TDevStatus CNCU;							// Status of device
TStates St;

xTaskHandle HTTPServerTaskHandle = NULL;
xTaskHandle netTaskHandle = NULL;
xTaskHandle CANtoTCPTaskHandle = NULL;
xTaskHandle TCPtoCANTaskHandle = NULL;
xTaskHandle PingTaskHandle = NULL;
xSemaphoreHandle USARTDebugMutex;
osMessageQId SDStatusQid;

CANframe_t CANFrame;

uint8_t FILE_LIST_BUF[2048];

/* Private function prototypes -----------------------------------------------*/
// Tasks
void MainTask (void *param);				// Initializes LWip and creates
void TCPtoCANTask (void *param);
void CANtoTCPTask (void *param);
void TCPClientTask (void *param);
void HTTPServerTask (void *param);			
void netTask (void *param);					// Created when incomming connection is detected
void CheckLinkTask(void *param);
void PingTask(void *param);

// Functions
static _Bool check_link_status(void);	// 1 - Link is UP, 0 - Down
void vApplicationIdleHook(void);
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName);
void SD_testread(_Bool mutual);

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
	MK_GPIO_Init();
	CNCU.Periph.LEDSstatus = Leds_Init();
	Leds_StartBlink(GEN_LED, 500);
	
	/* Timers */
	//CNCU.Periph.GTIMstatus = Gtimer_Init(ms_resolution);	
	CNCU.Periph.RTCstatus = RTC_Init();
	
	/* Communicational interfaces */
	CNCU.Periph.USART1status = USART1_Init(921600);		// Console
	CNCU.Periph.USART2status = RS232_Init(115200);
	CNCU.Periph.USART3status = USART3_Init(921600);
	CNCU.Periph.I2Cstatus = I2C_Init();				// TODO: add CLK freq as param
	CNCU.Config.CAN_SPEED = CAN_Init(CAN_BAUD);
	if (CNCU.Config.CAN_SPEED <= 0) CNCU.Periph.CANstatus = INIT_ERR;
	else CNCU.Periph.CANstatus = CNCU_OK;
	
	Buzz_Init();
		
	/* External Memory */
	CNCU.Periph.SDstatus = SD_DMA_Init();

	CNCU.Periph.SPIstatus = SPI_Init(5e6);				// TODO: add CLK freq as param, place inside Drivers/CNCU modules
	if(!CNCU.Periph.SPIstatus)
	{
		CNCU.Periph.EEPROMstatus = M95EEPROM_Init();
		CNCU.Periph.FLASHstatus = FLASH_Init();
		
		uint8_t tmp[90] = {0};
#if 0		
		FLASH_ErasePage(4024);
		if (FLASH_ReadPage(4024, 0, tmp, sizeof(tmp)) == CNCU_OK)
			print_arr("FLASH page :", tmp, sizeof(tmp));
		
		memset(tmp, 0xAA, sizeof(tmp));
		FLASH_WriteBytes(4024, 0, tmp, sizeof(tmp)/2);
		
		memset(tmp, 0x43, sizeof(tmp));
		FLASH_ReadModifyWrite(4024, 0, tmp, 5); 
		
		if (FLASH_ReadPage(4024, 0, tmp, sizeof(tmp)) == CNCU_OK)
			print_arr("FLASH page :", tmp, sizeof(tmp));
#endif
	}
	
	/* Configurations */
  CNCU.Config.DSWTCH = Get_DSwitch();
	ip4addr_aton(MY_DEFAULT_IP, (ip_addr_t*)&CNCU.Settings.myIP);
	ip4addr_aton(SERV_DEFAULT_IP, (ip_addr_t*)&CNCU.Settings.servIP);
	CNCU.Settings.servPort = SERV_DEFAULT_PORT;
	St.proc = GET_SETTINGS;
	St.link = DOWN;

	RTC_TimeTypeDef rtc_time;		
	RTC_DateTypeDef rtc_date;
	if (!CNCU.Periph.RTCstatus)
	{
		//RTC_Set_Time(9, 32, 30);			//set time (hr,min,sec)
		//RTC_Set_Date(27, RTC_WEEKDAY_TUESDAY, RTC_MONTH_AUGUST, 2019);
		
		RTC_Get_Time(&rtc_time);		
		RTC_Get_Date(&rtc_date);
	}
	
	/* WDT */
	MK_IWDG_Init(1000);	// 1 sec 			
	MK_IWDG_Reset();
	
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
	dbgmsg("Date: %02d.%02d.%04d, Time: %02d.%02d.%02d\r\n", rtc_date.Date, rtc_date.Month, 1980+rtc_date.Year,
		rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds);
	resmsg(CNCU.Periph.SDstatus, "microSD status:\t\t");
	if(CNCU.Periph.SDstatus == CNCU_OK)
	{
		//sd_print_info();	
		//sd_dir_files_list(NULL, FILE_LIST_BUF, sizeof(FILE_LIST_BUF), NULL);		//search root (NULL) directory of SD card for any files
		//dbgmsg("Files:\r\n%s", FILE_LIST_BUF);
		//dbgmsg("\r\n");

		//SD SEQUENTIAL READ EXAMPLE(LARGE FILES):
#if 0	
		SD_testread(0);
#endif
	}
	resmsg(CNCU.Periph.USART2status, "RS232 status:\t\t");
	resmsg(CNCU.Periph.USART3status, "UART status:\t\t");
	resmsg(CNCU.Periph.SPIstatus, "SPI status:\t\t");
	resmsg(CNCU.Periph.EEPROMstatus, "EEPROM status:\t\t");
	resmsg(CNCU.Periph.FLASHstatus, "FLASH status:\t\t");
	dbgmsg("CAN baudrate:\t\t%d [bps]\r\n", CNCU.Config.CAN_SPEED);
	resmsg(CNCU.Periph.CANstatus, "CAN status:\t\t");
	resmsg(CNCU.Periph.I2Cstatus, "I2C status:\t\t");
	//resmsg(CNCU.Periph.GTIMstatus, "GTIM status:\t\t");
	dbgmsg("\r\n===============================================\r\n");	
		
	/* Create the thread(s) */
	xTaskCreate(MainTask, "MainTask", 512, NULL, 3, NULL); 
	xTaskCreate(HTTPServerTask, "HTTPServerTask", 512, NULL, 2, &HTTPServerTaskHandle);
	xTaskCreate(CheckLinkTask, "CheckLinkTask", 128, NULL, 1, NULL);
	//xTaskCreate(TCPClientTask, "TCPClientTask", 1024, NULL, 1, NULL);	
	//xTaskCreate(TCPtoCANTask, "TCPtoCANTask", 256, NULL, 1, NULL);
	//xTaskCreate(CANtoTCPTask, "CANtoTCPTask", 128, NULL, 1, NULL);
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
	ip4addr_aton("192.168.0.111", (ip_addr_t*)&newIP);
	MX_LWIP_SetIP(newIP);
	m_dbgmsg("New IP set: %s\r\n", ip4addr_ntoa(&gnetif.ip_addr));
	
	St.link = UP;
	St.proc = CONNECTING;

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
		
	//Buzz_PlayMario();

	if(sd_mount() != FR_OK)
	{
		m_errmsg("(-) SD Mount failed\r\n");
	}
	m_dbgmsg("(+) SD Mount done\r\n");
#if 0	
	uint32_t fcnt = 1;
	uint32_t bytes_read = 0;
	
	for (;;)
	{			
		bytes_read = 0;
		res = FR_OK;
		while(res != EOF)
		{
			if (res != FR_OK) 
			{
				m_dbgmsg("Error seq_read file\r\n");
				break;
			}
			res = sd_f_seq_read("/style.css", file_buf, sizeof(file_buf), &bytes_read); 		
		}
		if(res == EOF) m_dbgmsg("Success seq_read files: %d\r\n", fcnt++);

		osDelay(30);
	}
#endif
	// Main loop
	for (;;)
	{

		switch(St.proc)
		{
			case CONNECTING: 
				m_dbgmsg("Connecting to Work Server...\r\n");
				//Leds_StartRunningBlink();
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
						St.proc = CONNECTED;
						break;
					}				
					osDelay(50);		// Try again every 50 milliseconds
				}
			break;
			
			case CONNECTED:
				Leds_StopRunningBlink();
				Leds_ON(GEN_LED);
				St.serv = ONLINE;
				m_dbgmsg("> Connected! Starting TCP and CAN routine.\r\n");
			  if (!PingTaskHandle) xTaskCreate(PingTask, "PingTask", 128, nc, 1, &PingTaskHandle);
				if (!PingTaskHandle) m_errmsg("Unable to create PingTask\r\n");

				for (;;) 
				{
					// Polling PHY link and Server online status
					if (St.link == DOWN || St.serv == OFFLINE)
					{	
						St.proc = CONNECTING;
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
							St.serv = OFFLINE;
							St.proc = CONNECTING;
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

// Check PHY connection
void CheckLinkTask(void *param)
{	
	if (CNCU.Periph.ETHstatus == CNCU_OK)
	{
		for (;;)
		{	
			if (!check_link_status())
			{
				m_dbgmsg(_RED"PHY Link is Down\r\n"_RESET);
				St.link = DOWN;
				Leds_ON(ERR_LED);
			}
			else if (St.link == DOWN)
			{
				St.link = UP;
				Leds_OFF(ERR_LED);
			}
			
			osDelay(1000);
		}
	}
}

void TCPtoCANTask (void *param)
{
	vTaskSuspend( NULL );
	
	m_dbgmsg("TCPtoCANTask starts\r\n");
	
	//struct netconn *nc = (struct netconn *) param;
	
	//netconn_close(nc);
	//netconn_delete(nc);
}

void CANtoTCPTask (void *param)
{
	vTaskSuspend( NULL );
}


void TCPClientTask (void *param)
{
	struct netconn *nc;
	struct netbuf *nb;
	volatile err_t res;
	uint16_t len;
	ip_addr_t myIP;
	ip_addr_t servIP;
	char *buf = pvPortMalloc(2048);
	
	/*MX_LWIP_Init(_dhcp);
	dbgmsg("LWiP initialized. Getting IP with DHCP");
	while(gnetif.ip_addr.addr == 0) osDelay(1);
	dbgmsg("\r\nIP: %s\r\n", ip4addr_ntoa(&gnetif.ip_addr));
	*/
	myIP = gnetif.ip_addr;
	ip4addr_aton("192.168.3.166", &servIP);
	
	nc = netconn_new(NETCONN_TCP);
	if (nc == NULL)
	{
		errmsg("netconn_new error %d\r\n",res);
		while(1) vTaskDelay (1 / portTICK_PERIOD_MS);
	}
	
	res = netconn_bind(nc, &myIP, 0);
	if (res != 0)
	{
		errmsg("netconn_bind error %d\r\n",res);
		while(1) vTaskDelay (1 / portTICK_PERIOD_MS);
	}	
	
	for (;;)
	{
		res = netconn_connect(nc, &servIP, 3333);
		if (res != 0)
		{
			errmsg("netconn_connect error %d\r\n",res);
			while(1) vTaskDelay (1 / portTICK_PERIOD_MS);
		}	
		
		dbgmsg("Connected!\r\n");
		
		sprintf(buf, "Helllo\r\n");
		res = netconn_write(nc, buf, strlen(buf), NETCONN_COPY);
		if (res != 0)
		{
			errmsg("netconn_wrute error %d\r\n",res);
			while(1) vTaskDelay (1 / portTICK_PERIOD_MS);
		}
		
		dbgmsg("Sended\r\n");
/*		
		res = netconn_recv(nc, &nb);
		if (res != 0)
		{
			dbgmsg("netconn_recv error %d\r\n",res);
			while(1) vTaskDelay (1 / portTICK_PERIOD_MS);
		}
		
		len = netbuf_len(nb);
		netbuf_copy(nb, buf, len);
		netbuf_delete(nb);
		buf[len] = 0;
		dbgmsg("Received %d bytes:\r\n%s\r\n", len, buf);
*/		
		//netconn_close(nc);
		//netconn_delete(nc);	
		
		dbgmsg("Closed!\r\n");
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
	while (St.link != UP) osDelay(1);
	
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
	
  // Creating queue for SD_hardware monitoring
	osMessageQDef(SDStatusQ, 5, uint8_t);
	SDStatusQid = osMessageCreate(osMessageQ(SDStatusQ), NULL);
	if (NULL == SDStatusQid) m_errmsg("SDStatusQueue creation error\r\n");
	
	m_dbgmsg("Web Server listening..\r\n");
	
  for(;;)	
  {
	/* Block until we get an incoming connection */
    res = netconn_accept(nc, &in_nc);
		if ((res != 0)) 
		{
			if (res == ERR_TIMEOUT) osDelay(10);
			else m_errmsg("netconn_accept error %d\r\n",res);
		}
		else
		{
			m_dbgmsg("Connection Accepted\r\n");
			
			/* HTTP server connection */
			http_server_serve(in_nc);
				
			/* delete connection */
			netconn_delete(in_nc);
			
			//osDelay(210);
			//dbgmsg("Deleted\r\n");	
		/* definition and creation of netTask to work with new connection */
			//xTaskCreate(netTask, "netTask", 512, (void*)in_nc, 0, netTaskHandle);

		}
  }
}

/** StartNetTask function 
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void netTask (void *pvParameters)
{
	// создрать mutex дл€ данных читаемых текущим потоком
	struct netconn *in_nc = (struct netconn *) pvParameters;
	//struct netbuf *nb;
	//volatile err_t res;
	//uint16_t len;	
	//char *buffer = pvPortMalloc(256);
	
	dbgmsg("Incomming connection\r\n");
	//sprintf (buffer, "Hello from STM32\r\n");
	// Send data to TCP connection 
	//netconn_write(in_nc, buffer, strlen(buffer), NETCONN_COPY);

	for(;;)
	{
// !!!! при разрыве соединени€ надо закрыть задачу, чтобы не было утечек пам€ти !!!!
		/* HTTP server connection */
		http_server_serve(in_nc);
			
		/* delete connection */
		netconn_delete(in_nc);
		/*	
		res = netconn_recv(nc, &nb);
		if (res != 0)
		{
		 
			dbgmsg("netconn_recv error %d\r\n",res);
			while(1) vTaskDelay (1 / portTICK_PERIOD_MS);
		}	
		len = netbuf_len(nb);
		netbuf_copy(nb,buffer,len);
		netbuf_delete(nb);
		buffer[len] = 0;
		dbgmsg("%s",buffer);
		*/		
		osThreadTerminate(NULL);	
	}
}

void SD_SatusWatcher (void* param)
{
	// Block at queue
	dbgmsg(" Trying to reinit SD driver..\r\n");
	HAL_SD_MspDeInit(&hsd);
	SD_DMA_Init();
	// Resume SD routine 
}


/* Ёта функци€ хука ничего не делает, кроме инкрементировани€ счетчика. */
unsigned long ulIdleCycleCount = 0UL;
void vApplicationIdleHook(void)
{
	//dbgmsg("IDLE\r\n");
  ulIdleCycleCount++;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
	m_errmsg("%s stck overflow\r\n", pcTaskName);
}

void SD_testread (_Bool mutual)
{
#if 0
	uint32_t bytes_read = 0;				//how much data of the file already read (nothing yet so 0)
	uint32_t fsize = sd_f_get_size(TEST_FILE_PTH);
	if(mutual){m_dbgmsg("File '%s' size: %d\r\n", TEST_FILE_PTH, fsize);}
	else {dbgmsg("File '%s' size: %d\r\n", TEST_FILE_PTH, fsize);}
	int res = 0;										//operation result variable (0 - OK, (-1) - EOF, others - error)
	while( bytes_read < fsize/*res!=EOF*/)	//keep reading until the whole file read
	{
		MK_IWDG_Reset();
		
		res	= sd_f_seq_read(TEST_FILE_PTH, FILE_LIST_BUF, sizeof(FILE_LIST_BUF), &bytes_read);		//read to buffer
		if(res<=FR_OK)
		{
			if(mutual) {m_dbgmsg("%s", FILE_LIST_BUF);}
			else {dbgmsg("%s", FILE_LIST_BUF);}
			
			memset(FILE_LIST_BUF, '\0', sizeof(FILE_LIST_BUF));
		}				
		else 
		{
			if(mutual) {m_errmsg("sd_f_seq_read() error: %d", res);	}
			else errmsg("sd_f_seq_read() error: %d", res);
			break;
		}				
	}
	if(mutual) {m_dbgmsg("\r\n");}
	else dbgmsg("\r\n");
#endif
}

/***************************** END OF FILE ************************************/
