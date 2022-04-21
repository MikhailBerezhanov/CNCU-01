/**
  ******************************************************************************
  * @file           : can.c
  * @brief          : Module working with CNCU CAN 2.0B interface
  ******************************************************************************
*/
#include <string.h>
#include "FIFO.h"
#include "can.h"

#define	DEBUG_AUTOBAUD	1
#define	DEBUG_SELFTEST	0

#define DEBUG_MSG DEBUG_CAN
#include "dbgmsg.h"

FIFO_CREATE(CAN_fifo, CANframe_t, CAN_FIFO_SIZE);
CAN_HandleTypeDef xCAN1;

/* Private functions ---------------------------------------------------------*/
static uint32_t CAN_ConvertToBS1(uint8_t num);
static uint32_t CAN_ConvertToBS2(uint8_t num);
static uint32_t CAN_ConvertToSJW(uint8_t num);

/* Private functions (HAL_Init callback)--------------------------------------*/
void HAL_CAN_MspInit(CAN_HandleTypeDef* hcan)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  if(hcan->Instance==CAN1)
  {
    /* Peripheral clock enable */
    __HAL_RCC_CAN1_CLK_ENABLE();
  
    /**CAN1 GPIO Configuration    
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* hcan)
{
  if(hcan->Instance==CAN1)
  {
    /* Peripheral clock disable */
    __HAL_RCC_CAN1_CLK_DISABLE();
  
    /**CAN1 GPIO Configuration    
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX 
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11|GPIO_PIN_12);
  }
}


/**
	@brief: ISR - Valid CAN Frame detected in CAN_RX_FIFO0
					Store incomming CAN2.0B frames into application FIFO buffer 
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	CAN_RxHeaderTypeDef Rx_hdr;
	uint8_t Rx_data[8];
	CANframe_t Fr = {0};

	HAL_CAN_GetRxMessage(&xCAN1, CAN_RX_FIFO0, &Rx_hdr, Rx_data);

	// Copy received data into application
	Fr.Header = Rx_hdr.ExtId;
	memcpy(Fr.Data, Rx_data, Rx_hdr.DLC);
	Fr.DLC = Rx_hdr.DLC;
	Fr.IDE = Rx_hdr.IDE ? 1 : 0;
	Fr.RTR = Rx_hdr.RTR ? 1 : 0;
	
	FIFO_PUSH(CAN_fifo, Fr);
}

/**
	@brief: ISR - CAN_RX_FIFO is full   
 */
void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan)
{
	errmsg("CAN_RX_FIFO0 is full\r\n");
}

/*******************************************************************************
                          Module interface (Common functions) 
*******************************************************************************/
/**
	@brief Calculate time quantum (TQ) and segments SJW, BS1, BS2 to set 
	required baudrate  
 */ 
