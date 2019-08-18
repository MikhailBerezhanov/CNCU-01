/**
  ******************************************************************************
  * @file           : i2c.c
  * @brief          : Module working with CNCU I2C interface
  ******************************************************************************
*/
#include "i2c.h"
#define DEBUG_MSG DEBUG_I2C
#include "dbgmsg.h"

static I2C_HandleTypeDef xI2C3;

/* Private functions ---------------------------------------------------------*/

void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(hi2c->Instance==I2C3)
  { 
    /**I2C3 GPIO Configuration    
    PC9     ------> I2C3_SDA
    PA8     ------> I2C3_SCL 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Peripheral clock enable */
    __HAL_RCC_I2C3_CLK_ENABLE();
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{

  if(hi2c->Instance==I2C3)
  {
    /* Peripheral clock disable */
    __HAL_RCC_I2C3_CLK_DISABLE();
  
    /**I2C3 GPIO Configuration    
    PC9     ------> I2C3_SDA
    PA8     ------> I2C3_SCL 
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_9);

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
  }

}

/*******************************************************************************
                          Module interface (Common functions) 
*******************************************************************************/
/* I2C3 init function */
char I2C_Init(void)
{
	xI2C3.Instance = I2C3;
	xI2C3.Init.ClockSpeed = 100000;
	xI2C3.Init.DutyCycle = I2C_DUTYCYCLE_2;
	xI2C3.Init.OwnAddress1 = 0;
	xI2C3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	xI2C3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	xI2C3.Init.OwnAddress2 = 0;
	xI2C3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	xI2C3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&xI2C3) != HAL_OK)
	{
		errmsg("I2C: Unable to init I2C3\r\n");
		return 1;
	}
	
	return 0;
}
