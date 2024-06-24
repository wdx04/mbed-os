#include "mbed_error.h"
#include "stm32f4xx_hal.h"

#define   SDRAM_OK         ((uint8_t)0x00)
#define   SDRAM_ERROR      ((uint8_t)0x01)

/**
  * @brief  FMC SDRAM Bank address
  */
#define SDRAM_DEVICE_ADDR         ((uint32_t)0xD0000000)
#define SDRAM_DEVICE_SIZE         ((uint32_t)0x800000)  /* SDRAM device size in Bytes */

/**
  * @brief  FMC SDRAM Memory Width
  */
/* #define SDRAM_MEMORY_WIDTH   FMC_SDRAM_MEM_BUS_WIDTH_8 */
#define SDRAM_MEMORY_WIDTH      FMC_SDRAM_MEM_BUS_WIDTH_16

/**
  * @brief  FMC SDRAM CAS Latency
  */
/* #define SDRAM_CAS_LATENCY    FMC_SDRAM_CAS_LATENCY_2 */
#define SDRAM_CAS_LATENCY       FMC_SDRAM_CAS_LATENCY_3

/**
  * @brief  FMC SDRAM Memory clock period
  */
#define SDCLOCK_PERIOD          FMC_SDRAM_CLOCK_PERIOD_2    /* Default configuration used with LCD */
/* #define SDCLOCK_PERIOD       FMC_SDRAM_CLOCK_PERIOD_3 */

/**
  * @brief  FMC SDRAM Memory Read Burst feature
  */
#define SDRAM_READBURST         FMC_SDRAM_RBURST_ENABLE    /* Default configuration used with LCD */
/* #define SDRAM_READBURST      FMC_SDRAM_RBURST_ENABLE */

/**
  * @brief  FMC SDRAM Bank Remap
  */
/* #define SDRAM_BANK_REMAP */

/* Set the refresh rate counter */
/* (15.62 us x Freq) - 20 */
#define REFRESH_COUNT           ((uint32_t)1386)   /* SDRAM refresh counter */
#define SDRAM_TIMEOUT           ((uint32_t)0xFFFF)

/* DMA definitions for SDRAM DMA transfer */
#define __DMAx_CLK_ENABLE       __HAL_RCC_DMA2_CLK_ENABLE
#define SDRAM_DMAx_CHANNEL      DMA_CHANNEL_0
#define SDRAM_DMAx_STREAM       DMA2_Stream0
#define SDRAM_DMAx_IRQn         DMA2_Stream0_IRQn
#define SDRAM_DMAx_IRQHandler   DMA2_Stream0_IRQHandler

/**
  * @brief  FMC SDRAM Mode definition register defines
  */
#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