static int32_t CalculatePrescaler (CAN_HandleTypeDef *hcan, uint32_t bdr)
{
	uint32_t CAN_PCLK;			// Frequency of CAN module work 
	uint8_t	Tq_num = 25;		// Number of time quants (TQ) in bit
	uint8_t	prescaler;			// Frequency prescaler value
	uint8_t	SP;							// Sample point position [%]
	int32_t	Fq;							// Frequency of time quantum (1/TQ)
	uint8_t Tseg1;
	uint8_t Tseg2;
	uint8_t SJW;
	
	CAN_PCLK = HAL_RCC_GetPCLK1Freq();	// bdr = 1/(Tq*(1+BS1+BS2)) = Fq/(1+BS1+BS2)
	
	// Calculating segments timing
	while (Tq_num >= 8)
	{
		Fq = bdr * Tq_num;
		prescaler = CAN_PCLK / Fq;	// TQ = PCLK1 / Prescaler

	// Frequency accuracy checking (should be around 0,2%)
		if ((CAN_PCLK % Fq) < (Fq >> 5)) break;
		
		Tq_num--;	// Try next value
	}

	if (Tq_num < 8) 
	{
		errmsg("Unable to find TQ number for CAN baudrate\r\n");
		return -1;			// Out of standart TQ number value
	}
	
	hcan->Init.Prescaler = prescaler;
	
	// Sample point position
	SP = (Tq_num * CAN_SP) / 100;
	// Bit-Segments lenght calculation 
	Tseg2 = Tq_num - SP;
	Tseg1 = SP - 1;
	SJW = (Tseg2 > 5) ? 4 : (Tseg2 - 1);   
	// Fill init structure
	hcan->Init.TimeSeg2 = CAN_ConvertToBS2(Tseg2);
	hcan->Init.TimeSeg1 = CAN_ConvertToBS1(Tseg1);
  hcan->Init.SyncJumpWidth = CAN_ConvertToSJW(SJW);

#if DEBUG_AUTOBAUD	
	dbgmsg("\r\n> CAN autobaud results:\r\n");
	dbgmsg("Prescaler:\t\t%u\r\n", prescaler);
	dbgmsg("Accuracy:\t\t%d\r\n", Fq >> 5);
	dbgmsg("Error:\t\t\t%lu\r\n", (unsigned long)CAN_PCLK % Fq);
	dbgmsg("F_quantum:\t\t%d\r\n", Fq);
	dbgmsg("quantum_num:\t\t%u\t[Tq]\r\n", Tq_num);
	dbgmsg("Tseg1:\t\t\t%u\t[Tq]\r\n", Tseg1);
	dbgmsg("Tseg2:\t\t\t%u\t[Tq]\r\n", Tseg2);
	dbgmsg("Sample Point:\t\t%u\t[Tq]\r\n", SP);
	dbgmsg("SJW:\t\t\t%u\t[Tq]\r\n", SJW);
#endif
	// Real baudrate 
	if (Tseg1 && Tseg2) return (Fq / (1 + Tseg1 + Tseg2));
	return 0;
}

/**
	@brief Initialization of CAN 2.0B onboard interface
 */ 
int32_t CAN_Init (uint32_t bdr)
{
  xCAN1.Instance = CAN1;
	GPIO_InitTypeDef GPIO_InitStruct;
	
	// Slope control (Deactivated)
	GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, GPIO_PIN_RESET);
	
	// Configure determined baudrate
	int32_t baud = CalculatePrescaler(&xCAN1, bdr);
					   
  if (baud <= 0) return -1;							// Autobaud error
	
	xCAN1.Init.Mode = CAN_MODE_NORMAL;  	          
  xCAN1.Init.TimeTriggeredMode = DISABLE;
  xCAN1.Init.AutoBusOff = DISABLE; 			
  xCAN1.Init.AutoWakeUp = DISABLE;
  xCAN1.Init.AutoRetransmission = DISABLE;
  xCAN1.Init.ReceiveFifoLocked = DISABLE;
  xCAN1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&xCAN1) != HAL_OK)	return -2;	// CAN hardware init error

	// Configure CAN Harware Reception FIFO
	if (CAN_SetBroadcastFilter(CAN_RX_FIFO0)) return -3;	// Filter settings error
	
	int8_t res = CAN_SelfTest(&xCAN1);
  if (res) 
	{
		errmsg("CAN_SelfTest error: %d\r\n", res);
		return -4;													// SelfTest error
	}
  
	// Enable interrupts  TODO: Enable error intterrupts and handle it
	HAL_CAN_ActivateNotification(&xCAN1, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO0_FULL);
	HAL_NVIC_SetPriority(CAN1_RX0_IRQn, CAN_RX0_PreP, CAN_RX0_SubP);
  HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
	HAL_NVIC_SetPriority(CAN1_SCE_IRQn, CAN_SCE_PreP, CAN_SCE_SubP);
  HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
	
	if (HAL_CAN_Start(&xCAN1)) return -5;
	
	FIFO_FLUSH(CAN_fifo);
	
  return baud;
}

/**
	@brief Activates CAN_RX_FIFO and sets filter receiving any frame 
 */ 
