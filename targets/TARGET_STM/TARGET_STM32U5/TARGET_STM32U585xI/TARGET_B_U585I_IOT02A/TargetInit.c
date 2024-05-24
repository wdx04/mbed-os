#include "mbed_error.h"
#include "stm32u5xx.h"
#include "aps6408.h"

/* Common Error codes */
#define BSP_ERROR_NONE                    0
#define BSP_ERROR_NO_INIT                -1
#define BSP_ERROR_WRONG_PARAM            -2
#define BSP_ERROR_BUSY                   -3
#define BSP_ERROR_PERIPH_FAILURE         -4
#define BSP_ERROR_COMPONENT_FAILURE      -5
#define BSP_ERROR_UNKNOWN_FAILURE        -6
#define BSP_ERROR_UNKNOWN_COMPONENT      -7
#define BSP_ERROR_BUS_FAILURE            -8
#define BSP_ERROR_CLOCK_FAILURE          -9
#define BSP_ERROR_MSP_FAILURE            -10
#define BSP_ERROR_FEATURE_NOT_SUPPORTED  -11

/* BSP OSPI error codes */
#define BSP_ERROR_OSPI_SUSPENDED          -20
#define BSP_ERROR_OSPI_ASSIGN_FAILURE     -24
#define BSP_ERROR_OSPI_SETUP_FAILURE      -25
#define BSP_ERROR_OSPI_MMP_LOCK_FAILURE   -26
#define BSP_ERROR_OSPI_MMP_UNLOCK_FAILURE -27

/* BSP BUS error codes */
#define BSP_ERROR_BUS_TRANSACTION_FAILURE    -100
#define BSP_ERROR_BUS_ARBITRATION_LOSS       -101
#define BSP_ERROR_BUS_ACKNOWLEDGE_FAILURE    -102
#define BSP_ERROR_BUS_PROTOCOL_FAILURE       -103

