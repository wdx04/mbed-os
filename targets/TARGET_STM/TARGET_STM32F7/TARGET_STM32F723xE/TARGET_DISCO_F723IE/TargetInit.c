/* mbed Microcontroller Library
 * SPDX-License-Identifier: BSD-3-Clause
 ******************************************************************************
 *
 * Copyright (c) 2015-2021 STMicroelectronics.
 * All rights reserved.
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

#include "stm32f7xx.h"

#define PSRAM_OK         ((uint8_t)0x00)
#define PSRAM_ERROR      ((uint8_t)0x01)

#define PSRAM_DEVICE_ADDR  ((uint32_t)0x60000000)
#define PSRAM_DEVICE_SIZE  ((uint32_t)0x80000)  /* SRAM device size in Bytes */  
  
#define PSRAM_MEMORY_WIDTH    FMC_NORSRAM_MEM_BUS_WIDTH_16

#define PSRAM_BURSTACCESS     FMC_BURST_ACCESS_MODE_DISABLE
/* #define PSRAM_BURSTACCESS  FMC_BURST_ACCESS_MODE_ENABLE*/
  
#define PSRAM_WRITEBURST      FMC_WRITE_BURST_DISABLE
/* #define PSRAM_WRITEBURST   FMC_WRITE_BURST_ENABLE */
 
#define CONTINUOUSCLOCK_FEATURE  FMC_CONTINUOUS_CLOCK_SYNC_ONLY 
/* #define CONTINUOUSCLOCK_FEATURE   FMC_CONTINUOUS_CLOCK_SYNC_ASYNC */ 

/**
  * @brief  Initializes PSRAM MSP.
  * @param  hsram: PSRAM handle
  * @param  Params  
  * @retval None
  */
__weak void BSP_PSRAM_MspInit(SRAM_HandleTypeDef  *hsram, void *Params)
{  
  GPIO_InitTypeDef gpio_init_structure;
  
  /* Enable FMC clock */
  __HAL_RCC_FMC_CLK_ENABLE();
  
  /* Enable GPIOs clock */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  
  /* Common GPIO configuration */
  gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
  gpio_init_structure.Pull      = GPIO_PULLUP;
  gpio_init_structure.Speed     = GPIO_SPEED_HIGH;
  gpio_init_structure.Alternate = GPIO_AF12_FMC;

  /* GPIOD configuration: GPIO_PIN_7 is  FMC_NE1 , GPIO_PIN_11 ans GPIO_PIN_12 are PSRAM_A16 and PSRAM_A17 */
  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7 | GPIO_PIN_8 |\
                              GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_14 | GPIO_PIN_15;
   
  HAL_GPIO_Init(GPIOD, &gpio_init_structure);

  /* GPIOE configuration */  
  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 |\
                              GPIO_PIN_12 |GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOE, &gpio_init_structure);
  
  /* GPIOF configuration */  
  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |\
                              GPIO_PIN_5 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15; 
  HAL_GPIO_Init(GPIOF, &gpio_init_structure);

  /* GPIOG configuration */  
  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |\
                              GPIO_PIN_5; 
  HAL_GPIO_Init(GPIOG, &gpio_init_structure); 
}

/**
  * @brief  Initializes the PSRAM device.
  * @retval PSRAM status
  */
uint8_t BSP_PSRAM_Init(void)
{
  static SRAM_HandleTypeDef psramHandle;
  static FMC_NORSRAM_TimingTypeDef Timing;  
  static uint8_t psram_status = PSRAM_ERROR;
  
  /* PSRAM device configuration */
  psramHandle.Instance = FMC_NORSRAM_DEVICE;
  psramHandle.Extended = FMC_NORSRAM_EXTENDED_DEVICE;
  
  /* PSRAM device configuration */
  /* Timing configuration derived from system clock (up to 216Mhz)
     for 108Mhz as PSRAM clock frequency */
  Timing.AddressSetupTime      = 9;
  Timing.AddressHoldTime       = 2;
  Timing.DataSetupTime         = 6;
  Timing.BusTurnAroundDuration = 1;
  Timing.CLKDivision           = 2;
  Timing.DataLatency           = 2;
  Timing.AccessMode            = FMC_ACCESS_MODE_A;
  
  psramHandle.Init.NSBank             = FMC_NORSRAM_BANK1;
  psramHandle.Init.DataAddressMux     = FMC_DATA_ADDRESS_MUX_DISABLE;
  psramHandle.Init.MemoryType         = FMC_MEMORY_TYPE_SRAM;
  psramHandle.Init.MemoryDataWidth    = PSRAM_MEMORY_WIDTH;
  psramHandle.Init.BurstAccessMode    = PSRAM_BURSTACCESS;
  psramHandle.Init.WaitSignalPolarity = FMC_WAIT_SIGNAL_POLARITY_LOW;
  psramHandle.Init.WaitSignalActive   = FMC_WAIT_TIMING_BEFORE_WS;
  psramHandle.Init.WriteOperation     = FMC_WRITE_OPERATION_ENABLE;
  psramHandle.Init.WaitSignal         = FMC_WAIT_SIGNAL_DISABLE;
  psramHandle.Init.ExtendedMode       = FMC_EXTENDED_MODE_DISABLE;
  psramHandle.Init.AsynchronousWait   = FMC_ASYNCHRONOUS_WAIT_DISABLE;
  psramHandle.Init.WriteBurst         = PSRAM_WRITEBURST;
  psramHandle.Init.WriteFifo          = FMC_WRITE_FIFO_DISABLE;
  psramHandle.Init.PageSize           = FMC_PAGE_SIZE_NONE;  
  psramHandle.Init.ContinuousClock    = CONTINUOUSCLOCK_FEATURE;
  
  /* PSRAM controller initialization */
  BSP_PSRAM_MspInit(&psramHandle, NULL); /* __weak function can be rewritten by the application */
  if(HAL_SRAM_Init(&psramHandle, &Timing, &Timing) != HAL_OK)
  {
    psram_status = PSRAM_ERROR;
  }
  else
  {
    psram_status = PSRAM_OK;
  }
  return psram_status;
}

/**
  * @brief  Configure the MPU attributes as Write Back for PSRAM.
  * @note   The Base Address is 0x64000000.
  *         The Region Size is 1MBytes, it is related to PSRAM memory size.
  * @param  None
  * @retval None
  */
static void MPU_ConfigPSRAM(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;
  
  /* Disable the MPU */
  HAL_MPU_Disable();
  
  /* Configure the MPU as Strongly ordered for not defined regions */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x00;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU attributes for PSRAM with recommended configurations:
     Normal memory, Shareable, write-back */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x60000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_1MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU attributes FMC control registers */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0xA0000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_8KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  
  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void TargetBSP_Init(void)
{
    // Enable external PSRAM
    BSP_PSRAM_Init();
    MPU_ConfigPSRAM();
}