HAL_StatusTypeDef CAN_SetBroadcastFilter (uint32_t rx_fifo)
{
	CAN_FilterTypeDef hfilter;
	
	hfilter.FilterBank = 0;
	hfilter.FilterFIFOAssignment = rx_fifo;
	hfilter.FilterIdHigh = 0x0000;
	hfilter.FilterIdLow = 0x0000;
	hfilter.FilterMaskIdHigh = 0x0000 << 5;
	hfilter.FilterMaskIdLow = 0x0000;
	hfilter.FilterMode = CAN_FILTERMODE_IDMASK;
	hfilter.FilterScale = CAN_FILTERSCALE_32BIT;
	hfilter.FilterActivation = ENABLE;

	return HAL_CAN_ConfigFilter(&xCAN1, &hfilter);
}

/**
	@brief Sets filter for CAN_RX_FIFO (TODO) 
 */ 
HAL_StatusTypeDef CAN_SetFilter (uint32_t rx_fifo)
{
	CAN_FilterTypeDef hfilter;
	return HAL_OK;
}

/**
	@brief Activates CAN_RX_FIFO and sets filter receiving any frame 
	@mote	0 - SUCCESS
 */ 
int8_t CAN_SelfTest(CAN_HandleTypeDef *hcan)
{
	int8_t res = 0;
	uint32_t tickstart = 0U;
	uint32_t mode = hcan->Init.Mode;
	uint32_t test_id = 0x12345678;
	uint8_t test_data[8] = {0x01, 0x02, 0xA3, 0xB4, 0xC5, 0xD6, 0xE7, 0xF8};
	CAN_RxHeaderTypeDef Rx_hdr;		// CAN frame received header
	uint8_t Rx_data[8] = {0};			// CAN received data
	
	// CAN module running?
	if (hcan->State == HAL_CAN_STATE_LISTENING)
	{
		if (HAL_CAN_Stop(hcan)) return -1;
	}
	
	hcan->Init.Mode = CAN_MODE_SILENT_LOOPBACK;
	
	if (HAL_CAN_Init(hcan))	return -2;
	
	if (HAL_CAN_Start(hcan)) return -3;
	
	if (CAN2B_SendDataFrame(test_id, test_data, sizeof(test_data)))	return -4;
	
	tickstart = HAL_GetTick();	
	while (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &Rx_hdr, Rx_data) != HAL_OK)
	{
		if ((HAL_GetTick() - tickstart) > SELFTEST_TMOUT) return -5;
	}
	
#if DEBUG_SELFTEST
	dbgmsg("> SelfTest CAN msg:\r\n");
	dbgmsg("RTR:\t\t\t%d\r\n", Rx_hdr.RTR);
	dbgmsg("DLC:\t\t\t%d\r\n", Rx_hdr.DLC);
	dbgmsg("IDE:\t\t\t%d\r\n", Rx_hdr.IDE);
	dbgmsg("Header:\t\t\t0x%08X\r\n", Rx_hdr.ExtId);
	for (int i = 0; i < Rx_hdr.DLC; i++)
	{
		dbgmsg("Data[%d]:\t\t0x%02X\r\n", i, Rx_data[i]);
	}
#endif
	
	if ((test_id != Rx_hdr.ExtId) || 
		strncmp((char*)test_data, (char*)Rx_data, sizeof(test_data))) res = -6;

	// Restore previous mode
	if (HAL_CAN_Stop(hcan)) res = -7;
	hcan->Init.Mode = mode;
	if (HAL_CAN_Init(hcan))	res = -8;
	return res;	// 0 - SUCCESS
}

/**
	@brief Sending CAN2.0B Data Frame via onboard CAN 
 */ 