static void MPU_Config(void)
{
	MPU_Region_InitTypeDef MPU_InitStruct;

	HAL_MPU_Disable();

	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = SDRAM_DEVICE_ADDR;
	MPU_InitStruct.Size             = SDRAM_DEVICE_SIZE;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER2;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;

	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

static SDRAM_HandleTypeDef SdramHandle;
static FMC_SDRAM_TimingTypeDef Timing;
static FMC_SDRAM_CommandTypeDef Command;

void BSP_SDRAM_MspInit(SDRAM_HandleTypeDef  *hsdram, void *Params)
{
  static DMA_HandleTypeDef dmaHandle;
  GPIO_InitTypeDef GPIO_InitStructure;

  if (hsdram != (SDRAM_HandleTypeDef *)NULL)
  {
    /* Enable FMC clock */
    __HAL_RCC_FMC_CLK_ENABLE();

    /* Enable chosen DMAx clock */
    __DMAx_CLK_ENABLE();

    /* Enable GPIOs clock */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    /*-- GPIOs Configuration -----------------------------------------------------*/
    /*
     +-------------------+--------------------+--------------------+--------------------+
     +                       SDRAM pins assignment                                      +
     +-------------------+--------------------+--------------------+--------------------+
     | PD0  <-> FMC_D2   | PE0  <-> FMC_NBL0  | PF0  <-> FMC_A0    | PG0  <-> FMC_A10   |
     | PD1  <-> FMC_D3   | PE1  <-> FMC_NBL1  | PF1  <-> FMC_A1    | PG1  <-> FMC_A11   |
     | PD8  <-> FMC_D13  | PE7  <-> FMC_D4    | PF2  <-> FMC_A2    | PG8  <-> FMC_SDCLK |
     | PD9  <-> FMC_D14  | PE8  <-> FMC_D5    | PF3  <-> FMC_A3    | PG15 <-> FMC_NCAS  |
     | PD10 <-> FMC_D15  | PE9  <-> FMC_D6    | PF4  <-> FMC_A4    |--------------------+
     | PD14 <-> FMC_D0   | PE10 <-> FMC_D7    | PF5  <-> FMC_A5    |
     | PD15 <-> FMC_D1   | PE11 <-> FMC_D8    | PF11 <-> FMC_NRAS  |
     +-------------------| PE12 <-> FMC_D9    | PF12 <-> FMC_A6    |
                         | PE13 <-> FMC_D10   | PF13 <-> FMC_A7    |
                         | PE14 <-> FMC_D11   | PF14 <-> FMC_A8    |
                         | PE15 <-> FMC_D12   | PF15 <-> FMC_A9    |
     +-------------------+--------------------+--------------------+
     | PB5 <-> FMC_SDCKE1|
     | PB6 <-> FMC_SDNE1 |
     | PC0 <-> FMC_SDNWE |
     +-------------------+

    */

    /* Common GPIO configuration */
    GPIO_InitStructure.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FAST;
    GPIO_InitStructure.Pull  = GPIO_NOPULL;
    GPIO_InitStructure.Alternate = GPIO_AF12_FMC;

    /* GPIOB configuration */
    GPIO_InitStructure.Pin = GPIO_PIN_5 | GPIO_PIN_6;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* GPIOC configuration */
    GPIO_InitStructure.Pin = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* GPIOD configuration */
    GPIO_InitStructure.Pin = GPIO_PIN_0 | GPIO_PIN_1  | GPIO_PIN_8 |
                             GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 |
                             GPIO_PIN_15;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);

    /* GPIOE configuration */
    GPIO_InitStructure.Pin = GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_7 |
                             GPIO_PIN_8  | GPIO_PIN_9  | GPIO_PIN_10 |
                             GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 |
                             GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStructure);

    /* GPIOF configuration */
    GPIO_InitStructure.Pin = GPIO_PIN_0  | GPIO_PIN_1 | GPIO_PIN_2 |
                             GPIO_PIN_3  | GPIO_PIN_4 | GPIO_PIN_5 |
                             GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 |
                             GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStructure);

    /* GPIOG configuration */
    GPIO_InitStructure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 |
                             GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);

    /* Configure common DMA parameters */
    dmaHandle.Init.Channel             = SDRAM_DMAx_CHANNEL;
    dmaHandle.Init.Direction           = DMA_MEMORY_TO_MEMORY;
    dmaHandle.Init.PeriphInc           = DMA_PINC_ENABLE;
    dmaHandle.Init.MemInc              = DMA_MINC_ENABLE;
    dmaHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    dmaHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    dmaHandle.Init.Mode                = DMA_NORMAL;
    dmaHandle.Init.Priority            = DMA_PRIORITY_HIGH;
    dmaHandle.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    dmaHandle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    dmaHandle.Init.MemBurst            = DMA_MBURST_SINGLE;
    dmaHandle.Init.PeriphBurst         = DMA_PBURST_SINGLE;

    dmaHandle.Instance = SDRAM_DMAx_STREAM;

    /* Associate the DMA handle */
    __HAL_LINKDMA(hsdram, hdma, dmaHandle);

    /* Deinitialize the stream for new transfer */
    HAL_DMA_DeInit(&dmaHandle);

    /* Configure the DMA stream */
    HAL_DMA_Init(&dmaHandle);

    /* NVIC configuration for DMA transfer complete interrupt */
    HAL_NVIC_SetPriority(SDRAM_DMAx_IRQn, 0x0F, 0);
    HAL_NVIC_EnableIRQ(SDRAM_DMAx_IRQn);
  } /* of if(hsdram != (SDRAM_HandleTypeDef  *)NULL) */
}

