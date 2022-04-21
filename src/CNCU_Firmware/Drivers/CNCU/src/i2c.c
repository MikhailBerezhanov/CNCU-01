/**
  ******************************************************************************
  * @file           : i2c.c
  * @brief          : Module working with CNCU I2C interface
  ******************************************************************************
*/
#include "i2c.h"

#define DEBUG_MSG DEBUG_I2C
#include "dbgmsg.h"

static I2C_HandleTypeDef xI2C1;

/* Private functions ---------------------------------------------------------*/

void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(hi2c->Instance==I2C1)
  {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration    
    PB8     ------> I2C1_SCL
    PB9     ------> I2C1_SDA 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Peripheral clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
    /* I2C1 interrupt Init */
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, I2C1_PreP, I2C1_SubP);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
  }

}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
  if(hi2c->Instance==I2C1)
  {
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();
  
    /**I2C1 GPIO Configuration    
    PB8     ------> I2C1_SCL
    PB9     ------> I2C1_SDA 
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8|GPIO_PIN_9);

    /* I2C1 interrupt DeInit */
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
  }
}

/*******************************************************************************
                          Module interface (Common functions) 
*******************************************************************************/
/* I2C3 init function */
char I2C_Init(void)
{
	xI2C1.Instance = I2C1;
	xI2C1.Init.ClockSpeed = 100000;
	xI2C1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	xI2C1.Init.OwnAddress1 = 0;
	xI2C1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	xI2C1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	xI2C1.Init.OwnAddress2 = 0;
	xI2C1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	xI2C1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&xI2C1) != HAL_OK)
	{
		errmsg("I2C: Unable to init I2C3\r\n");
		return 1;
	}
	
	return 0;
}