HAL_StatusTypeDef CAN2B_SendDataFrame (uint32_t id, uint8_t data[], uint8_t len)
{
	CAN_TxHeaderTypeDef Tx_hdr;
	HAL_StatusTypeDef res;
	uint32_t tickstart = 0U;
	uint32_t TxMailbox_num;	// Number of TxMailbox that has been used for transmission
	
	Tx_hdr.StdId = 0;
	Tx_hdr.ExtId = id;
	Tx_hdr.DLC = len;
	Tx_hdr.IDE = CAN_ID_EXT;
	Tx_hdr.RTR = CAN_RTR_DATA;
	Tx_hdr.TransmitGlobalTime = DISABLE;
	
	tickstart = HAL_GetTick();
	do
	{
		res = HAL_CAN_AddTxMessage(&xCAN1, &Tx_hdr, data, &TxMailbox_num);
		if ((HAL_GetTick() - tickstart) > SENDFRAME_TMOUT)
		{		
			HAL_CAN_AbortTxRequest(&xCAN1, TxMailbox_num);
			return res;
		}
	}while (res != HAL_OK);
	
	return HAL_OK;
}

/**
	@brief Sending CAN2.0B Remote Frame via onboard CAN (TODO) 
 */ 
HAL_StatusTypeDef CAN2B_SendRemoteFrame ()
{
	return HAL_OK;
}

/**
	@brief Getting CAN Frame via onboard CAN 
 */ 
_Bool CAN_GetFrame (CANframe_t *pstore)
{
	if (FIFO_AVAIL_COUNT(CAN_fifo))
	{		
		FIFO_POP(CAN_fifo, *pstore);
		return 1;
	}
  else return 0;		// No data in software fifo	
}

/**
	@brief Getting All available data from software FIFO 
 */
void CAN_GetAllAvailableData (CANframe_t *pstore)
{
	uint32_t cnt = FIFO_AVAIL_COUNT(CAN_fifo);
	if FIFO_IS_FULL(CAN_fifo) dbgmsg("CAN software FIFO is full\r\n");

	for (int i = 0; i < cnt; i++)
	{
		FIFO_POP(CAN_fifo, *pstore++);
	}
	// We've read all available data from fifo, so clear it for new input
	FIFO_FLUSH(CAN_fifo);
}

//-----------------------------------------------------------------------------
// Convert integer number of quants to HAL_CAN value
static uint32_t CAN_ConvertToBS1(uint8_t num)
{
	switch (num)
	{
		case 1: return CAN_BS1_1TQ;
		case 2: return CAN_BS1_2TQ;
		case 3: return CAN_BS1_3TQ;
		case 4: return CAN_BS1_4TQ;
		case 5: return CAN_BS1_5TQ;
		case 6: return CAN_BS1_6TQ;
		case 7: return CAN_BS1_7TQ;
		case 8: return CAN_BS1_8TQ;
		case 9: return CAN_BS1_9TQ;
		case 10: return CAN_BS1_10TQ;
		case 11: return CAN_BS1_11TQ;
		case 12: return CAN_BS1_12TQ;
		case 13: return CAN_BS1_13TQ;
		case 14: return CAN_BS1_14TQ;
		case 15: return CAN_BS1_15TQ;
		case 16: return CAN_BS1_16TQ;
		default: errmsg("Invalid CAN_BS1_TQ value\r\n");
			return 0;
	}
}

static uint32_t CAN_ConvertToBS2(uint8_t num)
{
	switch (num)
	{
		case 1: return CAN_BS2_1TQ;
		case 2: return CAN_BS2_2TQ;
		case 3: return CAN_BS2_3TQ;
		case 4: return CAN_BS2_4TQ;
		case 5: return CAN_BS2_5TQ;
		case 6: return CAN_BS2_6TQ;
		case 7: return CAN_BS2_7TQ;
		case 8: return CAN_BS2_8TQ;
		default: errmsg("Invalid CAN_BS2_TQ value\r\n");
			return 0;
	}
}

static uint32_t CAN_ConvertToSJW(uint8_t num)
{
	switch (num)
	{
		case 1: return CAN_SJW_1TQ;
		case 2: return CAN_SJW_2TQ;
		case 3: return CAN_SJW_3TQ;
		case 4: return CAN_SJW_4TQ;
		default: errmsg("Invalid CAN_SJW_TQ value\r\n");
			return 0;
	}
}