void BSP_SDRAM_Initialization_sequence(uint32_t RefreshCount)
{
  __IO uint32_t tmpmrd = 0;

  /* Step 1:  Configure a clock configuration enable command */
  Command.CommandMode             = FMC_SDRAM_CMD_CLK_ENABLE;
  Command.CommandTarget           = FMC_SDRAM_CMD_TARGET_BANK2;
  Command.AutoRefreshNumber       = 1;
  Command.ModeRegisterDefinition  = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(&SdramHandle, &Command, SDRAM_TIMEOUT);

  /* Step 2: Insert 100 us minimum delay */
  /* Inserted delay is equal to 1 ms due to systick time base unit (ms) */
  HAL_Delay(1);

  /* Step 3: Configure a PALL (precharge all) command */
  Command.CommandMode             = FMC_SDRAM_CMD_PALL;
  Command.CommandTarget           = FMC_SDRAM_CMD_TARGET_BANK2;
  Command.AutoRefreshNumber       = 1;
  Command.ModeRegisterDefinition  = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(&SdramHandle, &Command, SDRAM_TIMEOUT);

  /* Step 4: Configure an Auto Refresh command */
  Command.CommandMode             = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
  Command.CommandTarget           = FMC_SDRAM_CMD_TARGET_BANK2;
  Command.AutoRefreshNumber       = 4;
  Command.ModeRegisterDefinition  = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(&SdramHandle, &Command, SDRAM_TIMEOUT);

  /* Step 5: Program the external memory mode register */
  tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1          |
           SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
           SDRAM_MODEREG_CAS_LATENCY_3           |
           SDRAM_MODEREG_OPERATING_MODE_STANDARD |
           SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

  Command.CommandMode             = FMC_SDRAM_CMD_LOAD_MODE;
  Command.CommandTarget           = FMC_SDRAM_CMD_TARGET_BANK2;
  Command.AutoRefreshNumber       = 1;
  Command.ModeRegisterDefinition  = tmpmrd;

  /* Send the command */
  HAL_SDRAM_SendCommand(&SdramHandle, &Command, SDRAM_TIMEOUT);

  /* Step 6: Set the refresh rate counter */
  /* Set the device refresh rate */
  HAL_SDRAM_ProgramRefreshRate(&SdramHandle, RefreshCount);
}

uint8_t BSP_SDRAM_Init(void)
{
  static uint8_t sdramstatus = SDRAM_ERROR;

  /* SDRAM device configuration */
  SdramHandle.Instance = FMC_SDRAM_DEVICE;

  /* FMC Configuration -------------------------------------------------------*/
  /* FMC SDRAM Bank configuration */
  /* Timing configuration for 90 Mhz of SD clock frequency (180Mhz/2) */
  /* TMRD: 2 Clock cycles */
  Timing.LoadToActiveDelay    = 2;
  /* TXSR: min=70ns (7x11.11ns) */
  Timing.ExitSelfRefreshDelay = 7;
  /* TRAS: min=42ns (4x11.11ns) max=120k (ns) */
  Timing.SelfRefreshTime      = 4;
  /* TRC:  min=70 (7x11.11ns) */
  Timing.RowCycleDelay        = 7;
  /* TWR:  min=1+ 7ns (1+1x11.11ns) */
  Timing.WriteRecoveryTime    = 2;
  /* TRP:  20ns => 2x11.11ns*/
  Timing.RPDelay              = 2;
  /* TRCD: 20ns => 2x11.11ns */
  Timing.RCDDelay             = 2;

  /* FMC SDRAM control configuration */
  SdramHandle.Init.SDBank             = FMC_SDRAM_BANK2;
  /* Row addressing: [7:0] */
  SdramHandle.Init.ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_8;
  /* Column addressing: [11:0] */
  SdramHandle.Init.RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_12;
  SdramHandle.Init.MemoryDataWidth    = SDRAM_MEMORY_WIDTH;
  SdramHandle.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  SdramHandle.Init.CASLatency         = SDRAM_CAS_LATENCY;
  SdramHandle.Init.WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  SdramHandle.Init.SDClockPeriod      = SDCLOCK_PERIOD;
  SdramHandle.Init.ReadBurst          = SDRAM_READBURST;
  SdramHandle.Init.ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_1;

  /* SDRAM controller initialization */
  /* __weak function can be surcharged by the application code */
  BSP_SDRAM_MspInit(&SdramHandle, (void *)NULL);
  if (HAL_SDRAM_Init(&SdramHandle, &Timing) != HAL_OK)
  {
    sdramstatus = SDRAM_ERROR;
  }
  else
  {
    sdramstatus = SDRAM_OK;
  }

  /* SDRAM initialization sequence */
  BSP_SDRAM_Initialization_sequence(REFRESH_COUNT);

  return sdramstatus;
}

void TargetBSP_Init(void)
{
    BSP_SDRAM_Init();
    MPU_Config();
}