#define BSP_ERROR_BUS_MODE_FAULT             -104
#define BSP_ERROR_BUS_FRAME_ERROR            -105
#define BSP_ERROR_BUS_CRC_ERROR              -106
#define BSP_ERROR_BUS_DMA_FAILURE            -107

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup B_U585I_IOT02A_OSPI_Exported_Constants OSPI Exported Constants
  * @{
  */

/* Definition for OSPI clock resources */
#define OSPI_NOR_CLK_ENABLE()                 __HAL_RCC_OSPI2_CLK_ENABLE()
#define OSPI_NOR_CLK_DISABLE()                __HAL_RCC_OSPI2_CLK_DISABLE()

#define OSPI_NOR_CLK_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOF_CLK_ENABLE()
#define OSPI_NOR_DQS_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOF_CLK_ENABLE()
#define OSPI_NOR_CS_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOI_CLK_ENABLE()
#define OSPI_NOR_D0_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOF_CLK_ENABLE()
#define OSPI_NOR_D1_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOF_CLK_ENABLE()
#define OSPI_NOR_D2_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOF_CLK_ENABLE()
#define OSPI_NOR_D3_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOF_CLK_ENABLE()
#define OSPI_NOR_D4_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOH_CLK_ENABLE()
#define OSPI_NOR_D5_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOH_CLK_ENABLE()
#define OSPI_NOR_D6_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOH_CLK_ENABLE()
#define OSPI_NOR_D7_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOH_CLK_ENABLE()

#define OSPI_NOR_FORCE_RESET()                __HAL_RCC_OSPI2_FORCE_RESET()
#define OSPI_NOR_RELEASE_RESET()              __HAL_RCC_OSPI2_RELEASE_RESET()

/* Definition for OSPI Pins */
/* OSPI_CLK */
#define OSPI_NOR_CLK_PIN                      GPIO_PIN_4
#define OSPI_NOR_CLK_GPIO_PORT                GPIOF
#define OSPI_NOR_CLK_PIN_AF                   GPIO_AF5_OCTOSPI2
/* OSPI_DQS */
#define OSPI_NOR_DQS_PIN                      GPIO_PIN_12
#define OSPI_NOR_DQS_GPIO_PORT                GPIOF
#define OSPI_NOR_DQS_PIN_AF                   GPIO_AF5_OCTOSPI2
/* OSPI_CS */
#define OSPI_NOR_CS_PIN                       GPIO_PIN_5
#define OSPI_NOR_CS_GPIO_PORT                 GPIOI
#define OSPI_NOR_CS_PIN_AF                    GPIO_AF5_OCTOSPI2
/* OSPI_D0 */
#define OSPI_NOR_D0_PIN                       GPIO_PIN_0
#define OSPI_NOR_D0_GPIO_PORT                 GPIOF
#define OSPI_NOR_D0_PIN_AF                    GPIO_AF5_OCTOSPI2
/* OSPI_D1 */
#define OSPI_NOR_D1_PIN                       GPIO_PIN_1
#define OSPI_NOR_D1_GPIO_PORT                 GPIOF
#define OSPI_NOR_D1_PIN_AF                    GPIO_AF5_OCTOSPI2
/* OSPI_D2 */
#define OSPI_NOR_D2_PIN                       GPIO_PIN_2
#define OSPI_NOR_D2_GPIO_PORT                 GPIOF
#define OSPI_NOR_D2_PIN_AF                    GPIO_AF5_OCTOSPI2
/* OSPI_D3 */
#define OSPI_NOR_D3_PIN                       GPIO_PIN_3
#define OSPI_NOR_D3_GPIO_PORT                 GPIOF
#define OSPI_NOR_D3_PIN_AF                    GPIO_AF5_OCTOSPI2
/* OSPI_D4 */
#define OSPI_NOR_D4_PIN                       GPIO_PIN_9
#define OSPI_NOR_D4_GPIO_PORT                 GPIOH
#define OSPI_NOR_D4_PIN_AF                    GPIO_AF5_OCTOSPI2
/* OSPI_D5 */
#define OSPI_NOR_D5_PIN                       GPIO_PIN_10
#define OSPI_NOR_D5_GPIO_PORT                 GPIOH
#define OSPI_NOR_D5_PIN_AF                    GPIO_AF5_OCTOSPI2
/* OSPI_D6 */
#define OSPI_NOR_D6_PIN                       GPIO_PIN_11
#define OSPI_NOR_D6_GPIO_PORT                 GPIOH
#define OSPI_NOR_D6_PIN_AF                    GPIO_AF5_OCTOSPI2
/* OSPI_D7 */
#define OSPI_NOR_D7_PIN                       GPIO_PIN_12
#define OSPI_NOR_D7_GPIO_PORT                 GPIOH
#define OSPI_NOR_D7_PIN_AF                    GPIO_AF5_OCTOSPI2

/* Definition for OSPI RAM clock resources */
#define OSPI_RAM_CLK_ENABLE()                 __HAL_RCC_OSPI1_CLK_ENABLE()
#define OSPI_RAM_CLK_DISABLE()                __HAL_RCC_OSPI1_CLK_DISABLE()

#define OSPI_RAM_CLK_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOB_CLK_ENABLE()
#define OSPI_RAM_DQS_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOE_CLK_ENABLE()
#define OSPI_RAM_CS_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOB_CLK_ENABLE()
#define OSPI_RAM_D0_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOF_CLK_ENABLE()
#define OSPI_RAM_D1_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOF_CLK_ENABLE()
#define OSPI_RAM_D2_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOF_CLK_ENABLE()
#define OSPI_RAM_D3_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOF_CLK_ENABLE()
#define OSPI_RAM_D4_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOH_CLK_ENABLE()
#define OSPI_RAM_D5_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOI_CLK_ENABLE()
#define OSPI_RAM_D6_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOC_CLK_ENABLE()
#define OSPI_RAM_D7_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOD_CLK_ENABLE()

#define OSPI_RAM_FORCE_RESET()                __HAL_RCC_OSPI1_FORCE_RESET()
#define OSPI_RAM_RELEASE_RESET()              __HAL_RCC_OSPI1_RELEASE_RESET()

/* Definition for OSPI RAM Pins */
/* OSPI_CLK */
#define OSPI_RAM_CLK_PIN                      GPIO_PIN_10
#define OSPI_RAM_CLK_GPIO_PORT                GPIOB
#define OSPI_RAM_CLK_PIN_AF                   GPIO_AF10_OCTOSPI1
/* OSPI_DQS */
#define OSPI_RAM_DQS_PIN                      GPIO_PIN_3
#define OSPI_RAM_DQS_GPIO_PORT                GPIOE
#define OSPI_RAM_DQS_PIN_AF                   GPIO_AF3_OCTOSPI1
/* OSPI_CS */
#define OSPI_RAM_CS_PIN                       GPIO_PIN_11
#define OSPI_RAM_CS_GPIO_PORT                 GPIOB
#define OSPI_RAM_CS_PIN_AF                    GPIO_AF10_OCTOSPI1
/* OSPI_D0 */
#define OSPI_RAM_D0_PIN                       GPIO_PIN_8
#define OSPI_RAM_D0_GPIO_PORT                 GPIOF
#define OSPI_RAM_D0_PIN_AF                    GPIO_AF10_OCTOSPI1
/* OSPI_D1 */
#define OSPI_RAM_D1_PIN                       GPIO_PIN_9
#define OSPI_RAM_D1_GPIO_PORT                 GPIOF
#define OSPI_RAM_D1_PIN_AF                    GPIO_AF10_OCTOSPI1
/* OSPI_D2 */
#define OSPI_RAM_D2_PIN                       GPIO_PIN_7
#define OSPI_RAM_D2_GPIO_PORT                 GPIOF
#define OSPI_RAM_D2_PIN_AF                    GPIO_AF10_OCTOSPI1
/* OSPI_D3 */
#define OSPI_RAM_D3_PIN                       GPIO_PIN_6
#define OSPI_RAM_D3_GPIO_PORT                 GPIOF
#define OSPI_RAM_D3_PIN_AF                    GPIO_AF10_OCTOSPI1
/* OSPI_D4 */
#define OSPI_RAM_D4_PIN                       GPIO_PIN_2
#define OSPI_RAM_D4_GPIO_PORT                 GPIOH
#define OSPI_RAM_D4_PIN_AF                    GPIO_AF3_OCTOSPI1
/* OSPI_D5 */
#define OSPI_RAM_D5_PIN                       GPIO_PIN_0
#define OSPI_RAM_D5_GPIO_PORT                 GPIOI
#define OSPI_RAM_D5_PIN_AF                    GPIO_AF3_OCTOSPI1
/* OSPI_D6 */
#define OSPI_RAM_D6_PIN                       GPIO_PIN_3
#define OSPI_RAM_D6_GPIO_PORT                 GPIOC
#define OSPI_RAM_D6_PIN_AF                    GPIO_AF10_OCTOSPI1
/* OSPI_D7 */
#define OSPI_RAM_D7_PIN                       GPIO_PIN_7
#define OSPI_RAM_D7_GPIO_PORT                 GPIOD
#define OSPI_RAM_D7_PIN_AF                    GPIO_AF10_OCTOSPI1

/** @defgroup B_U585I_IOT02A_OSPI_RAM_Exported_Constants OSPI RAM Exported Constants
  * @{
  */
#define OSPI_RAM_INSTANCES_NUMBER         1U

/* OSPI Burst length */
#define BSP_OSPI_RAM_BURST_16_BYTES       (BSP_OSPI_RAM_BurstLength_t)APS6408_BURST_16_BYTES
#define BSP_OSPI_RAM_BURST_32_BYTES       (BSP_OSPI_RAM_BurstLength_t)APS6408_BURST_32_BYTES
#define BSP_OSPI_RAM_BURST_64_BYTES       (BSP_OSPI_RAM_BurstLength_t)APS6408_BURST_64_BYTES
#define BSP_OSPI_RAM_BURST_128_BYTES      (BSP_OSPI_RAM_BurstLength_t)APS6408_BURST_128_BYTES

/* Exported types ------------------------------------------------------------*/
/** @defgroup B_U585I_IOT02A_OSPI_Exported_Types OSPI Exported Types
  * @{
  */
typedef enum
{
  OSPI_ACCESS_NONE = 0,          /*!<  Instance not initialized,              */
  OSPI_ACCESS_INDIRECT,          /*!<  Instance use indirect mode access      */
  OSPI_ACCESS_MMP                /*!<  Instance use Memory Mapped Mode read   */
} OSPI_Access_t;
  
typedef struct
{
  uint32_t MemorySize;
  uint32_t ClockPrescaler;
  uint32_t SampleShifting;
  uint32_t TransferRate;
} MX_OSPI_InitTypeDef;

/** @defgroup B_U585I_IOT02A_OSPI_RAM_Exported_Types OSPI RAM Exported Types
  * @{
  */
#define BSP_OSPI_RAM_BurstLength_t APS6408_BurstLength_t

typedef enum
{
  BSP_OSPI_RAM_VARIABLE_LATENCY = HAL_OSPI_VARIABLE_LATENCY,
  BSP_OSPI_RAM_FIXED_LATENCY    = HAL_OSPI_FIXED_LATENCY
} BSP_OSPI_RAM_Latency_t;

typedef enum
{
  BSP_OSPI_RAM_HYBRID_BURST = 0,
  BSP_OSPI_RAM_LINEAR_BURST
} BSP_OSPI_RAM_BurstType_t;

typedef struct
{
  OSPI_Access_t               IsInitialized;   /*!< Instance access Flash method */
  BSP_OSPI_RAM_Latency_t      LatencyType;   /*!< Latency Type of Instance     */
  BSP_OSPI_RAM_BurstType_t    BurstType;     /*!< Burst Type of Instance       */
  BSP_OSPI_RAM_BurstLength_t  BurstLength;   /*!< Burst Length of Instance     */
} OSPI_RAM_Ctx_t;

/* Exported variables --------------------------------------------------------*/
/** @addtogroup B_U585I_IOT02A_OSPI_RAM_Exported_Variables OSPI RAM Exported Variables
  * @{
  */
OSPI_HandleTypeDef hospi_ram[OSPI_RAM_INSTANCES_NUMBER] = {0};
OSPI_RAM_Ctx_t Ospi_Ram_Ctx[OSPI_RAM_INSTANCES_NUMBER] = {{
    OSPI_ACCESS_NONE,
    BSP_OSPI_RAM_VARIABLE_LATENCY,
    BSP_OSPI_RAM_HYBRID_BURST,
    APS6408_BURST_16_BYTES
  }
};

/**
  * @brief  Initializes the OSPI MSP.
  * @param  hospi OSPI handle
  * @retval None
  */
static void OSPI_RAM_MspInit(const OSPI_HandleTypeDef *hospi)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  /* hospi unused argument(s) compilation warning */
  UNUSED(hospi);

  /* Enable the OctoSPI memory interface clock */
  OSPI_RAM_CLK_ENABLE();

  /* Reset the OctoSPI memory interface */
  OSPI_RAM_FORCE_RESET();
  OSPI_RAM_RELEASE_RESET();

  /* Enable GPIO clocks */
  OSPI_RAM_CLK_GPIO_CLK_ENABLE();
  OSPI_RAM_DQS_GPIO_CLK_ENABLE();
  OSPI_RAM_CS_GPIO_CLK_ENABLE();
  OSPI_RAM_D0_GPIO_CLK_ENABLE();
  OSPI_RAM_D1_GPIO_CLK_ENABLE();
  OSPI_RAM_D2_GPIO_CLK_ENABLE();
  OSPI_RAM_D3_GPIO_CLK_ENABLE();
  OSPI_RAM_D4_GPIO_CLK_ENABLE();
  OSPI_RAM_D5_GPIO_CLK_ENABLE();
  OSPI_RAM_D6_GPIO_CLK_ENABLE();
  OSPI_RAM_D7_GPIO_CLK_ENABLE();

  /* OctoSPI CS GPIO pin configuration  */
  GPIO_InitStruct.Pin       = OSPI_RAM_CS_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = OSPI_RAM_CS_PIN_AF;
  HAL_GPIO_Init(OSPI_RAM_CS_GPIO_PORT, &GPIO_InitStruct);

  /* OctoSPI DQS GPIO pin configuration  */
  GPIO_InitStruct.Pin       = OSPI_RAM_DQS_PIN;
  GPIO_InitStruct.Alternate = OSPI_RAM_DQS_PIN_AF;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  HAL_GPIO_Init(OSPI_RAM_DQS_GPIO_PORT, &GPIO_InitStruct);

  /* OctoSPI CLK GPIO pin configuration  */
  GPIO_InitStruct.Pin       = OSPI_RAM_CLK_PIN;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Alternate = OSPI_RAM_CLK_PIN_AF;
  HAL_GPIO_Init(OSPI_RAM_CLK_GPIO_PORT, &GPIO_InitStruct);

  /* OctoSPI D0 GPIO pin configuration  */
  GPIO_InitStruct.Pin       = OSPI_RAM_D0_PIN;
  GPIO_InitStruct.Alternate = OSPI_RAM_D0_PIN_AF;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  HAL_GPIO_Init(OSPI_RAM_D0_GPIO_PORT, &GPIO_InitStruct);

  /* OctoSPI D1 GPIO pin configuration  */
  GPIO_InitStruct.Pin       = OSPI_RAM_D1_PIN;
  GPIO_InitStruct.Alternate = OSPI_RAM_D1_PIN_AF;
  HAL_GPIO_Init(OSPI_RAM_D1_GPIO_PORT, &GPIO_InitStruct);

  /* OctoSPI D2 GPIO pin configuration  */
  GPIO_InitStruct.Pin       = OSPI_RAM_D2_PIN;
  GPIO_InitStruct.Alternate = OSPI_RAM_D2_PIN_AF;
  HAL_GPIO_Init(OSPI_RAM_D2_GPIO_PORT, &GPIO_InitStruct);

  /* OctoSPI D3 GPIO pin configuration  */
  GPIO_InitStruct.Pin       = OSPI_RAM_D3_PIN;
  GPIO_InitStruct.Alternate = OSPI_RAM_D3_PIN_AF;
  HAL_GPIO_Init(OSPI_RAM_D3_GPIO_PORT, &GPIO_InitStruct);

  /* OctoSPI D4 GPIO pin configuration  */
  GPIO_InitStruct.Pin       = OSPI_RAM_D4_PIN;
  GPIO_InitStruct.Alternate = OSPI_RAM_D4_PIN_AF;
  HAL_GPIO_Init(OSPI_RAM_D4_GPIO_PORT, &GPIO_InitStruct);

  /* OctoSPI D5 GPIO pin configuration  */
  GPIO_InitStruct.Pin       = OSPI_RAM_D5_PIN;
  GPIO_InitStruct.Alternate = OSPI_RAM_D5_PIN_AF;
  HAL_GPIO_Init(OSPI_RAM_D5_GPIO_PORT, &GPIO_InitStruct);

  /* OctoSPI D6 GPIO pin configuration  */
  GPIO_InitStruct.Pin       = OSPI_RAM_D6_PIN;
  GPIO_InitStruct.Alternate = OSPI_RAM_D6_PIN_AF;
  HAL_GPIO_Init(OSPI_RAM_D6_GPIO_PORT, &GPIO_InitStruct);

  /* OctoSPI D7 GPIO pin configuration  */
  GPIO_InitStruct.Pin       = OSPI_RAM_D7_PIN;
  GPIO_InitStruct.Alternate = OSPI_RAM_D7_PIN_AF;
  HAL_GPIO_Init(OSPI_RAM_D7_GPIO_PORT, &GPIO_InitStruct);
}

/**
  * @brief  This function enables delay block.
  * @param  hospi OSPI handle
  * @retval BSP status
  */
static int32_t OSPI_DLYB_Enable(OSPI_HandleTypeDef *hospi)
{
  HAL_OSPI_DLYB_CfgTypeDef dlyb_cfg, dlyb_cfg_test;
  int32_t ret = BSP_ERROR_NONE;
  uint32_t div_value = 4;

  /* Delay block configuration ------------------------------------------------ */
  if (HAL_OSPI_DLYB_GetClockPeriod(hospi, &dlyb_cfg) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }

  /* PhaseSel is divided by 4 (emperic value)*/
  dlyb_cfg.PhaseSel /= div_value;

  /* save the present configuration for check*/
  dlyb_cfg_test = dlyb_cfg;

  /*set delay block configuration*/
  if (HAL_OSPI_DLYB_SetConfig(hospi, &dlyb_cfg) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }

  /*check the set value*/
  if (HAL_OSPI_DLYB_GetConfig(hospi, &dlyb_cfg) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }

  if ((dlyb_cfg.PhaseSel != dlyb_cfg_test.PhaseSel) || (dlyb_cfg.Units != dlyb_cfg_test.Units))
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Initializes the OSPI interface.
  * @param  hospi          OSPI handle
  * @param  Init           OSPI config structure
  * @retval BSP status
  */
static HAL_StatusTypeDef MX_OSPI_RAM_Init(OSPI_HandleTypeDef *hospi, MX_OSPI_InitTypeDef *Init)
{
  /* OctoSPI initialization */
  hospi->Instance = OCTOSPI1;

  hospi->Init.FifoThreshold         = 1;
  hospi->Init.DualQuad              = HAL_OSPI_DUALQUAD_DISABLE;
  hospi->Init.MemoryType            = HAL_OSPI_MEMTYPE_APMEMORY;
  hospi->Init.DeviceSize            = Init->MemorySize;
  hospi->Init.ChipSelectHighTime    = 2;
  hospi->Init.ClockMode             = HAL_OSPI_CLOCK_MODE_0;
  hospi->Init.ClockPrescaler        = Init->ClockPrescaler;
  hospi->Init.SampleShifting        = Init->SampleShifting;
  hospi->Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_ENABLE;
  hospi->Init.ChipSelectBoundary    = 10;
  hospi->Init.FreeRunningClock      = HAL_OSPI_FREERUNCLK_DISABLE;
  hospi->Init.DelayBlockBypass      = HAL_OSPI_DELAY_BLOCK_USED;
  hospi->Init.WrapSize              = HAL_OSPI_WRAP_NOT_SUPPORTED;
  hospi->Init.MaxTran               = 0;
  hospi->Init.Refresh               = 320;

  return HAL_OSPI_Init(hospi);
}

/**
  * @brief  Initializes the OSPI interface.
  * @param  Instance   OSPI Instance
  * @retval BSP status
  */
int32_t BSP_OSPI_RAM_Init(uint32_t Instance)
{
  MX_OSPI_InitTypeDef ospi_init;
  int32_t ret = BSP_ERROR_NONE;

  /* MR0 register for read and write */
  uint8_t regW_MR0[2] = {0x24, 0x0D};

  /* MR8 register for read and write */
  uint8_t regW_MR8[2] = {0x0B, 0x08};

  /* Check if the instance is supported */
  if (Instance >= OSPI_RAM_INSTANCES_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Check if the instance is already initialized */
    if (Ospi_Ram_Ctx[Instance].IsInitialized == OSPI_ACCESS_NONE)
    {
#if (USE_HAL_OSPI_REGISTER_CALLBACKS == 0)
      /* Msp OSPI initialization */
      OSPI_RAM_MspInit(&hospi_ram[Instance]);
#else
      /* Register the OSPI MSP Callbacks */
      if (OspiRam_IsMspCbValid[Instance] == 0UL)
      {
        if (BSP_OSPI_RAM_RegisterDefaultMspCallbacks(Instance) != BSP_ERROR_NONE)
        {
          return BSP_ERROR_PERIPH_FAILURE;
        }
      }
#endif /* USE_HAL_OSPI_REGISTER_CALLBACKS */

      /* Fill config structure */
      ospi_init.ClockPrescaler = 2;
      ospi_init.MemorySize     = 23; /* 64 MBits */
      ospi_init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;

      /* STM32 OSPI interface initialization */
      if (MX_OSPI_RAM_Init(&hospi_ram[Instance], &ospi_init) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      /* Update current status parameter */
      Ospi_Ram_Ctx[Instance].IsInitialized = OSPI_ACCESS_INDIRECT;
      Ospi_Ram_Ctx[Instance].LatencyType   = BSP_OSPI_RAM_FIXED_LATENCY;
      Ospi_Ram_Ctx[Instance].BurstType     = BSP_OSPI_RAM_LINEAR_BURST;
    }
  }

  /* Configure Read Latency and drive Strength */
  if (APS6408_WriteReg(&hospi_ram[Instance], 0x00, *regW_MR0) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }

  /* Configure Burst Length */
  if (APS6408_WriteReg(&hospi_ram[Instance], 0x08, *regW_MR8) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Configure the OSPI in memory-mapped mode
  * @param  Instance  OSPI instance
  * @retval BSP status
  */
int32_t BSP_OSPI_RAM_EnableMemoryMappedMode(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if (Instance >= OSPI_RAM_INSTANCES_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  /* OSPI Delay Block enable */
  else if (OSPI_DLYB_Enable(&hospi_ram[Instance]) != BSP_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    if (APS6408_EnableMemoryMappedMode(&hospi_ram[0], DUMMY_CLOCK_CYCLES_READ, DUMMY_CLOCK_CYCLES_WRITE,
                                       1) != APS6408_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }

  /* Return BSP status */
  return ret;
}

void TargetBSP_Init(void)
{
  DCACHE_HandleTypeDef hdcache1;
  hdcache1.Instance = DCACHE1;
  hdcache1.Init.ReadBurstType = DCACHE_READ_BURST_WRAP;
  if (HAL_DCACHE_Init(&hdcache1) != HAL_OK)
  {
    error("HAL_DCACHE_Init");
  }  
  if(BSP_OSPI_RAM_Init(0) != BSP_ERROR_NONE)
  {
    error("BSP_OSPI_RAM_Init");
  }
  if(BSP_OSPI_RAM_EnableMemoryMappedMode(0) != BSP_ERROR_NONE)
  {
    error("BSP_OSPI_RAM_EnableMemoryMappedMode");
  }
}
